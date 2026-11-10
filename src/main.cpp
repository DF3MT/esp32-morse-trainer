#include <Arduino.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include "config.h"
#include "sidetone.h"
#include "keyer.h"
#include "decoder.h"
#include "display.h"
#include "menu.h"
#include "ble_manager.h"
#include "web_manager.h"
#include <WiFi.h>

// ============================================================
// Global objects
// ============================================================
IambicKeyer     keyer(IambicMode::MODE_A, WPM_DEFAULT);
MorseDecoder    decoder;
DisplayManager  display;
MenuController  menu(keyer, display);

// Inactivity timer & Deep Sleep
static volatile unsigned long lastActivityMs = 0;

static void resetInactivityTimer() {
    lastActivityMs = millis();
}

static void enterDeepSleep() {
    Serial.println("[PWR] Entering Deep Sleep due to 2 minutes of inactivity...");

    // 1. Show sleep message on display
    display.showSleepScreen();
    vTaskDelay(1500 / portTICK_PERIOD_MS);

    // 2. Shut down peripherals
    Sidetone::off();
    BleManager::end();
    WiFi.mode(WIFI_OFF);
    display.sleep();

    // 3. Configure RTC GPIO wakeup on Rotary Encoder Button (PIN_ENC_SW = GPIO 27)
    // GPIO 27 is active-LOW (pulled to GND on click)
    rtc_gpio_init((gpio_num_t)PIN_ENC_SW);
    rtc_gpio_set_direction((gpio_num_t)PIN_ENC_SW, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en((gpio_num_t)PIN_ENC_SW);
    rtc_gpio_pulldown_dis((gpio_num_t)PIN_ENC_SW);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_ENC_SW, 0); // 0 = LOW

    // 4. Ensure backlight GPIO 4 stays low during sleep
    rtc_gpio_init((gpio_num_t)PIN_TFT_BL);
    rtc_gpio_set_direction((gpio_num_t)PIN_TFT_BL, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level((gpio_num_t)PIN_TFT_BL, 0);

    // Wait until button is released before entering sleep so it doesn't wake immediately
    while (digitalRead(PIN_ENC_SW) == LOW) {
        delay(10);
    }
    delay(50);

    Serial.println("[PWR] Deep sleep started. Wakeup via Rotary Encoder Button.");
    esp_deep_sleep_start();
}

// Decoded text ring buffer (last DECODED_CHARS_MAX chars)
static char decodedText[DECODED_CHARS_MAX + 1] = "";
static portMUX_TYPE textMux = portMUX_INITIALIZER_UNLOCKED;

// Training state
static TrainingChallenge challenge;
static bool  trainingWaitingForSend = false;
static char  trainingTarget[8] = "K";
static char  trainingSent[8] = "";
static TrainingSet lastTrainingSet = TrainingSet::KOCH;

// Display state tracking
static bool displayDirty = true;

// Helper: Pick next training target based on configured TrainingSet & Koch level
static void getNextTrainingTarget(char* outTarget, size_t maxLen) {
    const Settings& s = menu.settings();
    switch (s.trainingSet) {
        case TrainingSet::KOCH: {
            uint8_t count = constrain((int)s.kochLevel + 1, 2, 40);
            char c = MenuController::KOCH_SEQUENCE[random(count)];
            snprintf(outTarget, maxLen, "%c", c);
            return;
        }
        case TrainingSet::LETTERS: {
            const char* letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
            snprintf(outTarget, maxLen, "%c", letters[random(26)]);
            return;
        }
        case TrainingSet::NUMBERS: {
            const char* digits = "0123456789";
            snprintf(outTarget, maxLen, "%c", digits[random(10)]);
            return;
        }
        case TrainingSet::ALL_CHARS: {
            const char* all = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/=-:";
            snprintf(outTarget, maxLen, "%c", all[random(strlen(all))]);
            return;
        }
        case TrainingSet::Q_CODES: {
            static const char* qCodes[] = {
                "QTH", "QSL", "QSO", "QRP", "QRZ",
                "QRM", "QRN", "QSY", "QRQ", "QRS"
            };
            const size_t n = sizeof(qCodes) / sizeof(qCodes[0]);
            snprintf(outTarget, maxLen, "%s", qCodes[random(n)]);
            return;
        }
        case TrainingSet::QSO_ABBR: {
            // Gebräuchlichste CW-Verkehrsabkürzungen aus qrp4fun.de/de/s3_02.htm
            static const char* abbrs[] = {
                "CQ", "73", "5NN", "GM", "GA", "GE", "GN",
                "PSE", "TNX", "RST", "NAME", "HW?", "WX",
                "RIG", "ANT", "FB", "RPT", "AGN", "CUL", "BCNU",
                "SKED", "ES", "DE", "K", "BK"
            };
            const size_t n = sizeof(abbrs) / sizeof(abbrs[0]);
            snprintf(outTarget, maxLen, "%s", abbrs[random(n)]);
            return;
        }
        case TrainingSet::PROSIGNS: {
            // Offizielle Verkehrszeichen ohne Pause (qrp4fun.de/de/s3_01.htm)
            static const char* prosigns[] = {
                "<SK>", "<AR>", "<BT>", "<AS>", "<KN>",
                "<KA>", "<BK>", "<VE>", "<HH>", "<SOS>"
            };
            const size_t n = sizeof(prosigns) / sizeof(prosigns[0]);
            snprintf(outTarget, maxLen, "%s", prosigns[random(n)]);
            return;
        }
    }
    snprintf(outTarget, maxLen, "K");
}

// ============================================================
// Callbacks — called from keyer task (Core 0)
// ============================================================

// Element emitted by keyer → push to decoder & live web stream
void onElement(KeyerElement e) {
    resetInactivityTimer();
    decoder.push(e);
    WebManager::pushElement(e);
    displayDirty = true;
}

// Token decoded (character, prosign, or space) → append to text buffer & BLE & Web
void onToken(const char* token) {
    if (!token || !token[0]) return;
    resetInactivityTimer();
    int currentWpm = keyer.isAutoSpeed() ? keyer.getWpm() : menu.settings().wpm;
    WebManager::pushToken(token, currentWpm);
    Serial.printf("[MAIN] onToken: '%s'\n", token);

    portENTER_CRITICAL(&textMux);
    size_t len = strlen(decodedText);
    size_t tLen = strlen(token);

    // Prevent leading spaces or multiple consecutive spaces in decode buffer
    if (token[0] == ' ') {
        if (len == 0 || decodedText[len - 1] == ' ') {
            portEXIT_CRITICAL(&textMux);
            return;
        }
    }

    // Scroll if needed to fit token
    while (len + tLen >= DECODED_CHARS_MAX) {
        memmove(decodedText, decodedText + 1, DECODED_CHARS_MAX);
        len--;
    }
    strcpy(decodedText + len, token);
    portEXIT_CRITICAL(&textMux);

    // Training mode verification
    if (menu.currentScreen() == AppScreen::TRAINING && trainingWaitingForSend) {
        if (token[0] == ' ') {
            // Word space emitted -> if operator keyed something, evaluate now
            if (strlen(trainingSent) > 0) {
                challenge.correct = (strcmp(trainingSent, trainingTarget) == 0);
                strncpy(challenge.sent, trainingSent, sizeof(challenge.sent) - 1);
                trainingWaitingForSend = false;
            }
        } else {
            // Append token to trainingSent
            strncat(trainingSent, token, sizeof(trainingSent) - strlen(trainingSent) - 1);
            strncpy(challenge.sent, trainingSent, sizeof(challenge.sent) - 1);

            // Check exact match
            bool match = false;
            if (strcmp(trainingSent, trainingTarget) == 0) {
                match = true;
            } else if (strcmp(trainingTarget, "=") == 0 && strcmp(trainingSent, "<BT>") == 0) {
                match = true;
            } else if (strcmp(trainingTarget, "+") == 0 && strcmp(trainingSent, "<AR>") == 0) {
                match = true;
            }

            if (match) {
                challenge.correct = true;
                trainingWaitingForSend = false;
            } else {
                // Check if current sent text is still a valid prefix of target
                size_t sLen = strlen(trainingSent);
                size_t targetLen = strlen(trainingTarget);
                if (sLen >= targetLen || strncmp(trainingTarget, trainingSent, sLen) != 0) {
                    // Mismatch! Operator made a mistake or sent wrong character
                    challenge.correct = false;
                    trainingWaitingForSend = false;
                }
            }
        }
    }

    // Push token to Bluetooth HID keyboard
    BleManager::pushString(token);

    displayDirty = true;
}

// ============================================================
// FreeRTOS Tasks
// ============================================================

// Core 0: keyer + decoder tick (1ms loop, high priority)
void keyerTask(void* pvParams) {
    for (;;) {
        keyer.update();
        if (keyer.isKeyDown()) {
            resetInactivityTimer();
        }
        decoder.tick(keyer.getT(), keyer.isKeyDown());
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

// Core 1: UI + menu + BLE + Web (20ms loop, normal priority)
void uiTask(void* pvParams) {
    bool lastKeyDown = false;
    int  lastWpm     = -1;
    AppScreen lastScreen = AppScreen::DECODE;
    bool lastInMenu  = false;
    bool lastBleConn = false;
    bool lastWifiActive = false;
    uint8_t lastWifiClients = 0;

    for (;;) {
        menu.tick();
        BleManager::tick();
        WebManager::tick();

        bool inMenu = menu.isInMenu();
        bool keyDown = keyer.isKeyDown();
        int wpm = keyer.isAutoSpeed() ? keyer.getWpm() : menu.settings().wpm;
        AppScreen curScreen = menu.currentScreen();
        bool bleConn = BleManager::isConnected();
        bool bleEn   = menu.settings().bleKeyboard;
        bool wifiActive = WebManager::isApActive();
        uint8_t wifiClients = WebManager::getClientCount();

        if (inMenu != lastInMenu) {
            displayDirty = true;
            lastInMenu = inMenu;
        }

        if (bleConn != lastBleConn) {
            displayDirty = true;
            lastBleConn = bleConn;
        }

        if (wifiActive != lastWifiActive || wifiClients != lastWifiClients) {
            displayDirty = true;
            lastWifiActive = wifiActive;
            lastWifiClients = wifiClients;
        }

        // Reset inactivity timer on any user interaction
        if (inMenu || keyDown || wpm != lastWpm ||
            digitalRead(PIN_DIT) == LOW || digitalRead(PIN_DAH) == LOW ||
            digitalRead(PIN_ENC_SW) == LOW) {
            resetInactivityTimer();
        }

        // Push WPM update to web clients if speed changed
        if (wpm != lastWpm) {
            WebManager::pushWpm(wpm);
        }

        // Deep Sleep Inactivity Check (only outside menu)
        uint8_t sleepMin = menu.settings().sleepTimeoutMin;
        if (sleepMin > 0 && !inMenu) {
            unsigned long timeoutMs = (unsigned long)sleepMin * 60000UL;
            if (millis() - lastActivityMs >= timeoutMs) {
                enterDeepSleep();
            }
        }

        if (!inMenu) {
            if (keyDown != lastKeyDown || wpm != lastWpm || curScreen != lastScreen) {
                displayDirty = true;
                lastKeyDown  = keyDown;
                lastWpm      = wpm;
                lastScreen   = curScreen;
            }

            if (displayDirty) {
                displayDirty = false;

                if (curScreen == AppScreen::DECODE) {
                    char textCopy[DECODED_CHARS_MAX + 1];
                    portENTER_CRITICAL(&textMux);
                    memcpy(textCopy, decodedText, sizeof(decodedText));
                    portEXIT_CRITICAL(&textMux);
                    display.showDecodeScreen(
                        textCopy, wpm,
                        keyer.isAutoSpeed(),
                        keyer.isAutoSpeedReady(),
                        keyer.isKeyDown(),
                        bleEn, bleConn,
                        wifiActive, wifiClients);
                } else if (curScreen == AppScreen::TRAINING) {
                    challenge.wpm       = wpm;
                    challenge.autoMode  = keyer.isAutoSpeed();
                    challenge.autoReady = keyer.isAutoSpeedReady();

                    // If training set changed, refresh target
                    if (menu.settings().trainingSet != lastTrainingSet) {
                        lastTrainingSet = menu.settings().trainingSet;
                        getNextTrainingTarget(trainingTarget, sizeof(trainingTarget));
                        trainingSent[0] = '\0';
                        trainingWaitingForSend = true;
                    }

                    if (trainingWaitingForSend) {
                        strncpy(challenge.target, trainingTarget, sizeof(challenge.target) - 1);
                        challenge.target[sizeof(challenge.target) - 1] = '\0';
                        strncpy(challenge.sent, trainingSent, sizeof(challenge.sent) - 1);
                        challenge.sent[sizeof(challenge.sent) - 1] = '\0';
                    }
                    display.showTrainingScreen(challenge, keyer.isKeyDown(), bleEn, bleConn, wifiActive, wifiClients);

                    // Advance to next target after 1.2s of feedback
                    if (!trainingWaitingForSend) {
                        vTaskDelay(1200 / portTICK_PERIOD_MS);
                        getNextTrainingTarget(trainingTarget, sizeof(trainingTarget));
                        trainingSent[0] = '\0';
                        trainingWaitingForSend = true;
                        displayDirty = true;
                    }
                }
            }
        }

        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}

// ============================================================
// setup()
// ============================================================
void setup() {
    Serial.begin(115200);

    // Check wakeup reason from Deep Sleep
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("[PWR] Woken up from Deep Sleep by Rotary Encoder Button!");
        // Wait until button is released so it doesn't open the menu right away
        while (digitalRead(PIN_ENC_SW) == LOW) {
            delay(10);
        }
        delay(80); // Debounce button release
    }

    // Paddle pins
    pinMode(PIN_DIT, INPUT_PULLUP);
    pinMode(PIN_DAH, INPUT_PULLUP);

    // Encoder button
    pinMode(PIN_ENC_SW, INPUT_PULLUP);

    // Sidetone
    Sidetone::begin(PIN_BUZZER, SIDETONE_HZ);

    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        Sidetone::playBeep(50); // Friendly wakeup chirp
    }

    // Display
    display.begin();

    // Menu (loads settings from NVS, applies to keyer, display brightness & hardware)
    menu.begin();

    // Wi-Fi Access Point & Real-time Web Terminal (starts if enabled in menu settings)
    WebManager::begin(menu.settings().wifiEnabled);

    // On cold boot, wait indefinitely on splash screen until user presses paddle or rotary encoder
    if (wakeup_reason != ESP_SLEEP_WAKEUP_EXT0) {
        // Wait for any button or paddle press
        while (digitalRead(PIN_DIT) == HIGH &&
               digitalRead(PIN_DAH) == HIGH &&
               digitalRead(PIN_ENC_SW) == HIGH) {
            WebManager::tick();
            delay(10);
        }

        // Wait until released so it doesn't immediately key a dit or open the menu
        while (digitalRead(PIN_DIT) == LOW ||
               digitalRead(PIN_DAH) == LOW ||
               digitalRead(PIN_ENC_SW) == LOW) {
            WebManager::tick();
            delay(10);
        }
        delay(60); // Debounce button release
        Sidetone::playBeep(45); // Audible confirmation
    }

    // Bluetooth Low Energy HID Keyboard
    BleManager::begin(menu.settings().bleKeyboard, menu.settings().bleLayout);

    // Register callbacks
    keyer.onElement(onElement);
    decoder.onToken(onToken);

    // Training init
    lastTrainingSet = menu.settings().trainingSet;
    getNextTrainingTarget(trainingTarget, sizeof(trainingTarget));
    trainingSent[0] = '\0';
    trainingWaitingForSend = true;
    challenge = {};
    strncpy(challenge.target, trainingTarget, sizeof(challenge.target) - 1);
    challenge.target[sizeof(challenge.target) - 1] = '\0';

    resetInactivityTimer();

    // Start FreeRTOS tasks
    xTaskCreatePinnedToCore(keyerTask, "Keyer", KEYER_TASK_STACK, nullptr, 10, nullptr, 0);
    xTaskCreatePinnedToCore(uiTask,    "UI",    UI_TASK_STACK,    nullptr,  5, nullptr, 1);

    Serial.println("Morse Trainer ready with enhanced rotary menu.");
}

// ============================================================
// loop() — nothing here; FreeRTOS tasks handle everything
// ============================================================
void loop() {
    vTaskDelay(portMAX_DELAY);
}

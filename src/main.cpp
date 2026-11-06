#include <Arduino.h>
#include "config.h"
#include "sidetone.h"
#include "keyer.h"
#include "decoder.h"
#include "display.h"
#include "menu.h"

// ============================================================
// Global objects
// ============================================================
IambicKeyer     keyer(IambicMode::MODE_A, WPM_DEFAULT);
MorseDecoder    decoder;
DisplayManager  display;
MenuController  menu(keyer, display);

// Decoded text ring buffer (last DECODED_CHARS_MAX chars)
static char decodedText[DECODED_CHARS_MAX + 1] = "";
static portMUX_TYPE textMux = portMUX_INITIALIZER_UNLOCKED;

// Training state
static TrainingChallenge challenge;
static bool  trainingWaitingForSend = false;
static char  trainingTarget = 'K';
static char  trainingSent[8] = "";

// Display state tracking
static bool displayDirty = true;

// Helper: Pick next training target based on configured TrainingSet & Koch level
static char getNextTrainingTarget() {
    const Settings& s = menu.settings();
    switch (s.trainingSet) {
        case TrainingSet::KOCH: {
            uint8_t count = constrain((int)s.kochLevel + 1, 2, 40);
            return MenuController::KOCH_SEQUENCE[random(count)];
        }
        case TrainingSet::LETTERS: {
            const char* letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
            return letters[random(26)];
        }
        case TrainingSet::NUMBERS: {
            const char* digits = "0123456789";
            return digits[random(10)];
        }
        case TrainingSet::ALL_CHARS: {
            const char* all = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/";
            return all[random(strlen(all))];
        }
        case TrainingSet::Q_CODES: {
            const char* qchars = "CQTHSLRNMG";
            return qchars[random(strlen(qchars))];
        }
    }
    return 'K';
}

// ============================================================
// Callbacks — called from keyer task (Core 0)
// ============================================================

// Element emitted by keyer → push to decoder
void onElement(KeyerElement e) {
    decoder.push(e);
    displayDirty = true;
}

// Character decoded → append to text buffer
void onChar(char c) {
    portENTER_CRITICAL(&textMux);
    size_t len = strlen(decodedText);

    // Prevent leading spaces or multiple consecutive spaces in decode buffer
    if (c == ' ') {
        if (len == 0 || decodedText[len - 1] == ' ') {
            portEXIT_CRITICAL(&textMux);
            return;
        }
    }

    if (len >= DECODED_CHARS_MAX) {
        // Scroll: shift left by 1 character
        memmove(decodedText, decodedText + 1, DECODED_CHARS_MAX);
        len = DECODED_CHARS_MAX - 1;
    }
    decodedText[len]     = c;
    decodedText[len + 1] = '\0';
    portEXIT_CRITICAL(&textMux);

    // Training mode: check if sent char matches target (ignore word spaces)
    if (menu.currentScreen() == AppScreen::TRAINING && trainingWaitingForSend && c != ' ') {
        challenge.sent[0] = c;
        challenge.sent[1] = '\0';
        challenge.correct = (c == trainingTarget);
        challenge.target[0] = trainingTarget;
        challenge.target[1] = '\0';
        trainingWaitingForSend = false;
    }

    displayDirty = true;
}

// ============================================================
// FreeRTOS Tasks
// ============================================================

// Core 0: keyer + decoder tick (1ms loop, high priority)
void keyerTask(void* pvParams) {
    for (;;) {
        keyer.update();
        decoder.tick(keyer.getT(), keyer.isKeyDown());
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

// Core 1: UI + menu (20ms loop, normal priority)
void uiTask(void* pvParams) {
    bool lastKeyDown = false;
    int  lastWpm     = -1;
    AppScreen lastScreen = AppScreen::DECODE;
    bool lastInMenu  = false;

    for (;;) {
        menu.tick();

        bool inMenu = menu.isInMenu();
        bool keyDown = keyer.isKeyDown();
        int wpm = keyer.isAutoSpeed() ? keyer.getWpm() : menu.settings().wpm;
        AppScreen curScreen = menu.currentScreen();

        if (inMenu != lastInMenu) {
            displayDirty = true;
            lastInMenu = inMenu;
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
                        keyer.isKeyDown());
                } else if (curScreen == AppScreen::TRAINING) {
                    challenge.wpm       = wpm;
                    challenge.autoMode  = keyer.isAutoSpeed();
                    challenge.autoReady = keyer.isAutoSpeedReady();
                    if (trainingWaitingForSend) {
                        challenge.target[0] = trainingTarget;
                        challenge.target[1] = '\0';
                        challenge.sent[0]   = '\0';
                    }
                    display.showTrainingScreen(challenge);

                    // Advance to next target after 1.2s of feedback
                    if (!trainingWaitingForSend) {
                        vTaskDelay(1200 / portTICK_PERIOD_MS);
                        trainingTarget = getNextTrainingTarget();
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

    // Paddle pins
    pinMode(PIN_DIT, INPUT_PULLUP);
    pinMode(PIN_DAH, INPUT_PULLUP);

    // Encoder button
    pinMode(PIN_ENC_SW, INPUT_PULLUP);

    // Sidetone
    Sidetone::begin(PIN_BUZZER, SIDETONE_HZ);

    // Display
    display.begin();

    // Menu (loads settings from NVS, applies to keyer & hardware)
    menu.begin();

    // Register callbacks
    keyer.onElement(onElement);
    decoder.onChar(onChar);

    // Training init
    trainingTarget = 'K';
    trainingWaitingForSend = true;
    challenge = {};

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

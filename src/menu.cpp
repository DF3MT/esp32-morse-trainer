#include "menu.h"
#include "config.h"
#include "sidetone.h"
#include "ble_manager.h"
#include <Preferences.h>

static void IRAM_ATTR encoderISR_wrapper();
static AiEsp32RotaryEncoder* g_encoder = nullptr;

static void IRAM_ATTR encoderISR_wrapper() {
    if (g_encoder) g_encoder->readEncoder_ISR();
}

const char MenuController::KOCH_SEQUENCE[] = "KMRSUAPTLONWJEF0Y,VG5/Q9ZH38B?427C1D6X";

MenuController::MenuController(IambicKeyer& keyer, DisplayManager& display, MorseDecoder& decoder)
    : _encoder(PIN_ENC_DT, PIN_ENC_CLK, PIN_ENC_SW, -1, 4) // DT and CLK swapped to invert rotation direction
    , _keyer(keyer)
    , _display(display)
    , _decoder(decoder)
{}

void MenuController::begin() {
    _encoder.begin();
    _encoder.setup(encoderISR_wrapper);
    g_encoder = &_encoder;

    loadSettings();
    applySettings(false);

    // Initial encoder setup: WPM direct adjust
    _encoder.setBoundaries(WPM_MIN, WPM_MAX, false);
    _encoder.setEncoderValue(_settings.wpm);
    _encoder.setAcceleration(50);
}

void MenuController::openMenu() {
    _inMenu = true;
    _isEditing = false;
    _encoder.setBoundaries(0, ITEM_COUNT - 1, false);
    _encoder.setEncoderValue(_menuIndex);
    _encoder.setAcceleration(0);
}

void MenuController::closeMenu() {
    _inMenu = false;
    _isEditing = false;
    saveSettings();

    // Restore encoder to WPM mode
    _encoder.setBoundaries(WPM_MIN, WPM_MAX, false);
    _encoder.setEncoderValue(_settings.wpm);
    _encoder.setAcceleration(50);
}

void MenuController::tick() {
    if (_inMenu) {
        if (_isEditing) {
            handleMenuEditing();
        } else {
            handleMenuNavigation();
        }
    } else {
        handleMainScreen();
    }
}

// ============================================================
// Main Screen: Quick WPM adjust by turning; Click opens Menu
// ============================================================
void MenuController::handleMainScreen() {
    if (_encoder.encoderChanged()) {
        _settings.wpm = (int)_encoder.readEncoder();
        _settings.wpm = constrain(_settings.wpm, WPM_MIN, WPM_MAX);
        _keyer.setWpm(_settings.wpm);
    }

    if (checkButtonClicked()) {
        Sidetone::playBeep(25);
        openMenu();
    }
}

// ============================================================
// Menu Browsing: Scroll items, Click toggles or enters Edit Mode
// ============================================================
void MenuController::handleMenuNavigation() {
    if (_encoder.encoderChanged()) {
        _menuIndex = (int8_t)constrain((long)_encoder.readEncoder(), 0, ITEM_COUNT - 1);
    }

    // Build and display menu view
    MenuItemView items[ITEM_COUNT];
    buildMenuItems(items);
    _display.showMenuScreen(items, ITEM_COUNT, _menuIndex, false);

    if (checkButtonClicked()) {
        Sidetone::playBeep(30);

        switch (_menuIndex) {
            case ITEM_BACK:
                closeMenu();
                return;

            case ITEM_AUTOSPEED:
                _settings.autoSpeed = !_settings.autoSpeed;
                applySettings();
                saveSettings();
                return;

            case ITEM_KEYER_MODE:
                _settings.keyerMode = (_settings.keyerMode == IambicMode::MODE_A)
                                      ? IambicMode::MODE_B : IambicMode::MODE_A;
                applySettings();
                saveSettings();
                return;

            case ITEM_PADDLE_SWAP:
                _settings.paddleSwap = !_settings.paddleSwap;
                applySettings();
                saveSettings();
                return;

            case ITEM_PAUSE_TOL:
                _settings.pauseTolerance = (_settings.pauseTolerance + 1) % 3;
                applySettings();
                saveSettings();
                return;

            case ITEM_BLE_KEYBOARD:
                _settings.bleKeyboard = !_settings.bleKeyboard;
                applySettings();
                saveSettings();
                return;

            case ITEM_BLE_LAYOUT:
                _settings.bleLayout = (_settings.bleLayout == BleLayout::GERMAN_DE)
                                      ? BleLayout::US_EN : BleLayout::GERMAN_DE;
                applySettings();
                saveSettings();
                return;

            // Items that switch to interactive edit mode:
            case ITEM_MODE:
                _isEditing = true;
                _encoder.setBoundaries(0, 1, false);
                _encoder.setEncoderValue((long)_settings.appScreen);
                break;

            case ITEM_WPM:
                _isEditing = true;
                _encoder.setBoundaries(WPM_MIN, WPM_MAX, false);
                _encoder.setEncoderValue(_settings.wpm);
                break;

            case ITEM_TONE:
                _isEditing = true;
                _encoder.setBoundaries(400 / 50, 2500 / 50, false);
                _encoder.setEncoderValue(_settings.sidetoneHz / 50);
                break;

            case ITEM_VOLUME:
                _isEditing = true;
                _encoder.setBoundaries(0, 10, false);
                _encoder.setEncoderValue(_settings.sidetoneVol / 10);
                break;

            case ITEM_BRIGHTNESS:
                _isEditing = true;
                _encoder.setBoundaries(1, 10, false);
                _encoder.setEncoderValue(_settings.brightness / 10);
                break;

            case ITEM_TRAIN_SET:
                _isEditing = true;
                _encoder.setBoundaries(0, 6, false);
                _encoder.setEncoderValue((long)_settings.trainingSet);
                break;

            case ITEM_KOCH_LVL:
                _isEditing = true;
                _encoder.setBoundaries(1, 40, false);
                _encoder.setEncoderValue(_settings.kochLevel);
                break;

            case ITEM_SLEEP_TIMEOUT: {
                _isEditing = true;
                _encoder.setBoundaries(0, 4, false);
                long curIdx = 2; // Default 2 Min (index 2)
                if (_settings.sleepTimeoutMin == 0) curIdx = 0;
                else if (_settings.sleepTimeoutMin == 1) curIdx = 1;
                else if (_settings.sleepTimeoutMin == 2) curIdx = 2;
                else if (_settings.sleepTimeoutMin == 5) curIdx = 3;
                else if (_settings.sleepTimeoutMin == 10) curIdx = 4;
                _encoder.setEncoderValue(curIdx);
                break;
            }

            default:
                break;
        }
    }
}

// ============================================================
// Menu Editing: Turn knob to change parameter, Click to save & exit
// ============================================================
void MenuController::handleMenuEditing() {
    if (_encoder.encoderChanged()) {
        long val = _encoder.readEncoder();

        switch (_menuIndex) {
            case ITEM_MODE:
                _settings.appScreen = (val == 0) ? AppScreen::DECODE : AppScreen::TRAINING;
                break;

            case ITEM_WPM:
                _settings.wpm = (int)constrain(val, (long)WPM_MIN, (long)WPM_MAX);
                _keyer.setWpm(_settings.wpm);
                break;

            case ITEM_TONE:
                _settings.sidetoneHz = (uint16_t)(val * 50);
                Sidetone::setFrequency(_settings.sidetoneHz);
                Sidetone::playBeep(45);
                break;

            case ITEM_VOLUME:
                _settings.sidetoneVol = (uint8_t)(val * 10);
                applySettings();
                Sidetone::playBeep(45);
                break;

            case ITEM_BRIGHTNESS:
                _settings.brightness = (uint8_t)(val * 10);
                applySettings();
                break;

            case ITEM_TRAIN_SET:
                _settings.trainingSet = (TrainingSet)constrain(val, 0L, 6L);
                break;

            case ITEM_KOCH_LVL:
                _settings.kochLevel = (uint8_t)constrain(val, 1L, 40L);
                break;

            case ITEM_SLEEP_TIMEOUT: {
                static const uint8_t opts[] = { 0, 1, 2, 5, 10 };
                long idx = constrain(val, 0L, 4L);
                _settings.sleepTimeoutMin = opts[idx];
                break;
            }

            default:
                break;
        }
    }

    // Render menu with edit highlight
    MenuItemView items[ITEM_COUNT];
    buildMenuItems(items);
    _display.showMenuScreen(items, ITEM_COUNT, _menuIndex, true);

    // Click confirms edit and returns to menu browsing
    if (checkButtonClicked()) {
        Sidetone::playBeep(40);
        _isEditing = false;
        applySettings();
        saveSettings();

        // Restore encoder to menu list scrolling
        _encoder.setBoundaries(0, ITEM_COUNT - 1, false);
        _encoder.setEncoderValue(_menuIndex);
    }
}

// ============================================================
// Format items and current values for the display
// ============================================================
void MenuController::buildMenuItems(MenuItemView* viewItems) {
    // 0. Mode
    viewItems[ITEM_MODE].label = "Betriebsart";
    snprintf(viewItems[ITEM_MODE].value, sizeof(viewItems[ITEM_MODE].value),
             "%s", (_settings.appScreen == AppScreen::DECODE) ? "Decodieren" : "Training");
    viewItems[ITEM_MODE].isAction = false;

    // 1. WPM
    viewItems[ITEM_WPM].label = "Tempo (WPM)";
    snprintf(viewItems[ITEM_WPM].value, sizeof(viewItems[ITEM_WPM].value), "%d WPM", _settings.wpm);
    viewItems[ITEM_WPM].isAction = false;

    // 2. Auto-speed
    viewItems[ITEM_AUTOSPEED].label = "Auto-Tempo";
    snprintf(viewItems[ITEM_AUTOSPEED].value, sizeof(viewItems[ITEM_AUTOSPEED].value),
             "%s", _settings.autoSpeed ? "AN" : "AUS");
    viewItems[ITEM_AUTOSPEED].isAction = true;

    // 3. Keyer Mode
    viewItems[ITEM_KEYER_MODE].label = "Keyer-Modus";
    snprintf(viewItems[ITEM_KEYER_MODE].value, sizeof(viewItems[ITEM_KEYER_MODE].value),
             "%s", (_settings.keyerMode == IambicMode::MODE_A) ? "Modus A" : "Modus B");
    viewItems[ITEM_KEYER_MODE].isAction = true;

    // 4. Paddle Swap
    viewItems[ITEM_PADDLE_SWAP].label = "Paddle-Tausch";
    snprintf(viewItems[ITEM_PADDLE_SWAP].value, sizeof(viewItems[ITEM_PADDLE_SWAP].value),
             "%s", _settings.paddleSwap ? "Getauscht" : "Normal");
    viewItems[ITEM_PADDLE_SWAP].isAction = true;

    // 5. Pause Tolerance
    viewItems[ITEM_PAUSE_TOL].label = "Pausen-Toleranz";
    static const char* pauseNames[] = { "Standard", "Grosszuegig", "Sehr weit" };
    snprintf(viewItems[ITEM_PAUSE_TOL].value, sizeof(viewItems[ITEM_PAUSE_TOL].value),
             "%s", pauseNames[constrain((int)_settings.pauseTolerance, 0, 2)]);
    viewItems[ITEM_PAUSE_TOL].isAction = true;

    // 6. Tone
    viewItems[ITEM_TONE].label = "Tonhoehe";
    snprintf(viewItems[ITEM_TONE].value, sizeof(viewItems[ITEM_TONE].value), "%d Hz", _settings.sidetoneHz);
    viewItems[ITEM_TONE].isAction = false;

    // 6. Volume
    viewItems[ITEM_VOLUME].label = "Lautstaerke";
    snprintf(viewItems[ITEM_VOLUME].value, sizeof(viewItems[ITEM_VOLUME].value), "%d %%", _settings.sidetoneVol);
    viewItems[ITEM_VOLUME].isAction = false;

    // 7. Brightness
    viewItems[ITEM_BRIGHTNESS].label = "Helligkeit";
    snprintf(viewItems[ITEM_BRIGHTNESS].value, sizeof(viewItems[ITEM_BRIGHTNESS].value), "%d %%", _settings.brightness);
    viewItems[ITEM_BRIGHTNESS].isAction = false;

    // 8. BLE Keyboard
    viewItems[ITEM_BLE_KEYBOARD].label = "BLE-Tastatur";
    if (!_settings.bleKeyboard) {
        snprintf(viewItems[ITEM_BLE_KEYBOARD].value, sizeof(viewItems[ITEM_BLE_KEYBOARD].value), "Aus");
    } else if (BleManager::isConnected()) {
        snprintf(viewItems[ITEM_BLE_KEYBOARD].value, sizeof(viewItems[ITEM_BLE_KEYBOARD].value), "Verbunden");
    } else {
        snprintf(viewItems[ITEM_BLE_KEYBOARD].value, sizeof(viewItems[ITEM_BLE_KEYBOARD].value), "Wartet...");
    }
    viewItems[ITEM_BLE_KEYBOARD].isAction = true;

    // 9. BLE Layout
    viewItems[ITEM_BLE_LAYOUT].label = "BLE-Layout";
    snprintf(viewItems[ITEM_BLE_LAYOUT].value, sizeof(viewItems[ITEM_BLE_LAYOUT].value),
             "%s", (_settings.bleLayout == BleLayout::GERMAN_DE) ? "DE (QWERTZ)" : "US (QWERTY)");
    viewItems[ITEM_BLE_LAYOUT].isAction = true;

    // 8. Training Set
    viewItems[ITEM_TRAIN_SET].label = "Uebungssatz";
    const char* setName = "Koch";
    switch (_settings.trainingSet) {
        case TrainingSet::KOCH:      setName = "Koch"; break;
        case TrainingSet::LETTERS:   setName = "A-Z"; break;
        case TrainingSet::NUMBERS:   setName = "0-9"; break;
        case TrainingSet::ALL_CHARS: setName = "Alle"; break;
        case TrainingSet::Q_CODES:   setName = "Q-Codes"; break;
        case TrainingSet::QSO_ABBR:  setName = "QSO-Abk."; break;
        case TrainingSet::PROSIGNS:  setName = "Spruchz."; break;
    }
    snprintf(viewItems[ITEM_TRAIN_SET].value, sizeof(viewItems[ITEM_TRAIN_SET].value), "%s", setName);
    viewItems[ITEM_TRAIN_SET].isAction = false;

    // 9. Koch Level
    viewItems[ITEM_KOCH_LVL].label = "Koch-Stufe";
    snprintf(viewItems[ITEM_KOCH_LVL].value, sizeof(viewItems[ITEM_KOCH_LVL].value), "Stufe %d", _settings.kochLevel);
    viewItems[ITEM_KOCH_LVL].isAction = false;

    // 10. Sleep Timeout
    viewItems[ITEM_SLEEP_TIMEOUT].label = "Auto-Standby";
    if (_settings.sleepTimeoutMin == 0) {
        snprintf(viewItems[ITEM_SLEEP_TIMEOUT].value, sizeof(viewItems[ITEM_SLEEP_TIMEOUT].value), "Aus");
    } else {
        snprintf(viewItems[ITEM_SLEEP_TIMEOUT].value, sizeof(viewItems[ITEM_SLEEP_TIMEOUT].value), "%d Min", _settings.sleepTimeoutMin);
    }
    viewItems[ITEM_SLEEP_TIMEOUT].isAction = false;

    // 11. Back / Exit
    viewItems[ITEM_BACK].label = "< Zurueck";
    viewItems[ITEM_BACK].value[0] = '\0';
    viewItems[ITEM_BACK].isAction = true;
}

// ============================================================
// Debounced Button Reading
// ============================================================
bool MenuController::checkButtonClicked() {
    static unsigned long lastPressTime = 0;
    static bool lastState = HIGH;
    bool curState = digitalRead(PIN_ENC_SW);
    bool clicked = false;

    if (lastState == HIGH && curState == LOW) {
        unsigned long now = millis();
        if (now - lastPressTime > 180) { // 180ms debounce
            clicked = true;
            lastPressTime = now;
        }
    }
    lastState = curState;
    return clicked;
}

// ============================================================
// Apply settings to hardware & Keyer
// ============================================================
void MenuController::applySettings(bool playFeedback) {
    _keyer.setMode(_settings.keyerMode);
    _keyer.setAutoSpeed(_settings.autoSpeed);
    _keyer.setPaddleSwap(_settings.paddleSwap);
    _keyer.setWpm(_settings.wpm);

    Sidetone::setFrequency(_settings.sidetoneHz);

    // Map volume 0..100% to duty cycle (0..128 for 50% max square wave)
    uint8_t duty = (uint8_t)((_settings.sidetoneVol * 128) / 100);
    Sidetone::setVolume(duty);

    // Map brightness 10..100% to PWM duty (0..255)
    uint8_t bl = (uint8_t)((_settings.brightness * 255) / 100);
    _display.setBrightness(bl);

    // Sync BLE Keyboard state & layout
    BleManager::setEnabled(_settings.bleKeyboard);
    BleManager::setLayout(_settings.bleLayout);

    // Apply pause tolerances to decoder
    switch (_settings.pauseTolerance) {
        case 0: // Standard
            _decoder.setPauseTolerance(3.5f, 7.0f, 220, 500);
            break;
        case 2: // Sehr weit / Anfänger
            _decoder.setPauseTolerance(5.2f, 10.0f, 350, 850);
            break;
        case 1: // Grosszügig (empfohlen / default)
        default:
            _decoder.setPauseTolerance(4.2f, 8.0f, 280, 650);
            break;
    }

    if (playFeedback) {
        Sidetone::playBeep(40);
    }
}

// ============================================================
// Persistence via ESP32 Preferences (Flash NVS)
// ============================================================
void MenuController::saveSettings() {
    Preferences prefs;
    if (prefs.begin("morse", false)) {
        prefs.putInt("screen", (int)_settings.appScreen);
        prefs.putInt("wpm",    _settings.wpm);
        prefs.putBool("auto",  _settings.autoSpeed);
        prefs.putInt("mode",   (int)_settings.keyerMode);
        prefs.putBool("swap",  _settings.paddleSwap);
        prefs.putUChar("pauseTol", _settings.pauseTolerance);
        prefs.putUShort("tone",_settings.sidetoneHz);
        prefs.putUChar("vol",  _settings.sidetoneVol);
        prefs.putUChar("brt",  _settings.brightness);
        prefs.putBool("ble",   _settings.bleKeyboard);
        prefs.putUChar("blelay", (uint8_t)_settings.bleLayout);
        prefs.putInt("tset",   (int)_settings.trainingSet);
        prefs.putUChar("koch", _settings.kochLevel);
        prefs.putUChar("sleep", _settings.sleepTimeoutMin);
        prefs.end();
    }
}

void MenuController::loadSettings() {
    Preferences prefs;
    if (prefs.begin("morse", true)) {
        _settings.appScreen   = (AppScreen)prefs.getInt("screen", (int)AppScreen::DECODE);
        _settings.wpm         = prefs.getInt("wpm",      WPM_DEFAULT);
        _settings.autoSpeed   = prefs.getBool("auto",    false);
        _settings.keyerMode   = (IambicMode)prefs.getInt("mode", (int)IambicMode::MODE_A);
        _settings.paddleSwap  = prefs.getBool("swap",    false);
        _settings.pauseTolerance = prefs.getUChar("pauseTol", 1);
        _settings.sidetoneHz  = prefs.getUShort("tone",  800);
        _settings.sidetoneVol = prefs.getUChar("vol",    100);
        _settings.brightness  = prefs.getUChar("brt",    80);
        _settings.bleKeyboard = prefs.getBool("ble",     true);
        _settings.bleLayout   = (BleLayout)prefs.getUChar("blelay", (uint8_t)BleLayout::GERMAN_DE);
        _settings.trainingSet = (TrainingSet)prefs.getInt("tset", (int)TrainingSet::KOCH);
        _settings.kochLevel   = prefs.getUChar("koch",   4);
        _settings.sleepTimeoutMin = prefs.getUChar("sleep", SLEEP_TIMEOUT_DEFAULT_MIN);
        prefs.end();
    }
}

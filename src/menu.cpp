#include "menu.h"
#include "config.h"
#include "sidetone.h"
#include "ble_manager.h"
#include "web_manager.h"
#include <Preferences.h>

static void IRAM_ATTR encoderISR_wrapper();
static AiEsp32RotaryEncoder* g_encoder = nullptr;

static void IRAM_ATTR encoderISR_wrapper() {
    if (g_encoder) g_encoder->readEncoder_ISR();
}

const char MenuController::KOCH_SEQUENCE[] = "KMRSUAPTLONWJEF0Y,VG5/Q9ZH38B?427C1D6X";

MenuController::MenuController(IambicKeyer& keyer, DisplayManager& display)
    : _encoder(PIN_ENC_DT, PIN_ENC_CLK, PIN_ENC_SW, -1, 4) // DT and CLK swapped to invert rotation direction
    , _keyer(keyer)
    , _display(display)
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
    _bondsResetFeedback = false;
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

            case ITEM_BLE_CLEAR_BONDS:
                BleManager::clearBonds();
                _bondsResetFeedback = true;
                Sidetone::playBeep(60);
                return;

            case ITEM_WIFI:
                _settings.wifiEnabled = !_settings.wifiEnabled;
                applySettings();
                saveSettings();
                return;

            // Items that switch to interactive edit mode:
            case ITEM_LANGUAGE:
                _isEditing = true;
                _encoder.setBoundaries(0, LANGUAGE_COUNT - 1, false);
                _encoder.setEncoderValue((long)_settings.language);
                break;

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
            case ITEM_LANGUAGE:
                // Live preview: the menu re-renders in the selected language right away
                _settings.language = (Language)constrain(val, 0L, (long)(LANGUAGE_COUNT - 1));
                I18n::setLanguage(_settings.language);
                break;

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
    auto setValue = [&](int idx, const char* text) {
        snprintf(viewItems[idx].value, sizeof(viewItems[idx].value), "%s", text);
    };

    // 0. Language
    viewItems[ITEM_LANGUAGE].label = tr(Str::M_LANGUAGE);
    setValue(ITEM_LANGUAGE, I18n::languageName(_settings.language));
    viewItems[ITEM_LANGUAGE].isAction = false;

    // 1. Mode
    viewItems[ITEM_MODE].label = tr(Str::M_MODE);
    setValue(ITEM_MODE, (_settings.appScreen == AppScreen::DECODE) ? tr(Str::V_DECODE) : tr(Str::V_TRAINING));
    viewItems[ITEM_MODE].isAction = false;

    // 2. WPM / BpM (CPM)
    viewItems[ITEM_WPM].label = tr(Str::M_SPEED);
    snprintf(viewItems[ITEM_WPM].value, sizeof(viewItems[ITEM_WPM].value), tr(Str::V_SPEED_FMT), _settings.wpm, _settings.wpm * 5);
    viewItems[ITEM_WPM].isAction = false;

    // 3. Auto-speed
    viewItems[ITEM_AUTOSPEED].label = tr(Str::M_AUTOSPEED);
    setValue(ITEM_AUTOSPEED, _settings.autoSpeed ? tr(Str::V_ON) : tr(Str::V_OFF));
    viewItems[ITEM_AUTOSPEED].isAction = true;

    // 4. Keyer Mode
    viewItems[ITEM_KEYER_MODE].label = tr(Str::M_KEYER_MODE);
    setValue(ITEM_KEYER_MODE, (_settings.keyerMode == IambicMode::MODE_A) ? tr(Str::V_MODE_A) : tr(Str::V_MODE_B));
    viewItems[ITEM_KEYER_MODE].isAction = true;

    // 5. Paddle Swap
    viewItems[ITEM_PADDLE_SWAP].label = tr(Str::M_PADDLE_SWAP);
    setValue(ITEM_PADDLE_SWAP, _settings.paddleSwap ? tr(Str::V_SWAPPED) : tr(Str::V_NORMAL));
    viewItems[ITEM_PADDLE_SWAP].isAction = true;

    // 6. Tone
    viewItems[ITEM_TONE].label = tr(Str::M_TONE);
    snprintf(viewItems[ITEM_TONE].value, sizeof(viewItems[ITEM_TONE].value), "%d Hz", _settings.sidetoneHz);
    viewItems[ITEM_TONE].isAction = false;

    // 7. Volume
    viewItems[ITEM_VOLUME].label = tr(Str::M_VOLUME);
    snprintf(viewItems[ITEM_VOLUME].value, sizeof(viewItems[ITEM_VOLUME].value), "%d %%", _settings.sidetoneVol);
    viewItems[ITEM_VOLUME].isAction = false;

    // 8. Brightness
    viewItems[ITEM_BRIGHTNESS].label = tr(Str::M_BRIGHTNESS);
    snprintf(viewItems[ITEM_BRIGHTNESS].value, sizeof(viewItems[ITEM_BRIGHTNESS].value), "%d %%", _settings.brightness);
    viewItems[ITEM_BRIGHTNESS].isAction = false;

    // 9. BLE Keyboard
    viewItems[ITEM_BLE_KEYBOARD].label = tr(Str::M_BLE_KEYBOARD);
    if (!_settings.bleKeyboard) {
        setValue(ITEM_BLE_KEYBOARD, tr(Str::V_BLE_OFF));
    } else if (BleManager::isConnected()) {
        setValue(ITEM_BLE_KEYBOARD, tr(Str::V_BLE_CONNECTED));
    } else {
        setValue(ITEM_BLE_KEYBOARD, tr(Str::V_BLE_WAITING));
    }
    viewItems[ITEM_BLE_KEYBOARD].isAction = true;

    // 10. BLE Layout
    viewItems[ITEM_BLE_LAYOUT].label = tr(Str::M_BLE_LAYOUT);
    setValue(ITEM_BLE_LAYOUT, (_settings.bleLayout == BleLayout::GERMAN_DE) ? "DE (QWERTZ)" : "US (QWERTY)");
    viewItems[ITEM_BLE_LAYOUT].isAction = true;

    // 11. BLE Clear Bonds
    viewItems[ITEM_BLE_CLEAR_BONDS].label = tr(Str::M_BLE_BONDS);
    setValue(ITEM_BLE_CLEAR_BONDS, _bondsResetFeedback ? tr(Str::V_BONDS_CLEARED) : tr(Str::V_BONDS_RESET));
    viewItems[ITEM_BLE_CLEAR_BONDS].isAction = true;

    // 12. Wi-Fi SoftAP
    viewItems[ITEM_WIFI].label = tr(Str::M_WIFI);
    setValue(ITEM_WIFI, _settings.wifiEnabled ? tr(Str::V_ON) : tr(Str::V_OFF));
    viewItems[ITEM_WIFI].isAction = true;

    // 13. Training Set
    viewItems[ITEM_TRAIN_SET].label = tr(Str::M_TRAIN_SET);
    const char* setName = "Koch";
    switch (_settings.trainingSet) {
        case TrainingSet::KOCH:      setName = "Koch"; break;
        case TrainingSet::LETTERS:   setName = "A-Z"; break;
        case TrainingSet::NUMBERS:   setName = "0-9"; break;
        case TrainingSet::ALL_CHARS: setName = tr(Str::V_SET_ALL); break;
        case TrainingSet::Q_CODES:   setName = "Q-Codes"; break;
        case TrainingSet::QSO_ABBR:  setName = tr(Str::V_SET_QSO); break;
        case TrainingSet::PROSIGNS:  setName = tr(Str::V_SET_PROSIGNS); break;
    }
    setValue(ITEM_TRAIN_SET, setName);
    viewItems[ITEM_TRAIN_SET].isAction = false;

    // 14. Koch Level
    viewItems[ITEM_KOCH_LVL].label = tr(Str::M_KOCH_LEVEL);
    snprintf(viewItems[ITEM_KOCH_LVL].value, sizeof(viewItems[ITEM_KOCH_LVL].value), tr(Str::V_LEVEL_FMT), _settings.kochLevel);
    viewItems[ITEM_KOCH_LVL].isAction = false;

    // 15. Sleep Timeout
    viewItems[ITEM_SLEEP_TIMEOUT].label = tr(Str::M_SLEEP);
    if (_settings.sleepTimeoutMin == 0) {
        setValue(ITEM_SLEEP_TIMEOUT, tr(Str::V_SLEEP_OFF));
    } else {
        snprintf(viewItems[ITEM_SLEEP_TIMEOUT].value, sizeof(viewItems[ITEM_SLEEP_TIMEOUT].value), tr(Str::V_MINUTES_FMT), _settings.sleepTimeoutMin);
    }
    viewItems[ITEM_SLEEP_TIMEOUT].isAction = false;

    // 16. Back / Exit
    viewItems[ITEM_BACK].label = tr(Str::M_BACK);
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
    I18n::setLanguage(_settings.language);
    WebManager::setLanguage(I18n::languageCode(_settings.language));

    _keyer.setMode(_settings.keyerMode);
    _keyer.setAutoSpeed(_settings.autoSpeed);
    _keyer.setPaddleSwap(_settings.paddleSwap);
    _keyer.setWpm(_settings.wpm);

    Sidetone::setFrequency(_settings.sidetoneHz);
    Sidetone::setVolume(_settings.sidetoneVol);

    // Map brightness 10..100% to PWM duty (0..255)
    uint8_t bl = (uint8_t)((_settings.brightness * 255) / 100);
    _display.setBrightness(bl);

    // Sync BLE Keyboard state & layout
    BleManager::setEnabled(_settings.bleKeyboard);
    BleManager::setLayout(_settings.bleLayout);

    // Sync Wi-Fi SoftAP state
    WebManager::setEnabled(_settings.wifiEnabled);

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
        prefs.putUChar("lang", (uint8_t)_settings.language);
        prefs.putInt("screen", (int)_settings.appScreen);
        prefs.putInt("wpm",    _settings.wpm);
        prefs.putBool("auto",  _settings.autoSpeed);
        prefs.putInt("mode",   (int)_settings.keyerMode);
        prefs.putBool("swap",  _settings.paddleSwap);
        prefs.putUShort("tone",_settings.sidetoneHz);
        prefs.putUChar("vol",  _settings.sidetoneVol);
        prefs.putUChar("brt",  _settings.brightness);
        prefs.putBool("ble",   _settings.bleKeyboard);
        prefs.putUChar("blelay", (uint8_t)_settings.bleLayout);
        prefs.putBool("wifi",  _settings.wifiEnabled);
        prefs.putInt("tset",   (int)_settings.trainingSet);
        prefs.putUChar("koch", _settings.kochLevel);
        prefs.putUChar("sleep", _settings.sleepTimeoutMin);
        prefs.end();
    }
}

void MenuController::loadSettings() {
    Preferences prefs;
    if (prefs.begin("morse", true)) {
        uint8_t lang          = prefs.getUChar("lang", (uint8_t)Language::DE);
        _settings.language    = (lang < LANGUAGE_COUNT) ? (Language)lang : Language::DE;
        _settings.appScreen   = (AppScreen)prefs.getInt("screen", (int)AppScreen::DECODE);
        _settings.wpm         = prefs.getInt("wpm",      WPM_DEFAULT);
        _settings.autoSpeed   = prefs.getBool("auto",    false);
        _settings.keyerMode   = (IambicMode)prefs.getInt("mode", (int)IambicMode::MODE_A);
        _settings.paddleSwap  = prefs.getBool("swap",    false);
        _settings.sidetoneHz  = prefs.getUShort("tone",  800);
        _settings.sidetoneVol = prefs.getUChar("vol",    100);
        _settings.brightness  = prefs.getUChar("brt",    80);
        _settings.bleKeyboard = prefs.getBool("ble",     true);
        _settings.bleLayout   = (BleLayout)prefs.getUChar("blelay", (uint8_t)BleLayout::GERMAN_DE);
        _settings.wifiEnabled = prefs.getBool("wifi",    true);
        _settings.trainingSet = (TrainingSet)prefs.getInt("tset", (int)TrainingSet::KOCH);
        _settings.kochLevel   = prefs.getUChar("koch",   4);
        _settings.sleepTimeoutMin = prefs.getUChar("sleep", SLEEP_TIMEOUT_DEFAULT_MIN);
        prefs.end();
    }
}

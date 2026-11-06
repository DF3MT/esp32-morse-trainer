#pragma once
#include <Arduino.h>
#include <AiEsp32RotaryEncoder.h>
#include "config.h"
#include "keyer.h"
#include "display.h"
#include "ble_manager.h"

// ============================================================
// MenuController — Voll konfigurierbar über Rotary Encoder
// ============================================================

enum class AppScreen {
    DECODE,
    TRAINING
};

enum class TrainingSet {
    KOCH,
    LETTERS,
    NUMBERS,
    ALL_CHARS,
    Q_CODES,
    QSO_ABBR,
    PROSIGNS
};

struct Settings {
    AppScreen   appScreen    = AppScreen::DECODE;
    int         wpm          = WPM_DEFAULT;        // 3 .. 40 WPM
    bool        autoSpeed    = false;              // Standard: AUS (Manuelles Tempo per Drehencoder)
    IambicMode  keyerMode    = IambicMode::MODE_A; // Mode A / Mode B
    bool        paddleSwap   = false;              // false = Normal, true = Getauscht
    uint16_t    sidetoneHz   = 800;                // 400 .. 2500 Hz
    uint8_t     sidetoneVol  = 100;                // 0 .. 100 % (100% = max. Leistung bei 50% PWM Duty)
    uint8_t     brightness   = 80;                 // 10 .. 100 %
    bool        bleKeyboard  = true;               // Bluetooth HID Tastatur Ein/Aus
    BleLayout   bleLayout    = BleLayout::GERMAN_DE;// Tastaturlayout: DE (QWERTZ) / US (QWERTY)
    TrainingSet trainingSet  = TrainingSet::KOCH;
    uint8_t     kochLevel    = 4;                  // 1 .. 40
};

class MenuController {
public:
    MenuController(IambicKeyer& keyer, DisplayManager& display);

    void begin();
    void tick();   // Aufruf aus dem UI-Task alle ~20ms

    AppScreen        currentScreen() const { return _settings.appScreen; }
    const Settings&  settings()      const { return _settings; }
    bool             isInMenu()      const { return _inMenu; }
    void             openMenu();
    void             closeMenu();

    // Koch sequence reference
    static const char KOCH_SEQUENCE[];

private:
    AiEsp32RotaryEncoder _encoder;
    IambicKeyer&         _keyer;
    DisplayManager&      _display;

    Settings  _settings;
    bool      _inMenu     = false;
    bool      _isEditing  = false;
    int8_t    _menuIndex  = 0;

    enum MenuItemId {
        ITEM_MODE = 0,
        ITEM_WPM,
        ITEM_AUTOSPEED,
        ITEM_KEYER_MODE,
        ITEM_PADDLE_SWAP,
        ITEM_TONE,
        ITEM_VOLUME,
        ITEM_BRIGHTNESS,
        ITEM_BLE_KEYBOARD,
        ITEM_BLE_LAYOUT,
        ITEM_TRAIN_SET,
        ITEM_KOCH_LVL,
        ITEM_BACK,
        ITEM_COUNT
    };

    void handleMainScreen();
    void handleMenuNavigation();
    void handleMenuEditing();
    void buildMenuItems(MenuItemView* viewItems);

    void applySettings(bool playFeedback = false);
    void saveSettings();
    void loadSettings();

    bool checkButtonClicked();
};

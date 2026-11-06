#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

// ============================================================
// DisplayManager — all screen rendering for the Morse Trainer
// Uses TFT_eSprite for flicker-free double-buffered drawing.
// Screen: ST7789 135x240 pixels (Landscape: 240 wide, 135 tall)
// ============================================================

enum class Screen { DECODE, TRAINING, MENU, SCORE };

struct TrainingChallenge {
    char   target[8];    // The character(s) to key
    char   sent[8];      // What the operator actually sent
    bool   correct;
    int    wpm;
    bool   autoMode;
    bool   autoReady;
};

struct MenuItemView {
    const char* label;
    char        value[24];
    bool        isAction;
};

class DisplayManager {
public:
    DisplayManager();
    void begin();

    // Full-screen Boot Splash
    void showSplashScreen();

    // Decoded text screen — shown during free keying
    void showDecodeScreen(const char* text, int wpm, bool autoMode, bool autoReady, bool keyDown, bool bleEnabled = false, bool bleConnected = false);

    // Training screen — show target and feedback
    void showTrainingScreen(const TrainingChallenge& ch, bool keyDown = false, bool bleEnabled = false, bool bleConnected = false);

    // Menu screen with values, scrolling and edit-mode highlighting
    void showMenuScreen(const MenuItemView* items, uint8_t count, uint8_t selected, bool isEditing);

    // Score screen
    void showScoreScreen(int correct, int total, int avgWpm);

    // Deep Sleep & Standby
    void showSleepScreen();
    void sleep();

    // Backlight (0–255)
    void setBrightness(uint8_t val);

private:
    TFT_eSPI    _tft;
    TFT_eSprite _spr;   // Full-screen sprite (double buffer)
    bool        _begun = false;

    void drawStatusBar(const char* label, int wpm, bool autoMode, bool autoReady, bool keyDown, bool bleEnabled = false, bool bleConnected = false);

    // Colour palette
    static const uint16_t C_BG     = TFT_BLACK;
    static const uint16_t C_FG     = TFT_WHITE;
    static const uint16_t C_ACCENT = 0x07FF;   // Cyan
    static const uint16_t C_OK     = TFT_GREEN;
    static const uint16_t C_ERR    = TFT_RED;
    static const uint16_t C_WARN   = TFT_YELLOW;
    static const uint16_t C_DIM    = 0x7BEF;   // Light grey
    static const uint16_t C_BAR    = 0x2945;   // Dark blue bar
};

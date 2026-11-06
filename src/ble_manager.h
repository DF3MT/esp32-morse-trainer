#pragma once
#include <Arduino.h>

// ============================================================
// BleManager — Bluetooth Low Energy HID Keyboard Manager
//
// Emulates a standard wireless Bluetooth keyboard recognized
// natively by Windows, macOS, Linux, Android, and iOS.
// Decoded Morse code characters are transmitted in real time.
//
// Supports German QWERTZ layout (default) and US QWERTY layout.
// On German layout, scan codes are remapped so that:
//   '/' -> Shift + 7
//   '?' -> Shift + ß (key next to 0)
//   '=' -> Shift + 0
//   '-' -> physical '-' key (0x38)
//   'Z'/'Y' are mapped to their proper German QWERTZ positions
//
// Thread safety:
// - pushChar(c) is called from Core 0 (0-wait queue push, non-blocking)
// - tick() is called from Core 1 (uiTask) to transmit via NimBLE
// ============================================================

enum class BleLayout : uint8_t {
    GERMAN_DE = 0, // QWERTZ: / -> Shift+7, ? -> Shift+ß, Z/Y swapped
    US_EN     = 1  // QWERTY: Standard US layout
};

class BleManager {
public:
    static void begin(bool enabled, BleLayout layout = BleLayout::GERMAN_DE);
    static void end();
    static void setEnabled(bool enabled);
    static bool isEnabled();
    static bool isConnected();
    static void setLayout(BleLayout layout);
    static BleLayout getLayout();
    static void pushChar(char c);
    static void pushString(const char* str);
    static void tick(); // Periodic dispatch called from Core 1 (uiTask)
    static void sendRawKey(uint8_t keycode, uint8_t modifiers);
    static void sendChar(char c);

private:
    static bool          _enabled;
    static bool          _started;
    static BleLayout     _layout;
    static QueueHandle_t _queue;
};

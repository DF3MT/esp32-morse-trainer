#pragma once
#include <Arduino.h>

// ============================================================
// BleManager — Bluetooth Low Energy HID Keyboard Manager
//
// Emulates a standard wireless Bluetooth keyboard recognized
// natively by Windows, macOS, Linux, Android, and iOS.
// Decoded Morse code characters are transmitted in real time.
//
// Thread safety:
// - pushChar(c) is called from Core 0 (0-wait queue push, non-blocking)
// - tick() is called from Core 1 (uiTask) to transmit via NimBLE
// ============================================================

class BleManager {
public:
    static void begin(bool enabled);
    static void end();
    static void setEnabled(bool enabled);
    static bool isEnabled();
    static bool isConnected();
    static void pushChar(char c);
    static void tick(); // Periodic dispatch called from Core 1 (uiTask)

private:
    static bool          _enabled;
    static bool          _started;
    static QueueHandle_t _queue;
};

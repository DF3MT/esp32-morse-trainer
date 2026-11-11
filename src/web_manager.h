#pragma once
#include <Arduino.h>
#include <IPAddress.h>
#include "keyer.h"

class WebManager {
public:
    // Initialize Wi-Fi SoftAP ("DF3MT-MorseTrainer"), WebServer (80), and WebSockets (81)
    static void begin(bool enabled = true, const char* ssid = "DF3MT-MorseTrainer", const char* password = nullptr);

    // Turn Wi-Fi AP & web services on or off
    static void setEnabled(bool enabled);
    static bool isEnabled();

    // Handle incoming HTTP and WebSocket packets; process queued events.
    // MUST be called from Core 1 (uiTask) to avoid interrupting Core 0 1ms keyer loop.
    static void tick();

    // Push decoded token (thread-safe, non-blocking FreeRTOS queue)
    static void pushToken(const char* token, int wpm);

    // Push raw keyer element (thread-safe, non-blocking FreeRTOS queue)
    static void pushElement(KeyerElement elem);

    // Push WPM update
    static void pushWpm(int wpm);

    // Set UI language of the web terminal (ISO 639-1 code, e.g. "de", "en").
    // Connected browsers switch immediately.
    static void setLanguage(const char* code);

    // Clear history text buffer
    static void clearHistory();

    // Status queries
    static bool isApActive();
    static uint8_t getClientCount();
    static IPAddress getIP();
};

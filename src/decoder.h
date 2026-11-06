#pragma once
#include <Arduino.h>
#include "keyer.h"

// ============================================================
// MorseDecoder — converts keyer elements into characters & prosigns
//
// Feed elements via push(). Tokens (e.g. "A", ".", "<SK>", "<AR>")
// are emitted via callback when inter-character silence is detected.
// Spaces are emitted when inter-word silence is detected.
// ============================================================
class MorseDecoder {
public:
    using TokenCallback = void (*)(const char* token);
    using CharCallback  = void (*)(char c);

    MorseDecoder();

    // Feed an element (call from keyer element callback)
    void push(KeyerElement elem);

    // Call every 1ms so inter-character/word gaps can be detected
    void tick(float keyerT_ms, bool isKeyDown = false);

    void onToken(TokenCallback cb) { _tokenCb = cb; }
    void onChar(CharCallback cb)   { _charCb = cb; }
    void reset();

    int  getDecodedWpm() const;

private:
    struct MorseMapping {
        const char* pattern;
        const char* token;
    };

    static const MorseMapping MORSE_TABLE[];
    static const size_t MORSE_TABLE_SIZE;

    char          _pattern[16];
    uint8_t       _patLen           = 0;
    unsigned long _lastSilenceStart = 0;   // millis() when tone stopped
    bool          _active           = false;
    bool          _spacePending     = false;

    TokenCallback _tokenCb          = nullptr;
    CharCallback  _charCb           = nullptr;

    float         _decodedT         = 60.0f;

    void emitToken();
    const char* findToken(const char* pattern) const;
};

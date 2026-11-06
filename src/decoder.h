#pragma once
#include <Arduino.h>
#include "keyer.h"

// ============================================================
// MorseDecoder — converts keyer elements into characters
//
// Feed elements via push(). Characters are emitted via callback
// when a 3T inter-character silence is detected.
// Spaces are emitted when a 7T inter-word silence is detected.
// ============================================================
class MorseDecoder {
public:
    using CharCallback = void (*)(char c);

    MorseDecoder();

    // Feed an element (call from keyer element callback)
    void push(KeyerElement elem);

    // Call every 1ms so inter-character/word gaps can be detected
    void tick(float keyerT_ms, bool isKeyDown = false);

    void onChar(CharCallback cb) { _charCb = cb; }
    void reset();

    int  getDecodedWpm() const;

private:
    static const char MORSE_TREE[128];

    uint8_t       _treeIdx          = 1;   // Current position in tree
    unsigned long _lastSilenceStart = 0;   // millis() when tone stopped
    bool          _active           = false;
    bool          _spacePending     = false;

    CharCallback _charCb = nullptr;

    float  _decodedT = 60.0f;

    void emitChar();
};

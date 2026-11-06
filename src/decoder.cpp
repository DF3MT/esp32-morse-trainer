#include "decoder.h"

// ============================================================
// Morse binary tree (flat array, index 1 = root)
// dit → 2*i, dah → 2*i+1
// Extended to 256 elements to cover A-Z, 0-9, and all standard
// punctuation and prosigns: . , ? / = - : ; ( ) @ +
// ============================================================
const char MorseDecoder::MORSE_TREE[256] = {
/*  0*/   0,   0, 'E', 'T', 'I', 'A', 'N', 'M',
/*  8*/ 'S', 'U', 'R', 'W', 'D', 'K', 'G', 'O',
/* 16*/ 'H', 'V', 'F',   0, 'L',   0, 'P', 'J',
/* 24*/ 'B', 'X', 'C', 'Y', 'Z', 'Q',   0,   0,
/* 32*/ '5', '4',   0, '3',   0,   0,   0, '2',
/* 40*/   0,   0, '+',   0,   0,   0,   0, '1',
/* 48*/ '6', '=', '/',   0,   0,   0, '(',   0,
/* 56*/ '7',   0,   0,   0, '8',   0, '9', '0',
/* 64*/   0,   0,   0,   0,   0,   0,   0,   0,
/* 72*/   0,   0,   0,   0, '?',   0,   0,   0,
/* 80*/   0,   0,   0,   0,   0, '.',   0,   0,
/* 88*/   0,   0, '@',   0,   0,   0,   0,   0,
/* 96*/   0, '-',   0,   0,   0,   0,   0,   0,
/*104*/   0,   0, ';',   0,   0, ')',   0,   0,
/*112*/   0,   0,   0, ',',   0,   0,   0,   0,
/*120*/ ':',   0,   0,   0,   0,   0,   0,   0,
/*128*/   0,   0,   0,   0,   0,   0,   0,   0,
/*136*/   0,   0,   0,   0,   0,   0,   0,   0,
/*144*/   0,   0,   0,   0,   0,   0,   0,   0,
/*152*/   0,   0,   0,   0,   0,   0,   0,   0,
/*160*/   0,   0,   0,   0,   0,   0,   0,   0,
/*168*/   0,   0,   0,   0,   0,   0,   0,   0,
/*176*/   0,   0,   0,   0,   0,   0,   0,   0,
/*184*/   0,   0,   0,   0,   0,   0,   0,   0,
/*192*/   0,   0,   0,   0,   0,   0,   0,   0,
/*200*/   0,   0,   0,   0,   0,   0,   0,   0,
/*208*/   0,   0,   0,   0,   0,   0,   0,   0,
/*216*/   0,   0,   0,   0,   0,   0,   0,   0,
/*224*/   0,   0,   0,   0,   0,   0,   0,   0,
/*232*/   0,   0,   0,   0,   0,   0,   0,   0,
/*240*/   0,   0,   0,   0,   0,   0,   0,   0,
/*248*/   0,   0,   0,   0,   0,   0,   0,   0
};

MorseDecoder::MorseDecoder() {
    _lastSilenceStart = millis();
}

// ============================================================
// Feed an element from the keyer
// ============================================================
void MorseDecoder::push(KeyerElement elem) {
    if (elem == KeyerElement::NONE) return;

    // A new element belongs to a character -> cancel pending word space
    _spacePending = false;

    // Traverse tree
    if (elem == KeyerElement::DIT) {
        _treeIdx = _treeIdx * 2;
    } else {
        _treeIdx = _treeIdx * 2 + 1;
    }

    if (_treeIdx >= 256) _treeIdx = 1;  // Overflow guard

    _lastSilenceStart = millis();
    _active = true;
}

// ============================================================
// Poll for inter-character / inter-word gaps
// Call every 1ms with current keyer T and key status
// ============================================================
void MorseDecoder::tick(float keyerT_ms, bool isKeyDown) {
    // If tone is actively sounding, silence is ZERO!
    if (isKeyDown) {
        _lastSilenceStart = millis();
        return;
    }

    unsigned long silence = millis() - _lastSilenceStart;

    // 1. Inter-character gap: 2.5T silence after element ends -> emit decoded character
    // In Morse, element space is 1T. An inter-character space is 3T.
    // The decision threshold is the midpoint (2.5T).
    if (_active && silence >= (unsigned long)(keyerT_ms * 2.5f)) {
        emitChar();
        _active = false;
        _spacePending = true;
    }

    // 2. Inter-word gap: 5.0T silence -> emit word space
    // Theoretical Morse word space is 7T; real operators pause 4.5T to 6T.
    // The decision threshold between letter gap (3T) and word gap (7T) is the midpoint (5.0T).
    if (_spacePending && silence >= (unsigned long)(keyerT_ms * 5.0f)) {
        _spacePending = false;
        if (_charCb) _charCb(' ');
    }
}

// ============================================================
// Emit the character at the current tree position
// ============================================================
void MorseDecoder::emitChar() {
    char c = (_treeIdx < 256) ? MORSE_TREE[_treeIdx] : 0;
    if (c && _charCb) {
        _charCb(c);
    }
    _treeIdx = 1;  // Reset to root
}

void MorseDecoder::reset() {
    _treeIdx = 1;
    _active = false;
    _spacePending = false;
    _lastSilenceStart = millis();
}

int MorseDecoder::getDecodedWpm() const {
    return (int)(1200.0f / _decodedT + 0.5f);
}

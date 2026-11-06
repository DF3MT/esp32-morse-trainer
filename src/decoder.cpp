#include "decoder.h"

// ============================================================
// Morse code lookup table
// Maps dit/dah patterns to tokens (letters, numbers, punctuation, prosigns)
// Includes squeeze-release timing tolerance aliases for reliable decoding.
// ============================================================
const MorseDecoder::MorseMapping MorseDecoder::MORSE_TABLE[] = {
    // Letters (A-Z)
    { ".-", "A" }, { "-...", "B" }, { "-.-.", "C" }, { "-..", "D" }, { ".", "E" },
    { "..-.", "F" }, { "--.", "G" }, { "....", "H" }, { "..", "I" }, { ".---", "J" },
    { "-.-", "K" }, { ".-..", "L" }, { "--", "M" }, { "-.", "N" }, { "---", "O" },
    { ".--.", "P" }, { "--.-", "Q" }, { ".-.", "R" }, { "...", "S" }, { "-", "T" },
    { "..-", "U" }, { "...-", "V" }, { ".--", "W" }, { "-..-", "X" }, { "-.--", "Y" },
    { "--..", "Z" },

    // Digits (0-9)
    { "-----", "0" }, { ".----", "1" }, { "..---", "2" }, { "...--", "3" },
    { "....-", "4" }, { ".....", "5" }, { "-....", "6" }, { "--...", "7" },
    { "---..", "8" }, { "----.", "9" },

    // Punctuation & Special Signs (ITU-R & Amateur Radio)
    { ".-.-.-", "." },   // Period / Punkt
    { ".-.-.-.", "." },  // Period squeeze tolerance (trailing dit)
    { ".-.-.--", "." },  // Period squeeze tolerance (trailing dah)
    { "--..--", "," },   // Comma / Beistrich
    { "--..--.", "," },  // Comma squeeze tolerance (trailing dit)
    { "--..---", "," },  // Comma squeeze tolerance (trailing dah)
    { "..--..", "?" },   // Question mark / Fragezeichen
    { "..--...", "?" },  // Question mark squeeze tolerance
    { "..--..-", "?" },
    { "-..-.", "/" },    // Slash / Bruchstrich
    { "-..--.", "/" },   // Slash tolerance
    { "-....-", "-" },   // Hyphen / Bindestrich
    { "-....-.", "-" },  // Hyphen tolerance
    { "---...", ":" },   // Colon / Doppelpunkt
    { "-.-.-.", ";" },   // Semicolon / Strichpunkt
    { ".----.", "'" },   // Single quote / Apostroph
    { ".-..-.", "\"" },  // Double quote / Anfuehrungszeichen
    { ".--.-.", "@" },   // At-sign / Klammeraffe
    { ".--.-..", "@" },  // At-sign tolerance
    { ".--.-.-", "@" },
    { "-.--.-", ")" },   // Close parenthesis / Klammer
    { "..--.-", "_" },   // Underscore / Unterstrich

    // Amateur Radio Prosigns (Verkehrsabkürzungen ohne Pause - qrp4fun.de)
    { "...-.-", "<SK>" },   // QSO-Ende / Schlusszeichen (End of contact / SK)
    { "...-.--", "<SK>" },  // SK squeeze tolerance
    { "...-.-.", "<SK>" },  // SK squeeze tolerance
    { ".-.-.", "<AR>" },    // Spruchende / Ende der Sendung / Kreuz (End of message / AR / +)
    { ".-.-..", "<AR>" },   // AR squeeze tolerance
    { "-...-", "<BT>" },    // Trennung / Pause (Break / BT / =)
    { "-...-.", "<BT>" },   // BT squeeze tolerance
    { ".-...", "<AS>" },    // Bitte warten (Wait / AS)
    { ".-....", "<AS>" },   // AS squeeze tolerance
    { "-.--.", "<KN>" },    // Aufforderung nur an bestimmte Station (Go only / KN / ()
    { "-.--..", "<KN>" },   // KN squeeze tolerance
    { "-.-.-", "<KA>" },    // Spruchanfang / Beginn des Verkehrs (Starting signal / KA)
    { "-...-.-", "<BK>" },  // Break / Zwischenruf (Break in / BK)
    { "-...-.--", "<BK>" }, // BK squeeze tolerance
    { "...-.", "<VE>" },    // Verstanden / Bestaetigt (Understood / VE / SN)
    { "........", "<HH>" }, // Irrung / Fehler (Correction / Error - 8 dits)
    { ".......", "<HH>" },  // HH tolerance (7 dits)
    { ".........", "<HH>" },// HH tolerance (9 dits)
    { "...---...", "<SOS>" }// Notruf (Distress / SOS)
};

const size_t MorseDecoder::MORSE_TABLE_SIZE = sizeof(MORSE_TABLE) / sizeof(MORSE_TABLE[0]);

MorseDecoder::MorseDecoder() {
    _pattern[0] = '\0';
    _patLen = 0;
    _lastSilenceStart = millis();
    _charSpaceMult  = 4.2f;
    _wordSpaceMult  = 8.0f;
    _minCharSpaceMs = 280;
    _minWordSpaceMs = 650;
}

void MorseDecoder::setPauseTolerance(float charMult, float wordMult, unsigned long minCharMs, unsigned long minWordMs) {
    _charSpaceMult  = charMult;
    _wordSpaceMult  = wordMult;
    _minCharSpaceMs = minCharMs;
    _minWordSpaceMs = minWordMs;
    Serial.printf("[DEC] Pause config: charMult=%.1f (min %lu ms), wordMult=%.1f (min %lu ms)\n",
                  charMult, minCharMs, wordMult, minWordMs);
}

void MorseDecoder::push(KeyerElement elem) {
    if (elem == KeyerElement::NONE) return;

    // A new element belongs to a character -> cancel pending word space
    _spacePending = false;

    if (_patLen < sizeof(_pattern) - 1) {
        _pattern[_patLen++] = (elem == KeyerElement::DIT) ? '.' : '-';
        _pattern[_patLen]   = '\0';
    }

    Serial.printf("[DEC] elem=%s -> pat='%s'\n", (elem == KeyerElement::DIT) ? "." : "-", _pattern);

    _lastSilenceStart = millis();
    _active = true;
}

void MorseDecoder::tick(float keyerT_ms, bool isKeyDown) {
    // If tone is actively sounding, silence is ZERO!
    if (isKeyDown) {
        _lastSilenceStart = millis();
        return;
    }

    unsigned long silence = millis() - _lastSilenceStart;

    unsigned long charThreshold = (unsigned long)(keyerT_ms * _charSpaceMult);
    if (charThreshold < _minCharSpaceMs) charThreshold = _minCharSpaceMs;

    unsigned long wordThreshold = (unsigned long)(keyerT_ms * _wordSpaceMult);
    if (wordThreshold < _minWordSpaceMs) wordThreshold = _minWordSpaceMs;

    // 1. Inter-character gap: emit decoded token when letter pause reached
    if (_active && silence >= charThreshold) {
        emitToken();
        _active = false;
        _spacePending = true;
    }

    // 2. Inter-word gap: emit word space when word pause reached
    if (_spacePending && silence >= wordThreshold) {
        _spacePending = false;
        if (_tokenCb) _tokenCb(" ");
        if (_charCb)  _charCb(' ');
    }
}

const char* MorseDecoder::findToken(const char* pattern) const {
    if (!pattern || !pattern[0]) return nullptr;
    for (size_t i = 0; i < MORSE_TABLE_SIZE; i++) {
        if (strcmp(MORSE_TABLE[i].pattern, pattern) == 0) {
            return MORSE_TABLE[i].token;
        }
    }
    return nullptr;
}

void MorseDecoder::emitToken() {
    if (_patLen == 0) return;

    const char* token = findToken(_pattern);
    Serial.printf("[DEC] emit pat='%s' -> token='%s'\n", _pattern, token ? token : "<UNKNOWN>");

    if (token) {
        if (_tokenCb) {
            _tokenCb(token);
        }
        if (_charCb && strlen(token) == 1) {
            _charCb(token[0]);
        }
    }

    _patLen = 0;
    _pattern[0] = '\0';
}

void MorseDecoder::reset() {
    _patLen = 0;
    _pattern[0] = '\0';
    _active = false;
    _spacePending = false;
    _lastSilenceStart = millis();
}

int MorseDecoder::getDecodedWpm() const {
    return (int)(1200.0f / _decodedT + 0.5f);
}

#include "keyer.h"
#include "config.h"
#include "sidetone.h"

IambicKeyer::IambicKeyer(IambicMode mode, int wpm)
    : _mode(mode)
{
    _activeT  = tFromWpm(wpm);
    _pendingT = _activeT;
    _stateStart = millis();
}

void IambicKeyer::setWpm(int wpm) {
    wpm = constrain(wpm, WPM_MIN, WPM_MAX);
    _pendingT = tFromWpm(wpm);
    _activeT  = _pendingT;
}

void IambicKeyer::setMode(IambicMode mode) { _mode = mode; }
void IambicKeyer::setAutoSpeed(bool en)    { _autoSpeed = en; }
int   IambicKeyer::getWpm()         const { return wpmFromT(_activeT); }
float IambicKeyer::getT()           const { return _activeT; }
bool  IambicKeyer::isAutoSpeed()    const { return _autoSpeed; }
bool  IambicKeyer::isAutoSpeedReady() const { return _autoReady; }
IambicMode IambicKeyer::getMode()   const { return _mode; }
bool  IambicKeyer::isKeyDown()      const { return _keyDown; }

// ============================================================
// Debounced paddle reading (10ms filter against mechanical contact bounce)
// ============================================================
bool IambicKeyer::readDit() {
    uint8_t pin = _paddleSwap ? PIN_DAH : PIN_DIT;
    bool raw = (digitalRead(pin) == LOW);
    if (raw != _ditStable) {
        if (millis() - _ditLastChange >= 10) {
            _ditStable = raw;
            _ditLastChange = millis();
        }
    } else {
        _ditLastChange = millis();
    }
    return _ditStable;
}

bool IambicKeyer::readDah() {
    uint8_t pin = _paddleSwap ? PIN_DIT : PIN_DAH;
    bool raw = (digitalRead(pin) == LOW);
    if (raw != _dahStable) {
        if (millis() - _dahLastChange >= 10) {
            _dahStable = raw;
            _dahLastChange = millis();
        }
    } else {
        _dahLastChange = millis();
    }
    return _dahStable;
}

// ============================================================
// Main update — call every 1ms
// ============================================================
void IambicKeyer::update() {
    bool dit = readDit();
    bool dah = readDah();

    // Squeeze detection: true if both paddles are simultaneously pressed
    if (dit && dah) {
        _squeezeOccurred = true;
    }

    // Latch memory for the OPPOSITE paddle while active tone is sounding
    // (Curtis standard: only latch during mark, not during space)
    if (_state == State::SENDING_DIT) {
        if (dah) _dahMem = true;
    } else if (_state == State::SENDING_DAH) {
        if (dit) _ditMem = true;
    }

    unsigned long elapsed = millis() - _stateStart;

    switch (_state) {
        case State::IDLE:
            if (dit && dah) {
                // Simultaneous squeeze from IDLE: Dit priority (Curtis standard)
                _ditMem = false;
                _dahMem = false;
                startDit();
            } else if (dit) {
                _ditMem = false;
                _dahMem = false;
                startDit();
            } else if (dah) {
                _ditMem = false;
                _dahMem = false;
                startDah();
            }
            break;

        case State::SENDING_DIT:
            if (elapsed >= (unsigned long)_activeT) {
                keyOff();
                _stateStart = millis();
                _state = State::DIT_SPACE;
            }
            break;

        case State::DIT_SPACE:
            if (elapsed >= (unsigned long)_activeT) {
                decideNext();
            }
            break;

        case State::SENDING_DAH:
            if (elapsed >= (unsigned long)(_activeT * 3.0f)) {
                keyOff();
                _stateStart = millis();
                _state = State::DAH_SPACE;
            }
            break;

        case State::DAH_SPACE:
            if (elapsed >= (unsigned long)_activeT) {
                decideNext();
            }
            break;
    }
}

// ============================================================
// Element decision after inter-element gap (1T silence)
// Follows strict Curtis 8044 Mode A / WB4VVF Mode B standards:
// - Mode A: when both paddles are released, finish current element and STOP.
// - Mode B: when both paddles are released, add ONE alternating element.
// ============================================================
void IambicKeyer::decideNext() {
    bool liveDit = readDit();
    bool liveDah = readDah();

    // 1. Both paddles currently pressed (active squeeze continuing)
    if (liveDit && liveDah) {
        if (_lastWasDah) {
            startDit();
        } else {
            startDah();
        }
        return;
    }

    // 2. Only Dit paddle currently pressed
    if (liveDit && !liveDah) {
        if (_lastWasDah) {
            // Alternation from Dah to Dit
            startDit();
        } else if (_dahMem) {
            // Operator tapped Dah while holding Dit (e.g. 'R' . - .)
            _dahMem = false;
            startDah();
        } else {
            // Continuous Dit hold (e.g. . . . .)
            startDit();
        }
        return;
    }

    // 3. Only Dah paddle currently pressed
    if (liveDah && !liveDit) {
        if (!_lastWasDah) {
            // Alternation from Dit to Dah
            startDah();
        } else if (_ditMem) {
            // Operator tapped Dit while holding Dah (e.g. 'K' - . -)
            _ditMem = false;
            startDit();
        } else {
            // Continuous Dah hold (e.g. - - - -)
            startDah();
        }
        return;
    }

    // 4. Neither paddle currently pressed (both released)
    if (_squeezeOccurred) {
        // Both paddles were squeezed during the element and then released.
        _squeezeOccurred = false;
        _ditMem = false;
        _dahMem = false;

        if (_mode == IambicMode::MODE_B) {
            // Mode B (Accu-Keyer): insert one final opposite element before stopping
            if (_lastWasDah) {
                startDit();
            } else {
                startDah();
            }
        } else {
            // Mode A (Curtis 8044): complete current element and stop dead!
            _state = State::IDLE;
        }
        return;
    }

    // 5. Neither paddle currently pressed, and not a squeeze:
    // Single-paddle memory (dot/dash memory for sequential keying like 'A' or 'N')
    if (!_lastWasDah && _dahMem) {
        _dahMem = false;
        startDah();
        return;
    }
    if (_lastWasDah && _ditMem) {
        _ditMem = false;
        startDit();
        return;
    }

    // 6. No paddles pressed, no memories -> IDLE
    _ditMem = false;
    _dahMem = false;
    _state = State::IDLE;
}

void IambicKeyer::startDit() {
    keyOn();
    _stateStart      = millis();
    _state           = State::SENDING_DIT;
    _lastWasDah      = false;
    _ditMem          = false;
    _squeezeOccurred = (readDit() && readDah());
    if (_elemCb) _elemCb(KeyerElement::DIT);
}

void IambicKeyer::startDah() {
    keyOn();
    _stateStart      = millis();
    _state           = State::SENDING_DAH;
    _lastWasDah      = true;
    _dahMem          = false;
    _squeezeOccurred = (readDit() && readDah());
    if (_elemCb) _elemCb(KeyerElement::DAH);
}

void IambicKeyer::keyOn() {
    _keyDown = true;
    Sidetone::on();
    if (_keyCb) _keyCb(true);
}

void IambicKeyer::keyOff() {
    _keyDown = false;
    Sidetone::off();
    if (_keyCb) _keyCb(false);
}

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
// Debounced paddle reading (10ms filter against contact bounce)
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

    // Track whether paddles were released during this element
    // A tap will release the paddle before the space finishes;
    // a continuous hold keeps the paddle pressed throughout.
    if (!dit) _ditReleased = true;
    if (!dah) _dahReleased = true;

    // Latch memory for the OPPOSITE paddle while an element is in progress
    if (_state == State::SENDING_DIT || _state == State::DIT_SPACE) {
        if (dah) _dahMem = true;
    } else if (_state == State::SENDING_DAH || _state == State::DAH_SPACE) {
        if (dit) _ditMem = true;
    }

    unsigned long elapsed = millis() - _stateStart;

    switch (_state) {
        case State::IDLE:
            if (dit) {
                _ditMem = false;
                _dahMem = false;
                _modeB_extraFired = false;
                startDit();
            } else if (dah) {
                _ditMem = false;
                _dahMem = false;
                _modeB_extraFired = false;
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
                _lastWasDah = false;
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
                _lastWasDah = true;
                decideNext();
            }
            break;
    }
}

// ============================================================
// Element decision after inter-element gap (1T silence)
// ============================================================
void IambicKeyer::decideNext() {
    bool liveDit = readDit();
    bool liveDah = readDah();

    // Squeeze detection: both paddles active or latched
    bool squeeze = (liveDit && liveDah) || (_ditMem && _dahMem);

    if (squeeze) {
        // Alternate from last element
        if (_lastWasDah) {
            _ditMem = false;
            startDit();
        } else {
            _dahMem = false;
            startDah();
        }
        _modeB_extraFired = false;
    } else if (_lastWasDah) {
        // Previous element was DAH
        if (_ditMem) {
            _ditMem = false;
            startDit();
            _modeB_extraFired = false;
        } else if (liveDit) {
            startDit();
            _modeB_extraFired = false;
        } else if (liveDah && !_dahReleased) {
            // Operator is CONTINUOUSLY HOLDING Dah paddle -> repeat Dah
            startDah();
            _modeB_extraFired = false;
        } else if (_mode == IambicMode::MODE_B && _dahMem && !_modeB_extraFired) {
            _modeB_extraFired = true;
            _dahMem = false;
            startDit();
        } else {
            // Released -> return to IDLE
            _ditMem = false;
            _dahMem = false;
            _modeB_extraFired = false;
            _state = State::IDLE;
        }
    } else {
        // Previous element was DIT
        if (_dahMem) {
            _dahMem = false;
            startDah();
            _modeB_extraFired = false;
        } else if (liveDah) {
            startDah();
            _modeB_extraFired = false;
        } else if (liveDit && !_ditReleased) {
            // Operator is CONTINUOUSLY HOLDING Dit paddle -> repeat Dit
            startDit();
            _modeB_extraFired = false;
        } else if (_mode == IambicMode::MODE_B && _ditMem && !_modeB_extraFired) {
            _modeB_extraFired = true;
            _ditMem = false;
            startDah();
        } else {
            // Released -> return to IDLE
            _ditMem = false;
            _dahMem = false;
            _modeB_extraFired = false;
            _state = State::IDLE;
        }
    }
}

void IambicKeyer::startDit() {
    keyOn();
    _stateStart  = millis();
    _state       = State::SENDING_DIT;
    _lastWasDah  = false;
    _ditReleased = false; // New dit started: reset release detector
    if (_elemCb) _elemCb(KeyerElement::DIT);
}

void IambicKeyer::startDah() {
    keyOn();
    _stateStart  = millis();
    _state       = State::SENDING_DAH;
    _lastWasDah  = true;
    _dahReleased = false; // New dah started: reset release detector
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

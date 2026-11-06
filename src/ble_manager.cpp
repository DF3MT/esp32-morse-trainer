#include "ble_manager.h"
#include <BleKeyboard.h>

bool          BleManager::_enabled = false;
bool          BleManager::_started = false;
BleLayout     BleManager::_layout  = BleLayout::GERMAN_DE;
QueueHandle_t BleManager::_queue   = nullptr;

// BleKeyboard instance: Device Name, Manufacturer Name, Battery percentage
static BleKeyboard bleKeyboard("CW-Keyer DF3MT", "DF3MT", 100);

void BleManager::begin(bool enabled, BleLayout layout) {
    _layout = layout;
    if (!_queue) {
        _queue = xQueueCreate(128, sizeof(char));
    }
    _enabled = enabled;
    if (_enabled && !_started) {
        bleKeyboard.begin();
        _started = true;
        Serial.printf("[BLE] HID Keyboard started (Layout: %s). Advertising as 'CW-Keyer DF3MT'...\n",
                      _layout == BleLayout::GERMAN_DE ? "DE/QWERTZ" : "US/QWERTY");
    }
}

void BleManager::end() {
    if (_started) {
        bleKeyboard.end();
        _started = false;
        Serial.println("[BLE] HID Keyboard stopped.");
    }
}

void BleManager::setEnabled(bool enabled) {
    if (_enabled == enabled) return;
    _enabled = enabled;
    if (_enabled && !_started) {
        bleKeyboard.begin();
        _started = true;
        Serial.println("[BLE] HID Keyboard enabled and started.");
    } else if (!_enabled && _started) {
        bleKeyboard.end();
        _started = false;
        Serial.println("[BLE] HID Keyboard disabled.");
    }
}

bool BleManager::isEnabled() {
    return _enabled;
}

bool BleManager::isConnected() {
    return _enabled && _started && bleKeyboard.isConnected();
}

void BleManager::setLayout(BleLayout layout) {
    if (_layout != layout) {
        _layout = layout;
        Serial.printf("[BLE] Layout changed to %s\n",
                      _layout == BleLayout::GERMAN_DE ? "DE/QWERTZ" : "US/QWERTY");
    }
}

BleLayout BleManager::getLayout() {
    return _layout;
}

void BleManager::pushChar(char c) {
    if (!_enabled || !_started || !_queue) return;
    // 0-wait push to never block the high-priority keyer/decoder loop on Core 0
    xQueueSend(_queue, &c, 0);
}

void BleManager::sendRawKey(uint8_t keycode, uint8_t modifiers) {
    if (!bleKeyboard.isConnected()) return;
    KeyReport report = {0};
    report.modifiers = modifiers;
    report.keys[0] = keycode;
    bleKeyboard.sendReport(&report);
    bleKeyboard.releaseAll();
}

void BleManager::sendChar(char c) {
    if (!bleKeyboard.isConnected()) return;

    uint8_t keycode = 0;
    uint8_t modifiers = 0;

    if (_layout == BleLayout::GERMAN_DE) {
        // German QWERTZ layout mapping
        if (c >= 'A' && c <= 'Z') {
            modifiers = 0x02; // Shift for uppercase
            if (c == 'Y') {
                keycode = 0x1d; // US 'Z' position -> produces 'Y' on German host
            } else if (c == 'Z') {
                keycode = 0x1c; // US 'Y' position -> produces 'Z' on German host
            } else {
                keycode = (c - 'A') + 0x04;
            }
        } else if (c >= 'a' && c <= 'z') {
            modifiers = 0x00;
            if (c == 'y') {
                keycode = 0x1d;
            } else if (c == 'z') {
                keycode = 0x1c;
            } else {
                keycode = (c - 'a') + 0x04;
            }
        } else if (c >= '1' && c <= '9') {
            keycode = (c - '1') + 0x1e;
            modifiers = 0x00;
        } else if (c == '0') {
            keycode = 0x27;
            modifiers = 0x00;
        } else {
            switch (c) {
                case ' ':
                    keycode = 0x2c;
                    modifiers = 0x00;
                    break;
                case '\n':
                case '\r':
                    keycode = 0x28; // Enter
                    modifiers = 0x00;
                    break;
                case '/':
                    // German QWERTZ: Shift + 7
                    keycode = 0x24;
                    modifiers = 0x02;
                    break;
                case '?':
                    // German QWERTZ: Shift + ß (key to the right of '0')
                    keycode = 0x2d;
                    modifiers = 0x02;
                    break;
                case '.':
                    keycode = 0x37;
                    modifiers = 0x00;
                    break;
                case ',':
                    keycode = 0x36;
                    modifiers = 0x00;
                    break;
                case '=':
                    // German QWERTZ: Shift + 0
                    keycode = 0x27;
                    modifiers = 0x02;
                    break;
                case '-':
                    // German QWERTZ: Key 0x38 (between '.' and right shift)
                    keycode = 0x38;
                    modifiers = 0x00;
                    break;
                case '_':
                    // German QWERTZ: Shift + 0x38
                    keycode = 0x38;
                    modifiers = 0x02;
                    break;
                case ':':
                    // German QWERTZ: Shift + . (0x37)
                    keycode = 0x37;
                    modifiers = 0x02;
                    break;
                case ';':
                    // German QWERTZ: Shift + , (0x36)
                    keycode = 0x36;
                    modifiers = 0x02;
                    break;
                case '+':
                    // German QWERTZ: Key 0x30
                    keycode = 0x30;
                    modifiers = 0x00;
                    break;
                case '(':
                    // German QWERTZ: Shift + 8
                    keycode = 0x25;
                    modifiers = 0x02;
                    break;
                case ')':
                    // German QWERTZ: Shift + 9
                    keycode = 0x26;
                    modifiers = 0x02;
                    break;
                case '\b':
                    keycode = 0x2a; // Backspace
                    modifiers = 0x00;
                    break;
                default:
                    bleKeyboard.print(c);
                    return;
            }
        }
    } else {
        // US QWERTY layout mapping
        if (c >= 'A' && c <= 'Z') {
            modifiers = 0x02; // Shift for uppercase
            keycode = (c - 'A') + 0x04;
        } else if (c >= 'a' && c <= 'z') {
            modifiers = 0x00;
            keycode = (c - 'a') + 0x04;
        } else if (c >= '1' && c <= '9') {
            keycode = (c - '1') + 0x1e;
            modifiers = 0x00;
        } else if (c == '0') {
            keycode = 0x27;
            modifiers = 0x00;
        } else {
            switch (c) {
                case ' ':
                    keycode = 0x2c;
                    modifiers = 0x00;
                    break;
                case '\n':
                case '\r':
                    keycode = 0x28; // Enter
                    modifiers = 0x00;
                    break;
                case '/':
                    // US QWERTY: Key 0x38 without Shift
                    keycode = 0x38;
                    modifiers = 0x00;
                    break;
                case '?':
                    // US QWERTY: Key 0x38 with Shift
                    keycode = 0x38;
                    modifiers = 0x02;
                    break;
                case '.':
                    keycode = 0x37;
                    modifiers = 0x00;
                    break;
                case ',':
                    keycode = 0x36;
                    modifiers = 0x00;
                    break;
                case '=':
                    // US QWERTY: Key 0x2e without Shift
                    keycode = 0x2e;
                    modifiers = 0x00;
                    break;
                case '-':
                    // US QWERTY: Key 0x2d without Shift
                    keycode = 0x2d;
                    modifiers = 0x00;
                    break;
                case '_':
                    // US QWERTY: Key 0x2d with Shift
                    keycode = 0x2d;
                    modifiers = 0x02;
                    break;
                case ':':
                    // US QWERTY: Key 0x33 with Shift
                    keycode = 0x33;
                    modifiers = 0x02;
                    break;
                case ';':
                    // US QWERTY: Key 0x33 without Shift
                    keycode = 0x33;
                    modifiers = 0x00;
                    break;
                case '+':
                    // US QWERTY: Key 0x2e with Shift
                    keycode = 0x2e;
                    modifiers = 0x02;
                    break;
                case '(':
                    // US QWERTY: Shift + 9
                    keycode = 0x26;
                    modifiers = 0x02;
                    break;
                case ')':
                    // US QWERTY: Shift + 0
                    keycode = 0x27;
                    modifiers = 0x02;
                    break;
                case '\b':
                    keycode = 0x2a; // Backspace
                    modifiers = 0x00;
                    break;
                default:
                    bleKeyboard.print(c);
                    return;
            }
        }
    }

    sendRawKey(keycode, modifiers);
}

void BleManager::tick() {
    if (!_enabled || !_started || !_queue) return;

    char c;
    while (xQueueReceive(_queue, &c, 0) == pdTRUE) {
        if (bleKeyboard.isConnected()) {
            sendChar(c);
        }
    }
}

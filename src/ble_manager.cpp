#include "ble_manager.h"
#include <BleKeyboard.h>

bool          BleManager::_enabled = false;
bool          BleManager::_started = false;
QueueHandle_t BleManager::_queue   = nullptr;

// BleKeyboard instance: Device Name, Manufacturer Name, Battery percentage
static BleKeyboard bleKeyboard("CW-Keyer DF3MT", "DF3MT", 100);

void BleManager::begin(bool enabled) {
    if (!_queue) {
        _queue = xQueueCreate(128, sizeof(char));
    }
    _enabled = enabled;
    if (_enabled && !_started) {
        bleKeyboard.begin();
        _started = true;
        Serial.println("[BLE] HID Keyboard started. Advertising as 'CW-Keyer DF3MT'...");
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

void BleManager::pushChar(char c) {
    if (!_enabled || !_started || !_queue) return;
    // 0-wait push to never block the high-priority keyer/decoder loop on Core 0
    xQueueSend(_queue, &c, 0);
}

void BleManager::tick() {
    if (!_enabled || !_started || !_queue) return;

    char c;
    while (xQueueReceive(_queue, &c, 0) == pdTRUE) {
        if (bleKeyboard.isConnected()) {
            if (c == '\n') {
                bleKeyboard.write(KEY_RETURN);
            } else if (c == ' ') {
                bleKeyboard.write(' ');
            } else {
                bleKeyboard.print(c);
            }
        }
    }
}

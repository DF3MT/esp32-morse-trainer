#include "ble_manager.h"
#include <BleKeyboard.h>
#include <NimBLEDevice.h>

bool          BleManager::_enabled = false;
bool          BleManager::_started = false;
BleLayout     BleManager::_layout  = BleLayout::GERMAN_DE;
QueueHandle_t BleManager::_queue   = nullptr;

// ============================================================
// Custom NimBLE-compatible BleKeyboard with optimized security
// and connection parameters for coexistence with Wi-Fi SoftAP
// ============================================================
class MorseBleKeyboard : public BleKeyboard {
public:
    MorseBleKeyboard(std::string deviceName, std::string deviceManufacturer, uint8_t batteryLevel)
        : BleKeyboard(deviceName, deviceManufacturer, batteryLevel) {}

protected:
    virtual void onStarted(BLEServer *pServer) override {
        // Override BleKeyboard's default MITM=true security setting.
        // For standard HID Keyboards without numeric entry/display, MITM must be false
        // so Windows/macOS/iOS/Android perform seamless "Just Works" pairing.
        NimBLEDevice::setSecurityAuth(true, false, false); // bonding=true, mitm=false, sc=false
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
        NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
        NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
        Serial.println("[BLE] Security initialized: Just Works (Bonding=ON, MITM=OFF, IO=None)");
    }

    virtual void onConnect(BLEServer* pServer, ble_gap_conn_desc* desc) override {
        BleKeyboard::onConnect(pServer);
        Serial.printf("[BLE] Host connected! Handle=%u, Interval=%u, Latency=%u, Timeout=%u\n",
                      desc->conn_handle, desc->conn_itvl, desc->conn_latency, desc->supervision_timeout);

        // Update connection parameters to ensure coexistence with Wi-Fi SoftAP:
        // minInterval = 24 (30ms), maxInterval = 40 (50ms)
        // latency = 2 (allows skipping 2 connection events when Wi-Fi radio is busy)
        // timeout = 400 (4000ms = 4.0s supervision timeout to prevent Wi-Fi beacon dropouts)
        pServer->updateConnParams(desc->conn_handle, 24, 40, 2, 400);
    }

    virtual void onDisconnect(BLEServer* pServer, ble_gap_conn_desc* desc) override {
        BleKeyboard::onDisconnect(pServer);
        Serial.printf("[BLE] Host disconnected (handle: %u).\n", desc->conn_handle);

        // Ensure advertising is restarted so host can reconnect cleanly
        NimBLEAdvertising* pAdv = pServer->getAdvertising();
        if (pAdv && !pAdv->isAdvertising()) {
            pAdv->start();
            Serial.println("[BLE] Advertising restarted.");
        }
    }

    virtual void onAuthenticationComplete(ble_gap_conn_desc* desc) override {
        Serial.printf("[BLE] Authentication complete: paired=%d, bonded=%d, encrypted=%d\n",
                      desc->sec_state.bonded, desc->sec_state.bonded, desc->sec_state.encrypted);
    }
};

static MorseBleKeyboard bleKeyboard("CW-Keyer DF3MT", "DF3MT", 100);

void BleManager::begin(bool enabled, BleLayout layout) {
    _layout = layout;
    if (!_queue) {
        _queue = xQueueCreate(128, sizeof(char));
    }
    _enabled = enabled;
    if (_enabled && !_started) {
        bleKeyboard.begin();
        // Redundant enforcement of Just Works pairing parameters
        NimBLEDevice::setSecurityAuth(true, false, false);
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
        NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
        NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
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
        NimBLEDevice::setSecurityAuth(true, false, false);
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
        NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
        NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
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

void BleManager::clearBonds() {
    Serial.println("[BLE] Clearing all stored Bluetooth pairing bonds...");
    NimBLEDevice::deleteAllBonds();
    Serial.printf("[BLE] Stored bonds remaining: %d\n", NimBLEDevice::getNumBonds());
}

void BleManager::pushChar(char c) {
    if (!_enabled || !_started || !_queue) return;
    // 0-wait push to never block the high-priority keyer/decoder loop on Core 0
    xQueueSend(_queue, &c, 0);
}

void BleManager::pushString(const char* str) {
    if (!str || !_enabled || !_started || !_queue) return;
    while (*str) {
        pushChar(*str++);
    }
}

void BleManager::sendRawKey(uint8_t keycode, uint8_t modifiers) {
    if (!bleKeyboard.isConnected()) return;
    KeyReport report = {0};
    report.modifiers = modifiers;
    report.keys[0] = keycode;
    bleKeyboard.sendReport(&report);
    vTaskDelay(15 / portTICK_PERIOD_MS);
    bleKeyboard.releaseAll();
    vTaskDelay(10 / portTICK_PERIOD_MS);
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
                case '@':
                    // German QWERTZ: AltGr + Q
                    keycode = 0x14; // Key 'Q'
                    modifiers = 0x40; // AltGr (Right Alt)
                    break;
                case '<':
                    // German QWERTZ: Key 0x64 (between Left Shift and Y)
                    keycode = 0x64;
                    modifiers = 0x00;
                    break;
                case '>':
                    // German QWERTZ: Shift + 0x64
                    keycode = 0x64;
                    modifiers = 0x02;
                    break;
                case '\'':
                    // German QWERTZ: Shift + # (0x31)
                    keycode = 0x31;
                    modifiers = 0x02;
                    break;
                case '"':
                    // German QWERTZ: Shift + 2 (0x1f)
                    keycode = 0x1f;
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
                case '@':
                    // US QWERTY: Shift + 2
                    keycode = 0x1f; // Key '2'
                    modifiers = 0x02; // Shift
                    break;
                case '<':
                    // US QWERTY: Shift + , (0x36)
                    keycode = 0x36;
                    modifiers = 0x02;
                    break;
                case '>':
                    // US QWERTY: Shift + . (0x37)
                    keycode = 0x37;
                    modifiers = 0x02;
                    break;
                case '\'':
                    // US QWERTY: Key 0x34
                    keycode = 0x34;
                    modifiers = 0x00;
                    break;
                case '"':
                    // US QWERTY: Shift + ' (0x34)
                    keycode = 0x34;
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

    Serial.printf("[BLE] sendChar '%c' (0x%02X) -> code=0x%02X, mod=0x%02X\n", c, (uint8_t)c, keycode, modifiers);
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

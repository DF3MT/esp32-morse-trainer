#include "ble_manager.h"
#include <BleKeyboard.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <nvs.h>
#include <esp_random.h>
#include "nimble/nimble/host/include/host/ble_hs_pvcy.h"

// ============================================================
// BLE identity address
//
// A host (Windows in particular) that still holds a bond for this
// device, while the device itself has lost it (NVS erased by a full
// flash, "Reset bonds" in the menu, ...), reconnects every ~0.5 s and
// immediately drops the link again with 0x13 — an endless up/down loop
// that can only be ended by removing the device on the host.
//
// To make that impossible, the keyboard advertises with a static random
// address that is regenerated whenever the device holds no bonds. A host
// with a stale bond then simply sees its old device as "offline" and the
// keyer shows up as a new, pairable keyboard.
// ============================================================
static void dumpBondStore(const char* when);

static const char* BLE_ID_NS  = "bleid";
static const char* BLE_ID_KEY = "addr";

static void generateStaticRandomAddr(uint8_t addr[6]) {
    esp_fill_random(addr, 6);
    addr[5] |= 0xC0; // NimBLE stores addresses LSB first; top two bits = 11 → static random
}

static void storeIdentity(const uint8_t addr[6]) {
    Preferences p;
    if (p.begin(BLE_ID_NS, false)) {
        p.putBytes(BLE_ID_KEY, addr, 6);
        p.end();
    }
}

static bool loadIdentity(uint8_t addr[6]) {
    Preferences p;
    if (!p.begin(BLE_ID_NS, true)) return false;
    bool ok = p.getBytesLength(BLE_ID_KEY) == 6 && p.getBytes(BLE_ID_KEY, addr, 6) == 6;
    p.end();
    return ok;
}

static bool applyIdentity(const uint8_t addr[6]) {
    // setOwnAddrType(RANDOM) enables resolvable private addresses (RPA) and writes
    // a freshly generated RPA into the controller. A host holding our old IRK
    // resolves that RPA back to its stale bond and loops again, so: select the
    // random address type, switch RPA off again, and only then set our static
    // random address (otherwise the RPA overwrites it).
    NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_RANDOM);
#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
    ble_hs_pvcy_rpa_config(NIMBLE_HOST_DISABLE_PRIVACY);
#endif
    int rc = ble_hs_id_set_rnd(addr);
    if (rc != 0) {
        Serial.printf("[BLE] Setting random address failed (rc=%d), using public address\n", rc);
        NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_PUBLIC);
        return false;
    }
    Serial.printf("[BLE] Identity address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                  addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
    return true;
}

// Called once after NimBLE init, before advertising starts.
static void setupIdentity() {
    uint8_t addr[6];
    int bonds = NimBLEDevice::getNumBonds();
    bool haveStored = loadIdentity(addr);

    if (bonds == 0) {
        // No bonds: any host that still "knows" us holds a stale bond → new identity.
        generateStaticRandomAddr(addr);
        storeIdentity(addr);
        Serial.println("[BLE] No bonds stored -> new identity address generated");
        applyIdentity(addr);
    } else if (haveStored) {
        applyIdentity(addr);
    } else {
        // Bonds from an older firmware made with the public address: keep it.
        Serial.println("[BLE] Legacy bonds found -> keeping public address");
    }
}

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
        // SC_BOND: Bonding=true, MITM=false, SC=true (Secure Connections without MITM PIN requirement)
        NimBLEDevice::setSecurityAuth(true, false, true);
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
        NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
        NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
        Serial.println("[BLE] Security initialized: Bonding=ON, MITM=OFF, SC=ON, IO=None");
        // Runs before BleKeyboard::begin() starts advertising
        setupIdentity();
    }

    virtual void onConnect(BLEServer* pServer, ble_gap_conn_desc* desc) override {
        BleKeyboard::onConnect(pServer);
        Serial.printf("[BLE] Host connected! Handle=%u, Peer=%s, Our=%s(type %u), Interval=%u, Latency=%u, Timeout=%u, bonded=%d\n",
                      desc->conn_handle,
                      NimBLEAddress(desc->peer_id_addr).toString().c_str(),
                      NimBLEAddress(desc->our_ota_addr).toString().c_str(), desc->our_ota_addr.type,
                      desc->conn_itvl, desc->conn_latency, desc->supervision_timeout,
                      desc->sec_state.bonded);
        // HID-over-GATT requires an encrypted link: ask for it right away instead
        // of waiting for the host. If the host holds a stale bond, NimBLE's
        // REPEAT_PAIRING handler deletes the old key and re-pairs.
        NimBLEDevice::startSecurity(desc->conn_handle);
    }

    virtual void onDisconnect(BLEServer* pServer, ble_gap_conn_desc* desc) override {
        BleKeyboard::onDisconnect(pServer);
        Serial.printf("[BLE] Host disconnected! Handle=%u\n",
                      desc ? desc->conn_handle : 0);
        // NimBLE automatically restarts advertising (m_advertiseOnDisconnect = true)
    }

    virtual void onAuthenticationComplete(ble_gap_conn_desc* desc) override {
        Serial.printf("[BLE] Auth complete: authenticated=%d, bonded=%d, encrypted=%d\n",
                      desc->sec_state.authenticated, desc->sec_state.bonded, desc->sec_state.encrypted);
    }

    virtual uint32_t onPassKeyRequest() override {
        Serial.println("[BLE] PassKey requested by host: returning 0");
        return 0;
    }

    virtual bool onConfirmPIN(uint32_t pin) override {
        Serial.printf("[BLE] ConfirmPIN requested: %u -> auto-accepted\n", pin);
        return true;
    }
};

static MorseBleKeyboard bleKeyboard("CW-Keyer DF3MT", "DF3MT", 100);

// Diagnostic GAP handler: logs low-level reasons that the server callbacks hide
static int bleGapDiagHandler(ble_gap_event* event, void* /*arg*/) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            Serial.printf("[BLE-GAP] connect status=%d\n", event->connect.status);
            break;
        case BLE_GAP_EVENT_DISCONNECT:
            Serial.printf("[BLE-GAP] disconnect reason=0x%03X (hci=0x%02X)\n",
                          event->disconnect.reason, event->disconnect.reason & 0xFF);
            break;
        case BLE_GAP_EVENT_ENC_CHANGE:
            Serial.printf("[BLE-GAP] enc_change status=%d\n", event->enc_change.status);
            break;
        case BLE_GAP_EVENT_REPEAT_PAIRING:
            Serial.println("[BLE-GAP] repeat pairing (stale bond on host or device)");
            break;
        case BLE_GAP_EVENT_CONN_UPDATE:
            Serial.printf("[BLE-GAP] conn_update status=%d\n", event->conn_update.status);
            break;
        case BLE_GAP_EVENT_SUBSCRIBE:
            Serial.printf("[BLE-GAP] subscribe attr=%u notify=%d\n",
                          event->subscribe.attr_handle, event->subscribe.cur_notify);
            break;
        default:
            break;
    }
    return 0;
}

static void dumpBondStore(const char* when) {
    Serial.printf("[BLE] --- bond store (%s) ---\n", when);
    Serial.printf("[BLE] Stored bonds: %d\n", NimBLEDevice::getNumBonds());
    for (int i = 0; i < NimBLEDevice::getNumBonds(); i++) {
        Serial.printf("[BLE]   bond %d: %s\n", i, NimBLEDevice::getBondedAddress(i).toString().c_str());
    }
    int n = 0;
    nvs_iterator_t it = nvs_entry_find("nvs", "nimble_bond", NVS_TYPE_ANY);
    while (it) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        Serial.printf("[BLE]   nvs nimble_bond/%s\n", info.key);
        n++;
        it = nvs_entry_next(it);
    }
    nvs_stats_t st;
    if (nvs_get_stats(NULL, &st) == ESP_OK) {
        Serial.printf("[BLE] NVS: nimble_bond entries=%d, used=%u free=%u total=%u\n",
                      n, (unsigned)st.used_entries, (unsigned)st.free_entries, (unsigned)st.total_entries);
    }
}

static void registerDiagnostics() {
    static bool registered = false;
    if (!registered) {
        NimBLEDevice::setCustomGapHandler(bleGapDiagHandler);
        registered = true;
    }
    dumpBondStore("boot");
}

// NimBLE stack + HID services are created exactly once. BleKeyboard::end()
// is a no-op in the library, so calling begin() again would register the
// server and HID services a second time; disable/enable only toggles the radio.
static bool bleStackInitialized = false;

static void startKeyboardStack() {
    if (!bleStackInitialized) {
        bleKeyboard.begin(); // init stack, create services, onStarted() → identity, start advertising
        // Redundant enforcement of Secure Connections pairing parameters
        NimBLEDevice::setSecurityAuth(true, false, true);
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
        NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
        NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
        registerDiagnostics();
        bleStackInitialized = true;
    } else {
        NimBLEDevice::startAdvertising();
    }
}

static void disconnectAllPeers() {
    NimBLEServer* server = NimBLEDevice::getServer();
    if (!server) return;
    for (uint16_t handle : server->getPeerDevices()) {
        server->disconnect(handle);
    }
}

static void stopKeyboardStack() {
    if (!bleStackInitialized) return;
    NimBLEServer* server = NimBLEDevice::getServer();
    if (server) server->advertiseOnDisconnect(false);
    NimBLEDevice::stopAdvertising();
    disconnectAllPeers();
}

void BleManager::begin(bool enabled, BleLayout layout) {
    _layout = layout;
    if (!_queue) {
        _queue = xQueueCreate(128, sizeof(char));
    }
    _enabled = enabled;
    if (_enabled && !_started) {
        startKeyboardStack();
        _started = true;
        Serial.printf("[BLE] HID Keyboard started (Layout: %s). Advertising as 'CW-Keyer DF3MT'...\n",
                      _layout == BleLayout::GERMAN_DE ? "DE/QWERTZ" : "US/QWERTY");
    }
}

void BleManager::end() {
    if (_started) {
        stopKeyboardStack();
        _started = false;
        Serial.println("[BLE] HID Keyboard stopped.");
    }
}

void BleManager::setEnabled(bool enabled) {
    if (_enabled == enabled) return;
    _enabled = enabled;
    if (_enabled && !_started) {
        NimBLEServer* server = NimBLEDevice::getServer();
        if (server) server->advertiseOnDisconnect(true);
        startKeyboardStack();
        _started = true;
        Serial.println("[BLE] HID Keyboard enabled and advertising.");
    } else if (!_enabled && _started) {
        stopKeyboardStack();
        _started = false;
        Serial.println("[BLE] HID Keyboard disabled (advertising stopped, peers disconnected).");
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

    if (!bleStackInitialized) return; // new identity is picked at next start (0 bonds)

    // Rotate the identity address so hosts that still hold the old bond
    // cannot fall into a reconnect loop with us.
    NimBLEDevice::stopAdvertising();
    disconnectAllPeers();
    uint8_t addr[6];
    generateStaticRandomAddr(addr);
    storeIdentity(addr);
    applyIdentity(addr);
    if (_enabled && _started) {
        NimBLEDevice::startAdvertising();
    }
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

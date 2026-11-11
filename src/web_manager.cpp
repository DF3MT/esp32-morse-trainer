#include "web_manager.h"
#include "web_page.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WebSocketsServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// ============================================================
// Internal State & FreeRTOS Event Queue
// ============================================================
enum class WebEvtType {
    TOKEN,
    ELEM,
    WPM
};

struct WebEvt {
    WebEvtType type;
    char text[16];
    int wpm;
};

static QueueHandle_t    _evtQueue = nullptr;
static WebServer        _server(80);
static WebSocketsServer _ws(81);
static bool             _begun = false;
static bool             _enabled = true;
static bool             _serversRunning = false;
static String           _ssid = "DF3MT-MorseTrainer";
static String           _password = "";
static int              _currentWpm = 20;
static String           _history = "";
static String           _lang = "de";
static const size_t     MAX_HISTORY_LEN = 2048;

// JSON escaping helper
static String escapeJson(const String& in) {
    String out;
    out.reserve(in.length() + 8);
    for (size_t i = 0; i < in.length(); i++) {
        char c = in[i];
        if (c == '"')       out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

// Initial state message (speed, UI language, transcript)
static String buildInitMsg(const String& history) {
    return "{\"t\":\"init\",\"wpm\":" + String(_currentWpm) + ",\"lang\":\"" + _lang + "\",\"history\":\"" + escapeJson(history) + "\"}";
}

// WebSocket Event Handler
static void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED: {
            IPAddress ip = _ws.remoteIP(num);
            Serial.printf("[WEB] Client #%u connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
            // Send initial state & full history to newly connected browser client
            String initMsg = buildInitMsg(_history);
            _ws.sendTXT(num, initMsg);
            break;
        }

        case WStype_DISCONNECTED:
            Serial.printf("[WEB] Client #%u disconnected\n", num);
            break;

        case WStype_TEXT: {
            String text = String((char*)payload);
            if (text.indexOf("\"clear\"") >= 0) {
                Serial.println("[WEB] Clear requested by client");
                _history = "";
                { String msg = buildInitMsg(""); _ws.broadcastTXT(msg); }
            }
            break;
        }

        default:
            break;
    }
}

// ============================================================
// Internal AP & Web Services Lifecycle
// ============================================================
static void startApAndServers() {
    if (_serversRunning) return;

    Serial.println("[WEB] Starting Wi-Fi SoftAP and Web Services...");

    WiFi.persistent(false);
    WiFi.mode(WIFI_AP);

    IPAddress apIP(192, 168, 4, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

    if (_password.length() >= 8) {
        WiFi.softAP(_ssid.c_str(), _password.c_str(), 1, 0, 4);
    } else {
        WiFi.softAP(_ssid.c_str(), nullptr, 1, 0, 4);
    }

    Serial.printf("[WEB] Access Point '%s' active on IP %s\n", _ssid.c_str(), WiFi.softAPIP().toString().c_str());

    // Setup mDNS: http://df3mt.local and http://morse.local
    if (MDNS.begin("df3mt")) {
        MDNS.addService("http", "tcp", 80);
        MDNS.addService("ws", "tcp", 81);
        Serial.println("[WEB] mDNS active: http://df3mt.local");
    }

    // Configure WebServer routes
    _server.on("/", HTTP_GET, []() {
        _server.send_P(200, "text/html", INDEX_HTML);
    });

    _server.on("/api/status", HTTP_GET, []() {
        char buf[128];
        snprintf(buf, sizeof(buf), "{\"clients\":%d,\"stations\":%d,\"wpm\":%d,\"lang\":\"%s\"}",
                 _ws.connectedClients(), WiFi.softAPgetStationNum(), _currentWpm, _lang.c_str());
        _server.send(200, "application/json", buf);
    });

    // Captive portal detection redirects
    _server.on("/generate_204", HTTP_GET, []() {
        _server.sendHeader("Location", "http://192.168.4.1/", true);
        _server.send(302, "text/plain", "");
    });
    _server.on("/hotspot-detect.html", HTTP_GET, []() {
        _server.sendHeader("Location", "http://192.168.4.1/", true);
        _server.send(302, "text/plain", "");
    });

    _server.onNotFound([]() {
        _server.sendHeader("Location", "http://192.168.4.1/", true);
        _server.send(302, "text/plain", "");
    });

    _server.begin();
    Serial.println("[WEB] HTTP Server started on port 80");

    // Setup WebSockets server
    _ws.begin();
    _ws.onEvent(onWebSocketEvent);
    Serial.println("[WEB] WebSocket Server started on port 81");

    _serversRunning = true;
}

static void stopApAndServers() {
    if (!_serversRunning) return;

    Serial.println("[WEB] Stopping Wi-Fi SoftAP and Web Services...");
    _ws.close();
    _server.stop();
    MDNS.end();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    _serversRunning = false;
    Serial.println("[WEB] Wi-Fi SoftAP completely stopped (Radio OFF).");
}

// ============================================================
// WebManager Public API
// ============================================================

void WebManager::begin(bool enabled, const char* ssid, const char* password) {
    if (_begun) return;

    _enabled = enabled;
    if (ssid) _ssid = ssid;
    if (password) _password = password;

    // Create FreeRTOS queue for cross-core messaging (Core 0 keyer -> Core 1 web server)
    _evtQueue = xQueueCreate(64, sizeof(WebEvt));

    if (_enabled) {
        startApAndServers();
    } else {
        WiFi.mode(WIFI_OFF);
        Serial.println("[WEB] Wi-Fi initially disabled by configuration (Radio OFF).");
    }

    _begun = true;
}

void WebManager::setEnabled(bool enabled) {
    if (_enabled == enabled) return;
    _enabled = enabled;
    if (_enabled) {
        startApAndServers();
    } else {
        stopApAndServers();
    }
}

bool WebManager::isEnabled() {
    return _enabled;
}

void WebManager::tick() {
    if (!_begun) return;

    // Process queued events from Core 0
    WebEvt ev;
    while (_evtQueue && xQueueReceive(_evtQueue, &ev, 0) == pdTRUE) {
        if (ev.type == WebEvtType::TOKEN) {
            _currentWpm = ev.wpm;
            _history += ev.text;
            if (_history.length() > MAX_HISTORY_LEN) {
                // Keep the most recent characters
                _history = _history.substring(_history.length() - (MAX_HISTORY_LEN - 256));
            }
            if (_serversRunning) {
                String payload = "{\"t\":\"token\",\"val\":\"" + escapeJson(ev.text) + "\",\"wpm\":" + String(ev.wpm) + "}";
                _ws.broadcastTXT(payload);
            }
        } else if (ev.type == WebEvtType::ELEM) {
            if (_serversRunning) {
                String payload = "{\"t\":\"elem\",\"val\":\"" + String(ev.text) + "\"}";
                _ws.broadcastTXT(payload);
            }
        } else if (ev.type == WebEvtType::WPM) {
            _currentWpm = ev.wpm;
            if (_serversRunning) {
                String payload = "{\"t\":\"wpm\",\"val\":" + String(ev.wpm) + "}";
                _ws.broadcastTXT(payload);
            }
        }
    }

    if (_serversRunning) {
        _ws.loop();
        _server.handleClient();
    }
}

void WebManager::pushToken(const char* token, int wpm) {
    if (!_begun || !_evtQueue || !token || !token[0]) return;

    WebEvt ev;
    ev.type = WebEvtType::TOKEN;
    strncpy(ev.text, token, sizeof(ev.text) - 1);
    ev.text[sizeof(ev.text) - 1] = '\0';
    ev.wpm = wpm;

    // Non-blocking send: if queue is full, drop to preserve Core 0 real-time timing
    xQueueSend(_evtQueue, &ev, 0);
}

void WebManager::pushElement(KeyerElement elem) {
    if (!_begun || !_evtQueue) return;

    WebEvt ev;
    ev.type = WebEvtType::ELEM;
    ev.text[0] = (elem == KeyerElement::DIT) ? '.' : '-';
    ev.text[1] = '\0';
    ev.wpm = 0;

    xQueueSend(_evtQueue, &ev, 0);
}

void WebManager::pushWpm(int wpm) {
    if (!_begun || !_evtQueue) return;

    WebEvt ev;
    ev.type = WebEvtType::WPM;
    ev.text[0] = '\0';
    ev.wpm = wpm;

    xQueueSend(_evtQueue, &ev, 0);
}

void WebManager::setLanguage(const char* code) {
    if (!code || !code[0] || _lang == code) return;
    _lang = code;
    if (_begun && _serversRunning) {
        String msg = "{\"t\":\"lang\",\"val\":\"" + _lang + "\"}";
        _ws.broadcastTXT(msg);
    }
}

void WebManager::clearHistory() {
    _history = "";
    if (_begun) {
        { String msg = buildInitMsg(""); _ws.broadcastTXT(msg); }
    }
}

bool WebManager::isApActive() {
    return _begun && _serversRunning && (WiFi.getMode() & WIFI_AP);
}

uint8_t WebManager::getClientCount() {
    if (!_begun || !_serversRunning) return 0;
    return _ws.connectedClients();
}

IPAddress WebManager::getIP() {
    if (!_begun || !_serversRunning) return IPAddress(0, 0, 0, 0);
    return WiFi.softAPIP();
}

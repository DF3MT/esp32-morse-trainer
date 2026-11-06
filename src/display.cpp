#include "display.h"
#include "config.h"

static const uint8_t BL_CHANNEL = 0;
static const uint8_t BL_RESOLUTION = 8;
static bool _useSprite = false;

DisplayManager::DisplayManager() : _tft(), _spr(&_tft) {}

void DisplayManager::begin() {
    // 1. Backlight on (Direct GPIO HIGH for guaranteed visibility)
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);

    // 2. Init ST7789 TFT
    _tft.init();
    _tft.setRotation(1);   // Landscape: 240 wide x 135 tall

    // 3. Show full-screen DF3MT boot splash screen
    showSplashScreen();

    // 4. Try to allocate full-screen sprite (64.8 KB contiguous RAM)
    void* sprPtr = _spr.createSprite(240, 135);
    _useSprite = (sprPtr != nullptr);

    Serial.printf("[DISPLAY] ST7789 Init done. Sprite: %s | FreeHeap: %u | MaxAlloc: %u\n",
                  _useSprite ? "OK (Double-buffered)" : "FAILED (Direct TFT Mode)",
                  ESP.getFreeHeap(),
                  ESP.getMaxAllocHeap());

    if (_useSprite) {
        _spr.setTextDatum(TL_DATUM);
    }

    _begun = true;
}

// ============================================================
// Full-screen Boot Splash Screen ("DF3MT", df3mt.de, github)
// ============================================================
void DisplayManager::showSplashScreen() {
    _tft.fillScreen(C_BG);

    // Top & bottom accent border lines (Cyan)
    _tft.drawFastHLine(8, 4, 224, C_ACCENT);
    _tft.drawFastHLine(8, 130, 224, C_ACCENT);

    // Main Callsign "DF3MT" — 52px tall (Font 4, size 2)
    _tft.setTextFont(4);
    _tft.setTextSize(2);
    _tft.setTextColor(C_FG, C_BG);
    int callWidth = _tft.textWidth("DF3MT", 4) * 2;
    int callX = (240 - callWidth) / 2;
    _tft.setCursor(callX, 12);
    _tft.print("DF3MT");
    _tft.setTextSize(1); // Reset size back to 1

    // Subtle divider line
    _tft.drawFastHLine(24, 68, 192, 0x2945);

    // URL 1: https://df3mt.de (Cyan, Font 2)
    _tft.setTextFont(2);
    _tft.setTextColor(C_ACCENT, C_BG);
    _tft.drawCentreString("https://df3mt.de", 120, 74, 2);

    // URL 2: GitHub URL (Yellow, Font 1)
    _tft.setTextFont(1);
    _tft.setTextColor(0xFFE0, C_BG);
    _tft.drawCentreString("github.com/DF3MT/esp32-morse-trainer", 120, 96, 1);

    // Subtitle: CW Trainer & BLE Keyer (Light grey, Font 1)
    _tft.setTextColor(C_DIM, C_BG);
    _tft.drawCentreString("CW MORSE TRAINER & BLE KEYER", 120, 112, 1);
}

void DisplayManager::setBrightness(uint8_t val) {
    if (val >= 240) {
        ledcDetachPin(PIN_TFT_BL);
        pinMode(PIN_TFT_BL, OUTPUT);
        digitalWrite(PIN_TFT_BL, HIGH);
    } else if (val <= 10) {
        ledcDetachPin(PIN_TFT_BL);
        pinMode(PIN_TFT_BL, OUTPUT);
        digitalWrite(PIN_TFT_BL, LOW);
    } else {
        ledcSetup(BL_CHANNEL, 5000, BL_RESOLUTION);
        ledcAttachPin(PIN_TFT_BL, BL_CHANNEL);
        ledcWrite(BL_CHANNEL, val);
    }
}

// ============================================================
// Sleep screen & low-power display shutdown
// ============================================================
void DisplayManager::showSleepScreen() {
    if (!_begun) return;
    TFT_eSPI& target = _useSprite ? (TFT_eSPI&)_spr : _tft;

    if (_useSprite) {
        _spr.fillSprite(C_BG);
    } else {
        _tft.fillScreen(C_BG);
    }

    // Top status bar
    target.fillRect(0, 0, 240, 20, C_BAR);
    target.setTextColor(C_WARN, C_BAR);
    target.setTextFont(2);
    target.setCursor(4, 3);
    target.print("STANDBY-MODUS");

    // Title
    target.setTextFont(4);
    target.setTextColor(C_FG, C_BG);
    target.setCursor(20, 34);
    target.print("DEEP SLEEP");

    // Hint: How to wake up
    target.setTextFont(2);
    target.setTextColor(C_ACCENT, C_BG);
    target.setCursor(12, 68);
    target.print("Drehknopf druecken zum Aufwecken");

    target.setTextColor(C_DIM, C_BG);
    target.setCursor(75, 98);
    target.print("73 de DF3MT");

    if (_useSprite) {
        _spr.pushSprite(0, 0);
    }
}

void DisplayManager::sleep() {
    if (!_begun) return;
    // Detach PWM and force backlight LOW (OFF)
    ledcDetachPin(PIN_TFT_BL);
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, LOW);

    // Send ST7789 sleep-in command (0x10) to minimize display controller draw
    _tft.writecommand(0x10);
    delay(5);
}

// ============================================================
// Status bar — top 20px: [MODE] [WPM] [BLE] [AUTO/MAN] [KEY indicator]
// ============================================================
void DisplayManager::drawStatusBar(const char* label, int wpm, bool autoMode, bool autoReady, bool keyDown, bool bleEnabled, bool bleConnected) {
    TFT_eSPI& target = _useSprite ? (TFT_eSPI&)_spr : _tft;

    target.fillRect(0, 0, 240, 20, C_BAR);

    // Left: screen label
    target.setTextColor(C_ACCENT, C_BAR);
    target.setTextFont(2);
    target.setCursor(4, 3);
    target.print(label);

    // Centre-left: WPM
    char buf[16];
    snprintf(buf, sizeof(buf), "%d WPM", wpm);
    target.setTextColor(C_FG, C_BAR);
    target.setCursor(76, 3);
    target.print(buf);

    // Centre-right: BLE badge
    if (bleEnabled) {
        if (bleConnected) {
            // Bright blue pill: connected
            target.fillRoundRect(134, 2, 34, 16, 3, 0x03FF);
            target.setTextColor(0x0000, 0x03FF);
            target.setTextFont(2);
            target.setCursor(137, 3);
            target.print("BLE");
        } else {
            // Outline: advertising / waiting for host
            target.drawRoundRect(134, 2, 34, 16, 3, 0x4208);
            target.setTextColor(0x7BEF, C_BAR);
            target.setTextFont(2);
            target.setCursor(137, 3);
            target.print("BLE");
        }
    }

    // Right: AUTO/MAN badge
    if (autoMode) {
        uint16_t col = autoReady ? C_OK : C_WARN;
        target.setTextColor(col, C_BAR);
        target.setCursor(176, 3);
        target.print(autoReady ? "AUTO" : "ACQ ");
    } else {
        target.setTextColor(C_DIM, C_BAR);
        target.setCursor(176, 3);
        target.print("MAN ");
    }

    // Key-down LED dot
    target.fillCircle(230, 10, 5, keyDown ? C_OK : 0x3186);
}

// ============================================================
// Decode screen — shows 3 full lines of decoded text (rolling scroll)
// ============================================================
void DisplayManager::showDecodeScreen(const char* text, int wpm, bool autoMode, bool autoReady, bool keyDown, bool bleEnabled, bool bleConnected) {
    if (!_begun) return;
    TFT_eSPI& target = _useSprite ? (TFT_eSPI&)_spr : _tft;

    if (_useSprite) {
        _spr.fillSprite(C_BG);
    } else {
        _tft.fillScreen(C_BG);
    }

    drawStatusBar("DECODE", wpm, autoMode, autoReady, keyDown, bleEnabled, bleConnected);

    // Bottom hint
    target.fillRect(0, 118, 240, 17, 0x18C3);
    target.setTextFont(1);
    target.setTextColor(C_ACCENT, 0x18C3);
    target.setCursor(4, 122);
    target.print("DREHEN: WPM  |  KLICK: MENUE OEFFNEN");

    // Decoded text: exactly 3 full lines in Font 4 (y = 24, 54, 84)
    target.setTextFont(4);
    target.setTextColor(C_FG, C_BG);

    static const int MAX_STORED_LINES = 16;
    static const int MAX_LINE_CHARS = 32;
    char lines[MAX_STORED_LINES][MAX_LINE_CHARS];
    int lineCount = 0;
    lines[0][0] = '\0';

    int curLen = 0;
    int curWidth = 0;
    const int maxLineWidth = 230; // 240px width with 5px margins on each side

    const char* p = text;
    while (*p) {
        char ch = *p++;
        char tempStr[2] = { ch, '\0' };
        // Font 4 default space is only 5px wide; use 10px so word space is clearly visible
        int chWidth = (ch == ' ') ? 10 : target.textWidth(tempStr, 4);

        if (curWidth + chWidth > maxLineWidth) {
            if (lineCount < MAX_STORED_LINES - 1) {
                lineCount++;
            } else {
                for (int i = 0; i < MAX_STORED_LINES - 1; i++) {
                    strcpy(lines[i], lines[i + 1]);
                }
            }
            lines[lineCount][0] = '\0';
            curLen = 0;
            curWidth = 0;
            if (ch == ' ') continue; // Don't start new line with leading space
        }

        if (curLen < MAX_LINE_CHARS - 1) {
            lines[lineCount][curLen++] = ch;
            lines[lineCount][curLen] = '\0';
            curWidth += chWidth;
        }
    }

    int totalLines = lineCount + 1;
    // Always show the 3 most recent lines
    int startLine = (totalLines > 3) ? (totalLines - 3) : 0;
    int yPositions[3] = { 24, 54, 84 };

    for (int i = 0; i < 3; i++) {
        int lIdx = startLine + i;
        if (lIdx < totalLines && lines[lIdx][0] != '\0') {
            target.setCursor(4, yPositions[i]);
            for (const char* cp = lines[lIdx]; *cp; cp++) {
                if (*cp == ' ') {
                    target.setCursor(target.getCursorX() + 10, yPositions[i]);
                } else {
                    target.print(*cp);
                }
            }
        }
    }

    if (_useSprite) {
        _spr.pushSprite(0, 0);
    }
}

// ============================================================
// Helper: Morse code pattern lookup for training hint
// ============================================================
static const char* getSingleCharMorseHint(char c) {
    switch (toupper(c)) {
        case 'A': return ".-";
        case 'B': return "-...";
        case 'C': return "-.-.";
        case 'D': return "-..";
        case 'E': return ".";
        case 'F': return "..-.";
        case 'G': return "--.";
        case 'H': return "....";
        case 'I': return "..";
        case 'J': return ".---";
        case 'K': return "-.-";
        case 'L': return ".-..";
        case 'M': return "--";
        case 'N': return "-.";
        case 'O': return "---";
        case 'P': return ".--.";
        case 'Q': return "--.-";
        case 'R': return ".-.";
        case 'S': return "...";
        case 'T': return "-";
        case 'U': return "..-";
        case 'V': return "...-";
        case 'W': return ".--";
        case 'X': return "-..-";
        case 'Y': return "-.--";
        case 'Z': return "--..";
        case '0': return "-----";
        case '1': return ".----";
        case '2': return "..---";
        case '3': return "...--";
        case '4': return "....-";
        case '5': return ".....";
        case '6': return "-....";
        case '7': return "--...";
        case '8': return "---..";
        case '9': return "----.";
        case '/': return "-..-.";
        case '?': return "..--..";
        case '.': return ".-.-.-";
        case ',': return "--..--";
        case '=': return "-...-";
        case '-': return "-....-";
        case ':': return "---...";
        case ';': return "-.-.-.";
        case '+': return ".-.-.";
        case '(': return "-.--.";
        case ')': return "-.--.-";
        case '@': return ".--.-.";
        default:  return "";
    }
}

static void getMorseHint(const char* target, char* outHint, size_t maxLen) {
    if (!target || !target[0]) {
        outHint[0] = '\0';
        return;
    }

    // Prosign exact hints (Verkehrszeichen gegeben ohne Pause)
    if (strcmp(target, "<SK>") == 0) { snprintf(outHint, maxLen, "...-.-"); return; }
    if (strcmp(target, "<AR>") == 0) { snprintf(outHint, maxLen, ".-.-."); return; }
    if (strcmp(target, "<BT>") == 0) { snprintf(outHint, maxLen, "-...-"); return; }
    if (strcmp(target, "<AS>") == 0) { snprintf(outHint, maxLen, ".-..."); return; }
    if (strcmp(target, "<KN>") == 0) { snprintf(outHint, maxLen, "-.--."); return; }
    if (strcmp(target, "<KA>") == 0) { snprintf(outHint, maxLen, "-.-.-"); return; }
    if (strcmp(target, "<BK>") == 0) { snprintf(outHint, maxLen, "-...-.-"); return; }
    if (strcmp(target, "<VE>") == 0) { snprintf(outHint, maxLen, "...-."); return; }
    if (strcmp(target, "<HH>") == 0) { snprintf(outHint, maxLen, "........"); return; }
    if (strcmp(target, "<SOS>") == 0) { snprintf(outHint, maxLen, "...---..."); return; }

    // Multi-character word hints: concatenate single-char hints separated by spaces
    outHint[0] = '\0';
    for (size_t i = 0; target[i]; i++) {
        const char* h = getSingleCharMorseHint(target[i]);
        if (h && h[0]) {
            if (outHint[0] != '\0') {
                strncat(outHint, " ", maxLen - strlen(outHint) - 1);
            }
            strncat(outHint, h, maxLen - strlen(outHint) - 1);
        }
    }
}

// ============================================================
// Training screen
// ============================================================
void DisplayManager::showTrainingScreen(const TrainingChallenge& ch, bool keyDown, bool bleEnabled, bool bleConnected) {
    if (!_begun) return;
    TFT_eSPI& target = _useSprite ? (TFT_eSPI&)_spr : _tft;

    if (_useSprite) {
        _spr.fillSprite(C_BG);
    } else {
        _tft.fillScreen(C_BG);
    }

    drawStatusBar("TRAINING", ch.wpm, ch.autoMode, ch.autoReady, keyDown, bleEnabled, bleConnected);

    // "Send:" label
    target.setTextFont(2);
    target.setTextColor(C_ACCENT, C_BG);
    target.setCursor(4, 24);
    target.print("Gib:");

    // Target text: adjust position and font size
    target.setTextFont(4);
    size_t tLen = strlen(ch.target);
    if (tLen <= 2) {
        target.setTextSize(2);
        target.setCursor(48, 26);
    } else if (tLen <= 4) {
        target.setTextSize(1);
        target.setCursor(48, 36);
    } else {
        target.setTextSize(1);
        target.setCursor(44, 36);
    }
    target.setTextColor(C_FG, C_BG);
    target.print(ch.target);
    target.setTextSize(1); // Reset size back to 1

    // Morse pattern hint
    char hint[32];
    getMorseHint(ch.target, hint, sizeof(hint));
    if (hint[0] != '\0') {
        if (strlen(hint) > 8) {
            target.setTextFont(2);
            target.setCursor(110, 42);
        } else {
            target.setTextFont(4);
            target.setCursor(115, 36);
        }
        target.setTextColor(C_WARN, C_BG);
        target.print(hint);
    }

    // Feedback
    if (ch.sent[0] != '\0') {
        target.setTextFont(4);
        uint16_t col = ch.correct ? C_OK : C_ERR;
        target.setTextColor(col, C_BG);
        target.setCursor(4, 88);
        target.print(ch.correct ? "RICHTIG!" : "FALSCH ");
        target.setTextFont(2);
        target.setTextColor(0xA514, C_BG);
        target.print("  (gesendet: ");
        target.setTextColor(C_WARN, C_BG);
        target.print(ch.sent);
        target.setTextColor(0xA514, C_BG);
        target.print(")");
    } else {
        target.setTextFont(1);
        target.setTextColor(C_DIM, C_BG);
        target.setCursor(4, 98);
        target.print("KLICK: Menue | DREHEN: Tempo anpassen");
    }

    if (_useSprite) {
        _spr.pushSprite(0, 0);
    }
}

// ============================================================
// Menu screen with values, scrolling and edit-mode highlighting
// ============================================================
void DisplayManager::showMenuScreen(const MenuItemView* items, uint8_t count, uint8_t selected, bool isEditing) {
    if (!_begun || count == 0) return;
    TFT_eSPI& target = _useSprite ? (TFT_eSPI&)_spr : _tft;

    if (_useSprite) {
        _spr.fillSprite(C_BG);
    } else {
        _tft.fillScreen(C_BG);
    }

    // 1. Status bar (top 20px)
    target.fillRect(0, 0, 240, 20, C_BAR);
    target.setTextFont(2);
    target.setTextColor(C_ACCENT, C_BAR);
    target.setCursor(4, 3);
    target.print("EINSTELLUNGEN");

    if (isEditing) {
        target.setTextColor(C_WARN, C_BAR);
        target.setCursor(155, 3);
        target.print("[ EDIT ]");
    } else {
        target.setTextColor(C_FG, C_BAR);
        char posBuf[16];
        snprintf(posBuf, sizeof(posBuf), "%d / %d", selected + 1, count);
        target.setTextDatum(TR_DATUM);
        target.drawString(posBuf, 235, 3);
        target.setTextDatum(TL_DATUM);
    }

    // 2. Visible items window (4 rows of 23px: y = 22, 45, 68, 91)
    const uint8_t visibleRows = 4;
    uint8_t topIndex = 0;
    if (selected >= visibleRows) {
        topIndex = selected - visibleRows + 1;
    }

    for (uint8_t row = 0; row < visibleRows; row++) {
        uint8_t idx = topIndex + row;
        if (idx >= count) break;

        uint16_t y = 22 + row * 23;
        bool isSel = (idx == selected);

        if (isSel) {
            uint16_t bg = isEditing ? 0x9320 : 0x2175;
            target.fillRect(0, y, 234, 22, bg);
        }

        // Left: Label
        target.setTextFont(2);
        uint16_t textCol = isSel ? C_FG : 0xC618;
        uint16_t bgCol = isSel ? (isEditing ? 0x9320 : 0x2175) : C_BG;
        target.setTextColor(textCol, bgCol);
        target.setCursor(4, y + 3);
        if (isSel) {
            target.print(isEditing ? "* " : "> ");
        } else {
            target.print("  ");
        }
        target.print(items[idx].label);

        // Right: Value
        if (items[idx].value && items[idx].value[0] != '\0') {
            target.setTextDatum(TR_DATUM);
            if (isSel && isEditing) {
                target.setTextColor(C_WARN, 0x9320);
                char editBuf[32];
                snprintf(editBuf, sizeof(editBuf), "< %s >", items[idx].value);
                target.drawString(editBuf, 230, y + 3);
            } else {
                target.setTextColor(isSel ? C_ACCENT : 0x9CD3, bgCol);
                target.drawString(items[idx].value, 230, y + 3);
            }
            target.setTextDatum(TL_DATUM);
        }
    }

    // Scroll indicators on the far right
    if (topIndex > 0) {
        target.setTextColor(C_ACCENT, C_BG);
        target.setTextFont(1);
        target.setCursor(235, 23);
        target.print("^");
    }
    if (topIndex + visibleRows < count) {
        target.setTextColor(C_ACCENT, C_BG);
        target.setTextFont(1);
        target.setCursor(235, 105);
        target.print("v");
    }

    // 3. Bottom hint bar (y = 117..134)
    target.fillRect(0, 117, 240, 18, 0x18C3);
    target.setTextFont(1);
    target.setCursor(4, 121);
    if (isEditing) {
        target.setTextColor(C_WARN, 0x18C3);
        target.print("DREHEN: Wert anpassen  |  KLICK: Speichern");
    } else {
        target.setTextColor(C_DIM, 0x18C3);
        target.print("DREHEN: Menue  |  KLICK: Waehlen/Aendern");
    }

    if (_useSprite) {
        _spr.pushSprite(0, 0);
    }
}

// ============================================================
// Score screen
// ============================================================
void DisplayManager::showScoreScreen(int correct, int total, int avgWpm) {
    if (!_begun) return;
    TFT_eSPI& target = _useSprite ? (TFT_eSPI&)_spr : _tft;

    if (_useSprite) {
        _spr.fillSprite(C_BG);
    } else {
        _tft.fillScreen(C_BG);
    }

    target.fillRect(0, 0, 240, 20, C_BAR);
    target.setTextFont(2);
    target.setTextColor(C_ACCENT, C_BAR);
    target.setCursor(4, 3);
    target.print("ERGEBNIS");

    int pct = total > 0 ? (correct * 100 / total) : 0;
    uint16_t col = pct >= 80 ? C_OK : (pct >= 50 ? C_WARN : C_ERR);

    char buf[32];
    snprintf(buf, sizeof(buf), "%d%%", pct);
    target.setTextFont(7);
    target.setTextColor(col, C_BG);
    target.setCursor(60, 28);
    target.print(buf);

    target.setTextFont(2);
    target.setTextColor(C_FG, C_BG);
    snprintf(buf, sizeof(buf), "%d / %d richtig", correct, total);
    target.setCursor(4, 100);
    target.print(buf);

    snprintf(buf, sizeof(buf), "Durchschnitt: %d WPM", avgWpm);
    target.setTextColor(C_DIM, C_BG);
    target.setCursor(4, 118);
    target.print(buf);

    if (_useSprite) {
        _spr.pushSprite(0, 0);
    }
}

# 📻 ESP32 Iambic Morse Code Trainer & Decoder

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Build%20Passed-brightgreen)](https://platformio.org/)
[![ESP32](https://img.shields.io/badge/Hardware-LilyGO%20T--Display%20v1.1-blue)](https://www.lilygo.cc/)
[![Speed](https://img.shields.io/badge/CW%20Speed-3--40%20WPM-orange)]()
[![Amateur Radio](https://img.shields.io/badge/Ham%20Radio-DF3MT-red)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A standalone, high-performance **CW Morse Code Trainer, Iambic Keyer, and Real-Time Decoder** built for the **LilyGO T-Display v1.1** (ESP32 with integrated 1.14" IPS color display).

Designed for both amateur radio operators honing high-speed CW skills (up to 40 WPM) and beginners learning Morse code from scratch using the **Koch Method**.

---

## ✨ Features

* **Dual-Mode Iambic Keyer (Mode A & Mode B)**:
  * Full Curtis 8044 timing emulation with squeeze keying (alternating Dit-Dah).
  * Smooth speed control from **3 WPM to 40 WPM** (single-WPM steps).
  * Robust paddle-release detection and 5 ms hardware debounce: single taps never produce accidental double elements.
  * Paddle swap option (left/right hand) toggleable in menu.
* **Wireless Bluetooth Low Energy (BLE) HID Keyboard**:
  * Emulates a wireless Bluetooth keyboard (**`CW-Keyer DF3MT`**) recognized natively on Windows, macOS, Linux, iOS, and Android without any extra drivers.
  * Transmits decoded Morse code characters, amateur radio prosigns (e.g. `<SK>`, `<AR>`, `<BT>`), and word spaces in real time directly into any text editor, web browser, contest logger (N1MM, Log4OM), or chat app.
  * **Full German QWERTZ & US QWERTY Layout Support**:
    * **German DE Layout (Default)**: Automatically remaps scan codes so that `@` (`AltGr + Q`), `/` (`Shift + 7`), `?` (`Shift + ß`), `=` (`Shift + 0`), `-` (key `0x38`), `<`, `>`, `'`, `"`, and `Z`/`Y` are transmitted perfectly without layout corruption.
    * **US EN Layout**: Standard US ASCII keymap for US-configured hosts (`Shift + 2` for `@`, `<`, `>`, etc.).
    * Switchable anytime directly in the menu (`BLE-Layout: DE (QWERTZ)` / `US (QWERTY)`) and saved to NVS Flash.
  * Live status badge (`[BLE]`) on the screen header shows connection state (dim = advertising/waiting, bright cyan/blue = connected).
  * Toggleable via the configuration menu (`BLE-Tastatur: Ein / Aus`) and saved in NVS Flash.
  * Thread-safe FreeRTOS queue architecture: 0-wait Core 0 transmission ensures zero jitter on the 1 ms keyer timing engine.
* **3-Line Rolling Terminal Decoder & Prosign Engine**:
  * Pattern-based lookup engine covering **A–Z, 0–9, all standard punctuation (`.`, `,`, `?`, `/`, `=`, `-`, `:`, `;`, `(`, `)`, `'`, `"`, `@`, `+`, `_`)**, and **official amateur radio prosigns given without pause** (`<SK>`, `<AR>`, `<BT>`, `<AS>`, `<KN>`, `<KA>`, `<BK>`, `<VE>`, `<HH>`, `<SOS>`).
  * **Iambic Squeeze Tolerance Matching**: Automatic tolerance filters for trailing squeeze elements guarantee that period `.` (`.-.-.-`), comma `,` (`--..--`), equals/BT, and prosigns decode 100% reliably even with contact bounce or Mode B trailing alternation.
  * Continuous 3-line display with automatic line-wrapping and teletype-style upward scrolling.
  * Intelligent word-space detection ($5.0T$ threshold) with clear visual spacing.
* **Koch Method & QSO Abbreviation Training Mode**:
  * 40 progressive Koch sequence stages (`K M R S U A P T L O N W J E F 0 Y , V G 5 / Q 9 Z H 3 8 B ? 4 2 7 C 1 D 6 X`).
  * **QSO-Verkehrsabkürzungen & Spruchzeichen Training**: Practice authentic on-the-air amateur radio traffic:
    * **QSO-Abk.**: Common CW traffic abbreviations (`CQ`, `73`, `5NN`, `GM`, `GA`, `GE`, `GN`, `PSE`, `TNX`, `RST`, `NAME`, `HW?`, `WX`, `RIG`, `ANT`, `FB`, `RPT`, `AGN`, `CUL`, `BCNU`, `SKED`, `ES`, `DE`, `K`, `BK`).
    * **Spruchz.**: Prosigns sent without pause (`<SK>`, `<AR>`, `<BT>`, `<AS>`, `<KN>`, `<KA>`, `<BK>`, `<VE>`, `<HH>`, `<SOS>`).
    * **Q-Codes**: Frequently used Q-signals (`QTH`, `QSL`, `QSO`, `QRP`, `QRZ`, `QRM`, `QRN`, `QSY`, `QRQ`, `QRS`).
  * Multi-character keying validation with dynamic feedback (`RICHTIG!` / `FALSCH`) and multi-element Morse pattern guide.
* **Full-Screen Boot Splash Screen**:
  * Shows prominent **`DF3MT`** callsign (52px tall), website `https://df3mt.de`, repository URL `github.com/DF3MT/esp32-morse-trainer`, and subtitle at cold boot.
  * Held for 1.8 seconds, or skipped instantly on any paddle tap or knob interaction.
* **Intelligent Deep Sleep & Low-Power Standby**:
  * Automatically enters ESP32 ultra-low-power deep sleep after **2 minutes of inactivity** (configurable in menu: `Aus`, `1 Min`, `2 Min`, `5 Min`, `10 Min`).
  * Powers down ST7789 display controller (`SLPIN`) and turns off backlight to minimize battery drain (<15 µA).
  * Shows friendly standby notice ("DEEP SLEEP... Knopf drücken zum Aufwecken").
  * Instant hardware wakeup via Rotary Encoder push button (`PIN_ENC_SW` on RTC GPIO 17) with audio wake chirp.
* **Rotary Encoder Menu System (KY-040)**:
  * Fully configurable on the fly via rotary encoder turn & click.
  * Live parameter adjustments with instant audio/visual preview.
  * All settings permanently saved in ESP32 Flash (NVS Preferences).
* **High-Power Audio Engine**:
  * Configurable sidetone frequency (**400 Hz to 2.500 Hz** in 50-Hz steps).
  * 50.0% symmetrical square-wave PWM for maximum acoustic energy.
  * **6.6V BTL Push-Pull Power Boost**: Differential output across GPIO 22 and GPIO 21 provides **4× acoustic power (+6 dB)** without any extra components.
  * Dedicated LEDC hardware timer (Timer 1) isolated from the display backlight PWM.
* **FreeRTOS Dual-Core Architecture**:
  * **Core 0**: Ultra-low-latency 1 ms timing engine for paddle polling, keyer state machine, and decoder.
  * **Core 1**: Smooth 50+ FPS UI rendering using double-buffered full-screen DMA sprites (`TFT_eSprite`).

---

## 🛠️ Hardware Requirements

1. **Microcontroller**: [LilyGO T-Display v1.1](https://www.lilygo.cc/) (ESP32-D0WDQ6-V3, 1.14" ST7789 IPS 135×240 TFT).
2. **Morse Paddle**: Dual-lever iambic paddle connected via a standard 3.5 mm stereo jack (TRS).
3. **Control**: KY-040 rotary encoder module with integrated push button.
4. **Sidetone**: Passive piezo buzzer or mini speaker with driver.

---

## 🔌 Pinout & Wiring

![Wiring Diagram](docs/wiring_diagram_exact.png)

| Component | Pin / Signal | ESP32 GPIO | Description |
|---|---|---|---|
| **Rotary Encoder** | **DT** | **GPIO 25** | Phase B (Direction) |
| **Rotary Encoder** | **CLK** | **GPIO 26** | Phase A (Clock interrupt) |
| **Rotary Encoder** | **SW** | **GPIO 27** | Push Button (Menu / Select) |
| **Rotary Encoder** | **+ / VCC** | **3V3** | **3.3V ONLY** (Do not connect to 5V!) |
| **Rotary Encoder** | **GND** | **GND** | Ground rail |
| **Morse Paddle** | **Tip** | **GPIO 32** | DIT paddle contact (`INPUT_PULLUP`, active LOW) |
| **Morse Paddle** | **Ring** | **GPIO 33** | DAH paddle contact (`INPUT_PULLUP`, active LOW) |
| **Morse Paddle** | **Sleeve** | **GND** | Ground rail |
| **Buzzer / Audio** | **(+) Plus** | **GPIO 22** | Primary PWM Sidetone (LEDC Ch 2, Timer 1) |
| **Buzzer (Standard)**| **(-) Minus** | **GND** | 3.3V Peak-to-Peak output |
| **Buzzer (4× Boost)**| **(-) Minus** | **GPIO 21** | **6.6V BTL Push-Pull Boost** (180° inverted phase) |

> 📘 **Detailed Cabling Guide**: For full step-by-step schematics, pinout illustrations, and audio driver circuits (Piezo vs. 8Ω Speaker), see [docs/CABLING_GUIDE.md](docs/CABLING_GUIDE.md).

---

## 🚀 Building & Flashing

This project uses [PlatformIO](https://platformio.org/).

### Prerequisites
* [VS Code](https://code.visualstudio.com/) with the [PlatformIO IDE extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) **or** PlatformIO Core CLI (`pip install platformio`).

### Build & Upload via CLI
```bash
# Clone the repository
git clone https://github.com/DF3MT/esp32-morse-trainer.git
cd esp32-morse-trainer

# Compile firmware
python -m platformio run

# Flash to LilyGO T-Display (adjust COM port if necessary)
python -m platformio run -t upload --upload-port COM13
```

---

## 📖 Operation & Menu

### Main Screen (Decode Mode)
* **Turn Knob**: Directly adjusts speed from **3 to 40 WPM** in real-time.
* **Click Knob**: Opens the configuration menu.
* **Key with Paddles**: Decoded characters appear immediately and wrap across **3 full lines**, scrolling upward automatically.

### Configuration Menu
Navigate by turning the knob, click to edit/toggle, click again to confirm and save:

1. **Betriebsart**: Switch between `Decodieren` (free keying) and `Training` (practice mode).
2. **Tempo (WPM)**: Set keyer speed (3 .. 40 WPM).
3. **Auto-Tempo**: Toggle automatic speed estimation on/off.
4. **Keyer-Modus**: Select `Iambic Mode A` (Curtis standard) or `Iambic Mode B`.
5. **Paddle-Tausch**: Swap Dit/Dah paddles for left-handed operation.
6. **Tonhöhe**: Sidetone frequency (400 .. 2500 Hz, with live audio preview).
7. **Lautstärke**: PWM duty cycle / volume (0 .. 100%).
8. **Helligkeit**: Display backlight brightness (10 .. 100%).
9. **BLE-Tastatur**: Toggle Bluetooth Low Energy HID keyboard (`Ein` / `Aus`).
10. **BLE-Layout**: Select keyboard layout (`DE (QWERTZ)` for German systems, `US (QWERTY)` for US systems).
11. **Übungssatz**: Training set (`Koch-Methode`, `Nur Buchstaben`, `Nur Ziffern`, `Alle Zeichen`, `Q-Codes`, `QSO-Abk.`, `Spruchz.`).
12. **Koch-Stufe**: Progressive lesson level (Level 1 to 40).
13. **Auto-Standby**: Inactivity timeout before entering Deep Sleep (`Aus`, `1 Min`, `2 Min`, `5 Min`, `10 Min`). Default: 2 Min.
14. **< Zurueck**: Exit menu and persist all settings to Flash.

---

## 📄 License

This project is licensed under the [MIT License](LICENSE) — free for personal, educational, and amateur radio use.

Developed with 73 de **DF3MT**.

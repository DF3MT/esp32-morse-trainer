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
  * Transmits decoded Morse code characters and word spaces in real time directly into any text editor, web browser, contest logger (N1MM, Log4OM), or chat app.
  * Live status badge (`[BLE]`) on the screen header shows connection state (dim = advertising/waiting, bright cyan/blue = connected).
  * Toggleable via the configuration menu (`BLE-Tastatur: Ein / Aus`) and saved in NVS Flash.
  * Thread-safe FreeRTOS queue architecture: 0-wait Core 0 transmission ensures zero jitter on the 1 ms keyer timing engine.
* **3-Line Rolling Terminal Decoder**:
  * Real-time decoding using an expanded binary tree covering **A–Z, 0–9, and punctuation (`/`, `?`, `.`, `,`)**.
  * Continuous 3-line display with automatic line-wrapping and teletype-style upward scrolling.
  * Intelligent word-space detection ($5.0T$ threshold) with clear visual spacing.
* **Koch Method Training Mode**:
  * 40 progressive Koch sequence stages (`K M R S U A P T L O N W J E F 0 Y , V G 5 / Q 9 Z H 3 8 B ? 4 2 7 C 1 D 6 X`).
  * Big, bold 52-pixel target character display with live visual Morse pattern guide.
  * Instant feedback (`RICHTIG!` / `FALSCH`) with automatic advancement to the next challenge.
  * Customizable training sets: Koch Sequence, Letters only, Numbers only, All characters, or Q-Codes.
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
10. **Übungssatz**: Training set (`Koch-Methode`, `Nur Buchstaben`, `Nur Ziffern`, `Alle Zeichen`, `Q-Codes`).
11. **Koch-Stufe**: Progressive lesson level (Level 1 to 40).
12. **< Zurueck**: Exit menu and persist all settings to Flash.

---

## 📄 License

This project is licensed under the [MIT License](LICENSE) — free for personal, educational, and amateur radio use.

Developed with 73 de **DF3MT**.

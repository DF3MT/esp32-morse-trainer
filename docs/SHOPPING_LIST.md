# 🛒 Shopping List: ESP32 Morse Trainer

**🇬🇧 English** · [🇩🇪 Deutsch](SHOPPING_LIST.de.md) · [⬅ Back to README](../README.md) · [🔌 Wiring guide](CABLING_GUIDE.md)

All parts are available on AliExpress. The links open an **AliExpress search**, so you can choose the seller, price and shipping yourself. Prices are rough guide values and change often.

---

## Required parts

| # | Qty | Part | What to look for | Approx. price | AliExpress |
|---|---|---|---|---|---|
| 1 | 1 | **LilyGO T-Display ESP32 (v1.1)** | 1.14" ST7789 display, 4 MB or 16 MB flash, USB-C (CH9102). Buy from the official *LILYGO Official Store*. | 10–15 € | [Search](https://www.aliexpress.com/w/wholesale-lilygo-t-display-esp32.html) |
| 2 | 1 | **KY-040 rotary encoder module** | Module with push button and 5 pins (CLK, DT, SW, +, GND). Packs of 3–5 are often cheaper. | 1–2 € | [Search](https://www.aliexpress.com/w/wholesale-ky-040-rotary-encoder-module.html) |
| 3 | 1 | **Passive piezo buzzer** | Must be **passive** (no built-in oscillator), 3.3–5 V, e.g. 12 mm. | 1 € | [Search](https://www.aliexpress.com/w/wholesale-passive-buzzer-3v.html) |
| 4 | 1 | **3.5 mm stereo jack socket** | Panel-mount socket, 3-pole (TRS), e.g. PJ-392 or PJ-307. For connecting the paddle. | 1 € | [Search](https://www.aliexpress.com/w/wholesale-3.5mm-stereo-jack-socket-panel-mount.html) |
| 5 | 1 set | **Dupont jumper wires** | Female-female and female-male, 10–20 cm. | 1–3 € | [Search](https://www.aliexpress.com/w/wholesale-dupont-jumper-wire.html) |
| 6 | 1 | **USB-C data cable** | Must carry data, not a charge-only cable. | 2 € | [Search](https://www.aliexpress.com/w/wholesale-usb-c-data-cable.html) |

**Paddle:** if you don't have one yet, you also need an **iambic paddle** (dual-lever) with a 3.5 mm stereo plug.

| # | Qty | Part | What to look for | Approx. price | AliExpress |
|---|---|---|---|---|---|
| 7 | 1 | **Iambic Morse paddle** | Dual-lever (iambic), 3.5 mm stereo plug or cable. Single-lever "straight keys" do **not** work with the keyer. | 15–40 € | [Search](https://www.aliexpress.com/w/wholesale-iambic-paddle-morse-key.html) |

---

## Optional parts

| # | Qty | Part | Purpose | Approx. price | AliExpress |
|---|---|---|---|---|---|
| 8 | 1 | **LiPo battery 3.7 V with JST 1.25 mm 2-pin plug** | Battery operation. The T-Display has a charging circuit and battery connector. Check the polarity before plugging in! | 3–6 € | [Search](https://www.aliexpress.com/w/wholesale-3.7v-lipo-battery-jst-1.25.html) |
| 9 | 2 | **100 nF ceramic capacitors** | Debouncing very hard paddle contacts | < 1 € | [Search](https://www.aliexpress.com/w/wholesale-100nf-ceramic-capacitor.html) |
| 10 | 1 | **PAM8403 amplifier module** | Louder sidetone through a speaker | 1 € | [Search](https://www.aliexpress.com/w/wholesale-pam8403-amplifier-module.html) |
| 11 | 1 | **Small speaker 8 Ω, 0.5–2 W** | Instead of the piezo, only with amplifier or transistor driver | 1–2 € | [Search](https://www.aliexpress.com/w/wholesale-mini-speaker-8-ohm.html) |
| 12 | 1 + 1 | **NPN transistor (2N2222 or BC547) + 1 kΩ resistor** | Simple speaker driver (see wiring guide) | < 1 € | [Search](https://www.aliexpress.com/w/wholesale-2n2222-transistor.html) |
| 13 | 1 | **Project box** | Enclosure, approx. 80 × 50 × 25 mm or larger | 2–4 € | [Search](https://www.aliexpress.com/w/wholesale-plastic-project-box-enclosure.html) |
| 14 | 1 | **Rotary knob for 6 mm shaft** | Nicer feel on the encoder | < 1 € | [Search](https://www.aliexpress.com/w/wholesale-rotary-knob-6mm.html) |
| 15 | 1 | **Pin headers 2.54 mm** | The T-Display often ships with unsoldered headers | < 1 € | [Search](https://www.aliexpress.com/w/wholesale-2.54mm-pin-header.html) |

---

## Tools

* Soldering iron and solder (for headers and the jack socket)
* Small screwdriver, side cutters
* Optional: multimeter for checking the paddle contacts and battery polarity

---

## Cost overview

| Build | Approx. total |
|---|---|
| Basic build (parts 1–6, own paddle) | **about 16–24 €** |
| With paddle (parts 1–7) | **about 31–64 €** |
| With battery and enclosure | **plus about 5–10 €** |

> Tip: AliExpress delivery often takes 1–3 weeks. Order cheap small parts (encoder, buzzer, jack, wires) in packs. They are hardly more expensive and leave you spares.

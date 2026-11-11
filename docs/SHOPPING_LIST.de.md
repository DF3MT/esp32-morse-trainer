# 🛒 Einkaufsliste: ESP32 Morse-Trainer

[🇬🇧 English](SHOPPING_LIST.md) · **🇩🇪 Deutsch** · [⬅ Zurück zum README](../README.de.md) · [🔌 Verkabelung](CABLING_GUIDE.de.md)

Alle Teile gibt es bei AliExpress. Die Links öffnen eine **AliExpress-Suche**, so dass du Händler, Preis und Versand selbst auswählen kannst. Die Preise sind grobe Richtwerte und ändern sich häufig.

---

## Benötigte Teile

| # | Anz. | Teil | Darauf achten | Preis ca. | AliExpress |
|---|---|---|---|---|---|
| 1 | 1 | **LilyGO T-Display ESP32 (v1.1)** | 1,14"-ST7789-Display, 4 MB oder 16 MB Flash, USB-C (CH9102). Am besten beim offiziellen *LILYGO Official Store* kaufen. | 10–15 € | [Suche](https://www.aliexpress.com/w/wholesale-lilygo-t-display-esp32.html) |
| 2 | 1 | **KY-040-Drehencoder-Modul** | Modul mit Taster und 5 Pins (CLK, DT, SW, +, GND). 3er- oder 5er-Packs sind oft günstiger. | 1–2 € | [Suche](https://www.aliexpress.com/w/wholesale-ky-040-rotary-encoder-module.html) |
| 3 | 1 | **Passiver Piezo-Summer** | Muss **passiv** sein (ohne eingebauten Oszillator), 3,3–5 V, z. B. 12 mm. | 1 € | [Suche](https://www.aliexpress.com/w/wholesale-passive-buzzer-3v.html) |
| 4 | 1 | **3,5-mm-Stereo-Klinkenbuchse** | Einbaubuchse, 3-polig (TRS), z. B. PJ-392 oder PJ-307. Für den Anschluss des Paddles. | 1 € | [Suche](https://www.aliexpress.com/w/wholesale-3.5mm-stereo-jack-socket-panel-mount.html) |
| 5 | 1 Set | **Dupont-Steckkabel** | Buchse-Buchse und Buchse-Stecker, 10–20 cm. | 1–3 € | [Suche](https://www.aliexpress.com/w/wholesale-dupont-jumper-wire.html) |
| 6 | 1 | **USB-C-Datenkabel** | Muss Daten übertragen, kein reines Ladekabel. | 2 € | [Suche](https://www.aliexpress.com/w/wholesale-usb-c-data-cable.html) |

**Paddle:** Wer noch keins hat, braucht zusätzlich ein **Iambic-Paddle** (Doppelhebel) mit 3,5-mm-Stereoklinke.

| # | Anz. | Teil | Darauf achten | Preis ca. | AliExpress |
|---|---|---|---|---|---|
| 7 | 1 | **Iambic-Morsepaddle** | Doppelhebel (iambic), 3,5-mm-Stereostecker oder -kabel. Einhebel-„Handtasten“ funktionieren mit dem Keyer **nicht**. | 15–40 € | [Suche](https://www.aliexpress.com/w/wholesale-iambic-paddle-morse-key.html) |

---

## Optionale Teile

| # | Anz. | Teil | Wozu | Preis ca. | AliExpress |
|---|---|---|---|---|---|
| 8 | 1 | **LiPo-Akku 3,7 V mit JST-1,25-mm-Stecker (2-polig)** | Akkubetrieb. Das T-Display hat Ladeschaltung und Akku-Anschluss. Vor dem Einstecken die Polung prüfen! | 3–6 € | [Suche](https://www.aliexpress.com/w/wholesale-3.7v-lipo-battery-jst-1.25.html) |
| 9 | 2 | **Keramikkondensatoren 100 nF** | Entprellen sehr harter Paddle-Kontakte | < 1 € | [Suche](https://www.aliexpress.com/w/wholesale-100nf-ceramic-capacitor.html) |
| 10 | 1 | **Verstärkermodul PAM8403** | Lauterer Mithörton über Lautsprecher | 1 € | [Suche](https://www.aliexpress.com/w/wholesale-pam8403-amplifier-module.html) |
| 11 | 1 | **Kleinlautsprecher 8 Ω, 0,5–2 W** | Statt Piezo, nur mit Verstärker oder Transistortreiber | 1–2 € | [Suche](https://www.aliexpress.com/w/wholesale-mini-speaker-8-ohm.html) |
| 12 | 1 + 1 | **NPN-Transistor (2N2222 oder BC547) + Widerstand 1 kΩ** | Einfacher Lautsprechertreiber (siehe Verkabelungsanleitung) | < 1 € | [Suche](https://www.aliexpress.com/w/wholesale-2n2222-transistor.html) |
| 13 | 1 | **Kunststoffgehäuse** | Gehäuse, ca. 80 × 50 × 25 mm oder größer | 2–4 € | [Suche](https://www.aliexpress.com/w/wholesale-plastic-project-box-enclosure.html) |
| 14 | 1 | **Drehknopf für 6-mm-Achse** | Angenehmere Bedienung des Encoders | < 1 € | [Suche](https://www.aliexpress.com/w/wholesale-rotary-knob-6mm.html) |
| 15 | 1 | **Stiftleisten 2,54 mm** | Das T-Display wird oft mit unverlöteten Stiftleisten geliefert | < 1 € | [Suche](https://www.aliexpress.com/w/wholesale-2.54mm-pin-header.html) |

---

## Werkzeug

* Lötkolben und Lötzinn (für Stiftleisten und Klinkenbuchse)
* Kleiner Schraubendreher, Seitenschneider
* Optional: Multimeter zum Prüfen der Paddle-Kontakte und der Akku-Polung

---

## Kostenübersicht

| Aufbau | Summe ca. |
|---|---|
| Grundaufbau (Teile 1–6, eigenes Paddle) | **ca. 16–24 €** |
| Mit Paddle (Teile 1–7) | **ca. 31–64 €** |
| Mit Akku und Gehäuse | **zzgl. ca. 5–10 €** |

> Tipp: Die Lieferung von AliExpress dauert oft 1–3 Wochen. Günstige Kleinteile (Encoder, Summer, Buchse, Kabel) am besten im Pack bestellen. Das kostet kaum mehr, und du hast Ersatz.

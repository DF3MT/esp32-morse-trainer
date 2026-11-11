#include "sidetone.h"
#include "esp32-hal-matrix.h"
#include "soc/gpio_sig_map.h"

// PIN_BUZZER_A is GPIO 22.
// PIN_BUZZER_B is GPIO 21 (directly adjacent on the LilyGO header!).
// In BTL (Bridge-Tied Load) mode:
// Connecting Buzzer(+) to GPIO 22 and Buzzer(-) to GPIO 21 swings 6.6V Peak-to-Peak (4x power / +6 dB).
// If Buzzer(-) is left on GND, it still operates normally at 3.3V.
static const uint8_t PIN_BUZZER_A = 22;
static const uint8_t PIN_BUZZER_B = 21;
static uint16_t _freq = 700;
static uint16_t _duty = 2048; // 12-bit: 2048 = 50% square wave
static uint8_t  _volumePercent = 100;
static const uint8_t BUZZER_CHANNEL = 2; // Channel 2 -> Timer 1 (isolated from Backlight Channel 0 -> Timer 0)
static const uint8_t BUZZER_RESOLUTION = 12; // 12-bit (0..4095) for ultra-fine microsecond pulse control

namespace Sidetone {

void begin(uint8_t pin, uint16_t freqHz) {
    _freq = freqHz;
    pinMode(PIN_BUZZER_A, OUTPUT);
    digitalWrite(PIN_BUZZER_A, LOW);
    pinMode(PIN_BUZZER_B, OUTPUT);
    digitalWrite(PIN_BUZZER_B, LOW);
    ledcSetup(BUZZER_CHANNEL, _freq, BUZZER_RESOLUTION);
    setVolume(100);
    off();
}

void on() {
    if (_duty > 0) {
        ledcAttachPin(PIN_BUZZER_A, BUZZER_CHANNEL);
        pinMatrixOutAttach(PIN_BUZZER_B, LEDC_HS_SIG_OUT2_IDX, true, false);
        ledcWrite(BUZZER_CHANNEL, _duty);
    }
}

void off() {
    ledcWrite(BUZZER_CHANNEL, 0);
    ledcDetachPin(PIN_BUZZER_A);
    pinMatrixOutDetach(PIN_BUZZER_B, false, false);
    pinMode(PIN_BUZZER_A, OUTPUT);
    digitalWrite(PIN_BUZZER_A, LOW);
    pinMode(PIN_BUZZER_B, OUTPUT);
    digitalWrite(PIN_BUZZER_B, LOW);
}

void setFrequency(uint16_t freqHz) {
    if (freqHz < 200) freqHz = 200;
    if (freqHz > 3000) freqHz = 3000;
    _freq = freqHz;
    ledcSetup(BUZZER_CHANNEL, _freq, BUZZER_RESOLUTION);
    if (_duty > 0) {
        ledcWrite(BUZZER_CHANNEL, _duty);
    }
}

uint16_t getFrequency() {
    return _freq;
}

void setVolume(uint8_t percent) {
    if (percent > 100) percent = 100;
    _volumePercent = percent;

    if (percent == 0) {
        _duty = 0;
        return;
    }

    // 12-bit LEDC: 50% max square wave = 2048 (at 800 Hz, period is 1250 µs, 1 tick = ~0.305 µs).
    // Calibrated perceptual exponential curve:
    // Narrow microsecond pulse widths at lower percentages (< 40%) restrict crystal deflection energy.
    static const uint16_t DUTY_TABLE[11] = {
        0,     // 0%   - Mute
        4,     // 10%  - ~1.2 us pulse (whisper quiet)
        14,    // 20%  - ~4.3 us pulse (quiet)
        36,    // 30%  - ~11 us pulse (moderate)
        90,    // 40%  - ~27 us pulse
        200,   // 50%  - ~61 us pulse
        420,   // 60%  - ~128 us pulse
        750,   // 70%  - ~229 us pulse
        1150,  // 80%  - ~350 us pulse
        1600,  // 90%  - ~488 us pulse
        2048   // 100% - 625 us pulse (50.0% pure symmetric square wave)
    };

    uint8_t idx = percent / 10;
    uint8_t rem = percent % 10;
    if (idx >= 10) {
        _duty = DUTY_TABLE[10];
    } else {
        uint16_t d0 = DUTY_TABLE[idx];
        uint16_t d1 = DUTY_TABLE[idx + 1];
        _duty = d0 + ((d1 - d0) * rem) / 10;
    }
}

uint8_t getVolume() {
    return _volumePercent;
}

void playBeep(uint16_t durationMs) {
    on();
    delay(durationMs);
    off();
}

} // namespace Sidetone

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
static uint16_t _freq = 800;
static uint8_t  _duty = 128; // 128 = 50% square wave (maximum fundamental acoustic power)
static const uint8_t BUZZER_CHANNEL = 2; // Channel 2 -> Timer 1 (isolated from Backlight Channel 0 -> Timer 0)
static const uint8_t BUZZER_RESOLUTION = 8;

namespace Sidetone {

void begin(uint8_t pin, uint16_t freqHz) {
    _freq = freqHz;
    _duty = 128;
    pinMode(PIN_BUZZER_A, OUTPUT);
    digitalWrite(PIN_BUZZER_A, LOW);
    pinMode(PIN_BUZZER_B, OUTPUT);
    digitalWrite(PIN_BUZZER_B, LOW);
    ledcSetup(BUZZER_CHANNEL, _freq, BUZZER_RESOLUTION);
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
}

uint16_t getFrequency() {
    return _freq;
}

void setVolume(uint8_t duty) {
    _duty = duty;
}

uint8_t getVolume() {
    return _duty;
}

void playBeep(uint16_t durationMs) {
    on();
    delay(durationMs);
    off();
}

} // namespace Sidetone

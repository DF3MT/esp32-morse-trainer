#pragma once
#include <Arduino.h>

// Passive piezo buzzer / speaker control via ESP32 LEDC (tone generation)
namespace Sidetone {
    void     begin(uint8_t pin, uint16_t freqHz = 700);
    void     on();
    void     off();
    void     setFrequency(uint16_t freqHz);
    uint16_t getFrequency();
    void     setVolume(uint8_t duty);   // 0–255 (128 = 50% max square wave)
    uint8_t  getVolume();
    void     playBeep(uint16_t durationMs = 40);
}

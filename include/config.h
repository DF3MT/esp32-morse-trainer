#pragma once

// ============================================================
// GPIO Assignments — LilyGO T-Display v1.1
// ============================================================
#define PIN_DIT      32   // Paddle dit contact (INPUT_PULLUP, active LOW)
#define PIN_DAH      33   // Paddle dah contact (INPUT_PULLUP, active LOW)
#define PIN_ENC_CLK  26   // Rotary encoder CLK/A
#define PIN_ENC_DT   25   // Rotary encoder DT/B
#define PIN_ENC_SW   27   // Rotary encoder push button (INPUT_PULLUP, active LOW)
#define PIN_BUZZER   22   // Passive piezo buzzer (LEDC PWM)
#define PIN_TFT_BL    4   // TFT backlight (drive HIGH)

// ============================================================
// Sidetone
// ============================================================
#define SIDETONE_HZ     700   // Standard ham sidetone frequency
#define SIDETONE_DUTY   128   // 0-255 (128 = 50% = full volume)

// ============================================================
// Keyer defaults
// ============================================================
#define WPM_MIN        3   // Range down to 3 WPM for slow practice
#define WPM_MAX       40   // Range up to 40 WPM for high speed
#define WPM_DEFAULT   15   // Start lower at 15 WPM (comfortable training baseline)

// ============================================================
// Auto-speed detection
// ============================================================
#define AUTOSPEED_ALPHA       0.15f   // EMA weight for speed tracking
#define AUTOSPEED_GATE_LOW    0.50f   // Reject samples < 50% of current T
#define AUTOSPEED_GATE_HIGH   3.50f   // Reject samples > 350% of current T
#define AUTOSPEED_HYST        0.05f   // 5% dead-band hysteresis at latch
#define AUTOSPEED_MIN_ELEMS   8       // Elements before auto-speed is trusted

// ============================================================
// Display
// ============================================================
#define DISPLAY_BRIGHTNESS  200   // Backlight PWM 0-255
#define DECODED_CHARS_MAX   128   // Extended buffer for multi-line scrolling (holds 8+ lines of history)

// ============================================================
// Power Management & Deep Sleep
// ============================================================
#define SLEEP_TIMEOUT_DEFAULT_MIN   2      // 2 minutes inactivity timeout

// ============================================================
// FreeRTOS task stack sizes
// ============================================================
#define KEYER_TASK_STACK   4096
#define UI_TASK_STACK      16384

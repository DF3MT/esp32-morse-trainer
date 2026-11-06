#pragma once
#include <Arduino.h>

// ============================================================
// IambicKeyer — Curtis Mode A / Mode B iambic electronic keyer
//
// Run update() every 1ms from a FreeRTOS task (Core 0).
// ============================================================

enum class IambicMode { MODE_A, MODE_B };
enum class KeyerElement { NONE, DIT, DAH };

class IambicKeyer {
public:
    using KeyCallback  = void (*)(bool keyDown);
    using ElemCallback = void (*)(KeyerElement e);

    IambicKeyer(IambicMode mode, int wpm);

    // Called every 1ms from keyer task
    void update();

    // Configuration
    void setWpm(int wpm);
    void setMode(IambicMode mode);
    void setAutoSpeed(bool enabled);
    void setPaddleSwap(bool swap) { _paddleSwap = swap; }

    // Getters
    int         getWpm()           const;
    float       getT()             const;
    bool        isAutoSpeed()      const;
    bool        isAutoSpeedReady() const;
    IambicMode  getMode()          const;
    bool        isKeyDown()        const;
    bool        isPaddleSwapped()  const { return _paddleSwap; }

    // Register callbacks
    void onKey(KeyCallback cb)      { _keyCb  = cb; }
    void onElement(ElemCallback cb) { _elemCb = cb; }

private:
    enum class State {
        IDLE,
        SENDING_DIT,
        DIT_SPACE,
        SENDING_DAH,
        DAH_SPACE
    };

    IambicMode _mode;
    State      _state      = State::IDLE;
    bool       _keyDown    = false;

    // Timing (ms)
    float          _activeT;
    float          _pendingT;
    unsigned long  _stateStart = 0;

    // Paddle memory
    bool          _paddleSwap       = false;
    volatile bool _ditMem           = false;
    volatile bool _dahMem           = false;
    bool          _lastWasDah       = false;
    bool          _modeB_extraFired = false;

    // Continuous hold vs tap detection
    bool          _ditReleased      = true;
    bool          _dahReleased      = true;

    // Paddle debounce state (member variables for instance isolation)
    unsigned long _ditLastChange    = 0;
    bool          _ditStable        = false;
    unsigned long _dahLastChange    = 0;
    bool          _dahStable        = false;

    // Auto-speed flag (informational, does NOT modify active keyer timing)
    bool          _autoSpeed        = false;
    bool          _autoReady        = false;

    // Callbacks
    KeyCallback  _keyCb  = nullptr;
    ElemCallback _elemCb = nullptr;

    // Internal helpers
    bool readDit();
    bool readDah();
    void keyOn();
    void keyOff();
    void startDit();
    void startDah();
    void decideNext();

    int   wpmFromT(float t_ms) const { return (int)(1200.0f / t_ms + 0.5f); }
    float tFromWpm(int wpm)    const { return 1200.0f / (float)wpm; }
};

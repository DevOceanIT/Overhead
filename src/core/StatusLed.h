#pragma once
// core/StatusLed — the CYD's rear RGB LED as a quiet alert light (pins 4/16/17,
// active LOW, LEDC PWM). The Director picks the mode about once a second; tick()
// animates it. Off unless something is worth noticing: no idle heartbeat.
// Settings: ledEnabled (Health toggle), ledDayMax / ledNightMax (peak 0-255,
// night = sun below the horizon), ledQuietStart / ledQuietEnd (local hours).
#include <Arduino.h>

class Settings;

class StatusLed {
public:
  // Lowest to highest priority.
  enum class Mode : uint8_t { Off, Aurora, Overhead, Pass, Launch, Emergency };

  void begin(Settings* s);
  void set(Mode m, bool night) { _mode = m; _night = night; }
  void test(Mode m, uint32_t ms) { _testMode = m; _testUntil = millis() + ms; }   // serial bench test
  void tick(uint32_t nowMs);
  static const char* name(Mode m);

private:
  void write(uint8_t r, uint8_t g, uint8_t b);

  Settings* _s = nullptr;
  bool      _ok = false;
  Mode      _mode = Mode::Off;
  bool      _night = false;
  Mode      _testMode = Mode::Off;
  uint32_t  _testUntil = 0;
  uint32_t  _lastMs = 0;
  uint8_t   _last[3] = {1, 1, 1};     // force the first write
};

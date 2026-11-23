#include "StatusLed.h"
#include "../services/Settings.h"
#include <math.h>

#if defined(BOARD_CYD28_ILI9341)
// ESP32-2432S028R rear RGB LED. LEDC channels 3-5 (backlight uses 7, Morse buzzer 6).
static const int kPin[3] = {4, 16, 17};
static const int kCh[3]  = {3, 4, 5};
#endif

void StatusLed::begin(Settings* s) {
  _s = s;
#if defined(BOARD_CYD28_ILI9341)
  for (int i = 0; i < 3; ++i) {
    ledcSetup(kCh[i], 5000, 8);
    ledcAttachPin(kPin[i], kCh[i]);
  }
  _ok = true;
  write(0, 0, 0);
#endif
}

const char* StatusLed::name(Mode m) {
  switch (m) {
    case Mode::Aurora:    return "aurora";
    case Mode::Overhead:  return "overhead";
    case Mode::Pass:      return "pass";
    case Mode::Launch:    return "launch";
    case Mode::Emergency: return "emergency";
    default:              return "off";
  }
}

void StatusLed::write(uint8_t r, uint8_t g, uint8_t b) {
#if defined(BOARD_CYD28_ILI9341)
  const uint8_t v[3] = {r, g, b};
  for (int i = 0; i < 3; ++i) {
    if (v[i] == _last[i]) continue;
    ledcWrite(kCh[i], 255 - v[i]);          // active LOW
    _last[i] = v[i];
  }
#endif
}

void StatusLed::tick(uint32_t nowMs) {
  if (!_ok || nowMs - _lastMs < 20) return;  // ~50 fps is plenty for a breathing LED
  _lastMs = nowMs;

  bool testing = (int32_t)(_testUntil - nowMs) > 0;
  Mode m = testing ? _testMode : (_s->getBool("ledEnabled", true) ? _mode : Mode::Off);
  if (m == Mode::Off) { write(0, 0, 0); return; }

  float peak = (float)(_night ? _s->getInt("ledNightMax", 8) : _s->getInt("ledDayMax", 160));
  float lv = 0, r = 0, g = 0, b = 0;
  switch (m) {
    case Mode::Emergency:                    // red, fast flash
      lv = ((nowMs / 125) & 1) ? 1.0f : 0.0f; r = 1; break;
    case Mode::Launch: {                     // purple, 1 s pulse
      float p = (nowMs % 1000) / 1000.0f;
      lv = p < 0.5f ? p * 2 : (1 - p) * 2;   r = 0.6f; b = 1; break;
    }
    case Mode::Pass:                         // blue, slow 4 s breathing
      lv = 0.5f - 0.5f * cosf((nowMs % 4000) * (2.0f * (float)M_PI / 4000.0f)); b = 1; break;
    case Mode::Overhead:                     // short white blink every 2 s
      lv = (nowMs % 2000) < 150 ? 1.0f : 0.0f; r = g = b = 1; break;
    case Mode::Aurora:                       // steady soft green
      lv = 0.6f; g = 1; break;
    default: break;
  }
  lv *= lv;                                  // rough perceptual curve for the fades
  write((uint8_t)lroundf(r * lv * peak), (uint8_t)lroundf(g * lv * peak), (uint8_t)lroundf(b * lv * peak));
}

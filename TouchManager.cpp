#include "TouchManager.h"
#include "Config.h"

namespace {
constexpr uint8_t BRIGHTNESS_LEVELS[] = {32, 80, 150, 220, 255};
constexpr uint8_t BRIGHTNESS_LEVEL_COUNT =
    sizeof(BRIGHTNESS_LEVELS) / sizeof(BRIGHTNESS_LEVELS[0]);
constexpr uint8_t DEFAULT_BRIGHTNESS_INDEX = 3;
constexpr uint32_t LONG_PRESS_MS = 2000UL;
constexpr uint32_t TAP_DEBOUNCE_MS = 40UL;
constexpr uint32_t RELEASE_GRACE_MS = 150UL;
constexpr uint32_t BACKLIGHT_PWM_FREQUENCY = 5000UL;
constexpr uint8_t BACKLIGHT_PWM_RESOLUTION = 8;
}

TouchManager::TouchManager(TFT_eSPI &tft, PageManager &pages)
  : _tft(tft),
    _pages(pages),
    _lastActivityMs(0),
    _pressStartMs(0),
    _lastTouchMs(0),
    _lastX(0),
    _lastY(0),
    _brightnessIndex(DEFAULT_BRIGHTNESS_INDEX),
    _screenAwake(true),
    _pressActive(false),
    _longPressHandled(false),
    _waitForWakeRelease(false) {}

void TouchManager::begin() {
  // TFT_eSPI configures TFT_BL during tft.init(), so PWM must be attached
  // afterwards. DisplayManager::begin() has already called tft.init().
  if (!ledcAttach(TFT_BL, BACKLIGHT_PWM_FREQUENCY, BACKLIGHT_PWM_RESOLUTION)) {
#if DEV_MODE
    Serial.println("ERROR: could not attach backlight PWM");
#endif
  }

  setBacklight(BRIGHTNESS_LEVELS[_brightnessIndex]);
  _lastActivityMs = millis();
}

bool TouchManager::isScreenAwake() const {
  return _screenAwake;
}

void TouchManager::update() {
  const uint32_t now = millis();

  if (_screenAwake && !_pressActive &&
      now - _lastActivityMs >= SCREEN_SLEEP_TIMEOUT_MS) {
    sleepScreen();
  }

  int16_t x = 0;
  int16_t y = 0;
  const bool touched = readScreenPoint(x, y);

  if (!_screenAwake) {
    // A touch that was already present when sleep began must be released before
    // another touch is allowed to wake the display.
    if (_waitForWakeRelease) {
      if (!touched) {
        _waitForWakeRelease = false;
#if DEV_MODE
        Serial.println("Wake touch armed");
#endif
      }
      return;
    }

    if (touched) {
      wakeScreen(now);
      _waitForWakeRelease = true;
    }
    return;
  }

  if (touched) {
    _lastTouchMs = now;
    _lastX = x;
    _lastY = y;
    _lastActivityMs = now;

    if (!_pressActive) {
      _pressActive = true;
      _pressStartMs = now;
      _longPressHandled = false;
    }

    if (!_longPressHandled && now - _pressStartMs >= LONG_PRESS_MS) {
      increaseBrightness();
      _longPressHandled = true;
    }
    return;
  }

  if (_pressActive) {
    // Resistive touch can briefly drop out while held. Do not finish the press
    // until it has remained released for the grace period.
    if (now - _lastTouchMs < RELEASE_GRACE_MS) return;

    const uint32_t pressDuration = _lastTouchMs - _pressStartMs;

    if (!_longPressHandled && pressDuration >= TAP_DEBOUNCE_MS) {
      _pages.handleTouch(_lastX, _lastY);
#if DEV_MODE
      Serial.printf("Touch: x=%d y=%d\n", _lastX, _lastY);
#endif
    }

    _pressActive = false;
    _longPressHandled = false;
  }
}

bool TouchManager::readScreenPoint(int16_t &x, int16_t &y) {
  uint16_t touchX = 0;
  uint16_t touchY = 0;
  if (!_tft.getTouch(&touchX, &touchY)) return false;

#if TOUCH_MIRROR_X
  touchX = (_tft.width() - 1) - touchX;
#endif

  x = constrain(static_cast<int16_t>(touchX), 0, _tft.width() - 1);
  y = constrain(static_cast<int16_t>(touchY), 0, _tft.height() - 1);
  return true;
}

void TouchManager::setBacklight(uint8_t pwm) {
  ledcWrite(TFT_BL, pwm);
}

void TouchManager::wakeScreen(uint32_t now) {
  setBacklight(BRIGHTNESS_LEVELS[_brightnessIndex]);
  _screenAwake = true;
  _lastActivityMs = now;
  _pressActive = false;
  _longPressHandled = false;
  _pages.requestRedraw();

#if DEV_MODE
  Serial.println("Screen awake");
#endif
}

void TouchManager::sleepScreen() {
  _screenAwake = false;
  _pressActive = false;
  _longPressHandled = false;
  _waitForWakeRelease = false;

  // Stop page rendering in the main loop, paint black once, then turn the LED
  // backlight fully off using the proven GPIO27 PWM control.
  _tft.fillScreen(TFT_BLACK);
  delay(25);
  setBacklight(0);

#if DEV_MODE
  Serial.println("Screen asleep");
#endif
}

void TouchManager::increaseBrightness() {
  _brightnessIndex = (_brightnessIndex + 1) % BRIGHTNESS_LEVEL_COUNT;
  setBacklight(BRIGHTNESS_LEVELS[_brightnessIndex]);
  _lastActivityMs = millis();

#if DEV_MODE
  const uint8_t percent = static_cast<uint8_t>(
      (BRIGHTNESS_LEVELS[_brightnessIndex] * 100UL) / 255UL);
  Serial.printf("Brightness: %u%%\n", percent);
#endif
}

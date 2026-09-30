#pragma once
#include <TFT_eSPI.h>
#include "PageManager.h"

class TouchManager {
public:
  TouchManager(TFT_eSPI &tft, PageManager &pages);
  void begin();
  void update();
  bool isScreenAwake() const;

private:
  TFT_eSPI &_tft;
  PageManager &_pages;

  uint32_t _lastActivityMs;
  uint32_t _pressStartMs;
  uint32_t _lastTouchMs;
  int16_t _lastX;
  int16_t _lastY;

  uint8_t _brightnessIndex;
  bool _screenAwake;
  bool _pressActive;
  bool _longPressHandled;
  bool _waitForWakeRelease;

  bool readScreenPoint(int16_t &x, int16_t &y);
  void setBacklight(uint8_t pwm);
  void wakeScreen(uint32_t now);
  void sleepScreen();
  void increaseBrightness();
};

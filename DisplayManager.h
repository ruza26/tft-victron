#pragma once
#include <TFT_eSPI.h>

class DisplayManager {
public:
  explicit DisplayManager(TFT_eSPI &display);
  void begin();
  void drawSplash();
  TFT_eSPI &screen();
  int16_t width() const;
  int16_t height() const;

private:
  TFT_eSPI &_tft;
};

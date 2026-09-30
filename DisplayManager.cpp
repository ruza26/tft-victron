#include "DisplayManager.h"
#include "Config.h"
#include "Theme.h"

DisplayManager::DisplayManager(TFT_eSPI &display) : _tft(display) {}

void DisplayManager::begin() {
  _tft.init();
  _tft.setRotation(DISPLAY_ROTATION);
  _tft.invertDisplay(true);
  _tft.fillScreen(Theme::background);
  _tft.setTextDatum(TL_DATUM);
}

void DisplayManager::drawSplash() {
  const int16_t cx = _tft.width() / 2;
  const int16_t cardW = (_tft.width() - 40 < 400) ? (_tft.width() - 40) : 400;
  const int16_t cardH = 150;
  const int16_t cardX = cx - cardW / 2;
  const int16_t cardY = (_tft.height() - cardH) / 2;

  _tft.fillScreen(Theme::background);
  _tft.fillRoundRect(cardX, cardY, cardW, cardH, 12, Theme::card);
  _tft.drawRoundRect(cardX, cardY, cardW, cardH, 12, Theme::cardBorder);

  _tft.setTextDatum(MC_DATUM);
  _tft.setTextColor(Theme::text, Theme::card);
  _tft.drawString("CampMonitor", cx, cardY + 40, 4);

  _tft.setTextColor(Theme::navActive, Theme::card);
  _tft.drawString("Version " APP_VERSION, cx, cardY + 82, 2);

  _tft.setTextColor(Theme::textMuted, Theme::card);
  _tft.drawString("Initialising Victron BLE...", cx, cardY + 119, 2);
  _tft.setTextDatum(TL_DATUM);
}

TFT_eSPI &DisplayManager::screen() { return _tft; }
int16_t DisplayManager::width() const { return _tft.width(); }
int16_t DisplayManager::height() const { return _tft.height(); }

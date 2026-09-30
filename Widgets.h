#pragma once
#include <TFT_eSPI.h>
#include "DataModel.h"

namespace Widgets {
  void drawHeader(TFT_eSPI &tft, const char *title);
  void drawHeaderTime(TFT_eSPI &tft);
  void drawNavigation(TFT_eSPI &tft, uint8_t activePage);
  void drawCard(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h, const char *title);
  void drawBatteryCard(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h, const BatteryData &battery);
  void drawValueCard(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h,
                     const char *label, const char *value, uint16_t accent);
  void drawPowerFlowCard(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h,
                         float solarWatts, float solarAmps,
                         float batteryWatts, float loadWatts, float loadAmps);
  void drawMenuRow(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h,
                   const char *label, const char *value, uint16_t statusColour);

  void drawSolarIcon(TFT_eSPI &tft, int16_t cx, int16_t cy, uint16_t colour);
  void drawBatteryIcon(TFT_eSPI &tft, int16_t cx, int16_t cy,
                       int16_t w, int16_t h, float percent, uint16_t colour);
  void drawHouseIcon(TFT_eSPI &tft, int16_t cx, int16_t cy, uint16_t colour);
  void drawFlowArrow(TFT_eSPI &tft, int16_t x1, int16_t x2, int16_t y,
                     bool leftToRight, uint16_t colour);

  uint16_t batteryColour(float soc);
  const char *chargeStateName(uint8_t state);
}

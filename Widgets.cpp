#include "Widgets.h"
#include "Config.h"
#include "Theme.h"
#include "RtcManager.h"

namespace {
  const char *navNames[] = {"Home", "Charge", "Loads", "Battery", "Energy", "History", "Clock", "Status"};
}

uint16_t Widgets::batteryColour(float soc) {
  // Five immediately recognisable SOC bands. Green is deliberately reserved
  // for a genuinely high battery state rather than merely "not low".
  if (soc >= 90.0f) return Theme::batteryGood;
  if (soc >= 60.0f) return Theme::batteryBlue;
  if (soc >= 40.0f) return Theme::batteryYellow;
  if (soc >= 20.0f) return Theme::batteryWarning;
  return Theme::batteryCritical;
}

const char *Widgets::chargeStateName(uint8_t state) {
  switch (state) {
    case 0: return "Off";
    case 1: return "Low Power";
    case 2: return "Fault";
    case 3: return "Bulk";
    case 4: return "Absorb";
    case 5: return "Float";
    case 6: return "Storage";
    case 7: return "Equalize";
    case 245: return "Starting";
    case 252: return "External";
    default: return "Unknown";
  }
}

void Widgets::drawHeader(TFT_eSPI &tft, const char *title) {
  tft.fillRect(0, 0, tft.width(), HEADER_HEIGHT, Theme::header);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(Theme::text, Theme::header);
  tft.drawString(title, 8, HEADER_HEIGHT / 2, 2);

  drawHeaderTime(tft);

  uint16_t bleColour = CampData::current.system.bleConnected ? Theme::batteryGood : Theme::textDim;
  uint16_t wifiColour = CampData::current.system.wifiConnected ? Theme::batteryGood : Theme::textDim;

  tft.fillCircle(tft.width() - 58, HEADER_HEIGHT / 2, 4, bleColour);
  tft.setTextColor(Theme::textMuted, Theme::header);
  tft.drawString("BLE", tft.width() - 50, HEADER_HEIGHT / 2, 1);
  tft.fillCircle(tft.width() - 22, HEADER_HEIGHT / 2, 4, wifiColour);
  tft.drawString("W", tft.width() - 14, HEADER_HEIGHT / 2, 1);
  tft.setTextDatum(TL_DATUM);
}

void Widgets::drawHeaderTime(TFT_eSPI &tft) {
  // The clock/date are isolated in a bounded centre region. Only this region is
  // refreshed once per minute, avoiding a full-page redraw and visible flicker.
  const int16_t centreX = tft.width() / 2;
  const int16_t width = 170;
  const int16_t left = centreX - (width / 2);
  tft.fillRect(left, 0, width, HEADER_HEIGHT, Theme::header);
  tft.setTextDatum(MC_DATUM);

  if (rtcManager.available()) {
    const DateTime now = rtcManager.now();
    uint8_t hour12 = now.hour() % 12;
    if (hour12 == 0) hour12 = 12;

    char timeText[12];
    snprintf(timeText, sizeof(timeText), "%u:%02u %s",
             hour12, now.minute(), now.hour() < 12 ? "AM" : "PM");
    char dateText[22];
    static const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    snprintf(dateText, sizeof(dateText), "%s %u %s %u", days[now.dayOfTheWeek()],
             now.day(), months[now.month() - 1], now.year());

    const uint16_t clockColour = rtcManager.needsSetting() ? Theme::batteryWarning : Theme::text;
    tft.setTextColor(clockColour, Theme::header);
    tft.drawString(timeText, centreX, 12, 4);
    tft.drawString(timeText, centreX + 1, 12, 4);
    tft.setTextColor(Theme::textMuted, Theme::header);
    tft.drawString(dateText, centreX, 31, 1);
  } else {
    tft.setTextColor(Theme::textDim, Theme::header);
    tft.drawString("--:-- --", centreX, 12, 4);
    tft.drawString("RTC unavailable", centreX, 31, 1);
  }
  tft.setTextDatum(TL_DATUM);
}

void Widgets::drawNavigation(TFT_eSPI &tft, uint8_t activePage) {
  const int16_t y = tft.height() - NAV_HEIGHT;
  const int16_t itemWidth = tft.width() / NAV_ITEM_COUNT;
  tft.fillRect(0, y, tft.width(), NAV_HEIGHT, Theme::nav);
  tft.drawFastHLine(0, y, tft.width(), Theme::cardBorder);

  for (uint8_t i = 0; i < NAV_ITEM_COUNT; ++i) {
    const bool active = (i == activePage);
    const uint16_t colour = active ? Theme::navActive : Theme::textMuted;
    const int16_t centreX = (i * itemWidth) + (itemWidth / 2);
    if (active) {
      tft.fillRoundRect(i * itemWidth + 6, y + 7, itemWidth - 12,
                        NAV_HEIGHT - 14, 9, Theme::card);
    }
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(colour, active ? Theme::card : Theme::nav);
    tft.drawString(navNames[i], centreX, y + NAV_HEIGHT / 2, NAV_ITEM_COUNT > 5 ? 1 : 2);
  }
  tft.setTextDatum(TL_DATUM);
}

void Widgets::drawCard(TFT_eSPI &tft, int16_t x, int16_t y,
                       int16_t w, int16_t h, const char *title) {
  tft.fillRoundRect(x, y, w, h, Theme::cornerRadius, Theme::card);
  tft.drawRoundRect(x, y, w, h, Theme::cornerRadius, Theme::cardBorder);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(Theme::textMuted, Theme::card);
  tft.drawString(title, x + 8, y + 5, 2);
}

void Widgets::drawBatteryIcon(TFT_eSPI &tft, int16_t cx, int16_t cy,
                              int16_t w, int16_t h, float percent, uint16_t colour) {
  const int16_t x = cx - w / 2;
  const int16_t y = cy - h / 2;
  const int16_t terminalW = 5;
  const int16_t inset = 3;

  tft.drawRoundRect(x, y, w, h, 5, colour);
  tft.fillRoundRect(x + w, cy - h / 5, terminalW, (h * 2) / 5, 2, colour);
  tft.fillRect(x + inset, y + inset, w - inset * 2, h - inset * 2, Theme::background);

  const float bounded = constrain(percent, 0.0f, 100.0f);
  const int16_t usableW = w - inset * 2;
  const int16_t fillW = static_cast<int16_t>(usableW * bounded / 100.0f);
  if (fillW > 0) {
    tft.fillRoundRect(x + inset, y + inset, fillW, h - inset * 2, 2, colour);
  }
}

void Widgets::drawSolarIcon(TFT_eSPI &tft, int16_t cx, int16_t cy, uint16_t colour) {
  tft.drawCircle(cx, cy, 8, colour);
  tft.fillCircle(cx, cy, 4, colour);
  tft.drawFastHLine(cx - 15, cy, 6, colour);
  tft.drawFastHLine(cx + 10, cy, 6, colour);
  tft.drawFastVLine(cx, cy - 15, 6, colour);
  tft.drawFastVLine(cx, cy + 10, 6, colour);
  tft.drawLine(cx - 11, cy - 11, cx - 7, cy - 7, colour);
  tft.drawLine(cx + 7, cy + 7, cx + 11, cy + 11, colour);
  tft.drawLine(cx + 7, cy - 7, cx + 11, cy - 11, colour);
  tft.drawLine(cx - 11, cy + 11, cx - 7, cy + 7, colour);
}

void Widgets::drawHouseIcon(TFT_eSPI &tft, int16_t cx, int16_t cy, uint16_t colour) {
  tft.drawTriangle(cx - 13, cy - 1, cx, cy - 13, cx + 13, cy - 1, colour);
  tft.drawRect(cx - 10, cy - 1, 20, 15, colour);
  tft.drawRect(cx - 3, cy + 6, 6, 8, colour);
}

void Widgets::drawFlowArrow(TFT_eSPI &tft, int16_t x1, int16_t x2, int16_t y,
                            bool leftToRight, uint16_t colour) {
  if (x2 < x1) {
    int16_t temp = x1;
    x1 = x2;
    x2 = temp;
  }
  tft.drawFastHLine(x1, y, x2 - x1 + 1, colour);
  if (leftToRight) {
    tft.fillTriangle(x2, y, x2 - 7, y - 4, x2 - 7, y + 4, colour);
  } else {
    tft.fillTriangle(x1, y, x1 + 7, y - 4, x1 + 7, y + 4, colour);
  }
}

void Widgets::drawBatteryCard(TFT_eSPI &tft, int16_t x, int16_t y,
                              int16_t w, int16_t h, const BatteryData &battery) {
  drawCard(tft, x, y, w, h, "HOUSE BATTERY");

  const uint16_t colour = batteryColour(battery.soc);
  const uint16_t flowColour = battery.current >= 0.0f ? Theme::charging : Theme::discharging;
  const bool lowBatteryVisible = battery.soc >= 20.0f || ((millis() / 500UL) & 1U) == 0U;
  char buffer[32];

  const int16_t iconX = x + w / 4;
  const int16_t iconY = y + h * 43 / 100;
  const int16_t iconW = min((int16_t)116, (int16_t)(w / 3));
  const int16_t iconH = min((int16_t)52, (int16_t)(h / 2));
  if (lowBatteryVisible) {
    drawBatteryIcon(tft, iconX, iconY, iconW, iconH, battery.soc, colour);

    // A compact lightning bolt makes charging state readable from a distance.
    if (battery.connected && battery.current > 0.10f) {
      const int16_t bx = iconX;
      const int16_t by = iconY;
      tft.fillTriangle(bx + 2, by - 18, bx - 8, by + 1, bx + 1, by + 1, Theme::text);
      tft.fillTriangle(bx - 1, by - 1, bx + 8, by - 1, bx - 3, by + 18, Theme::text);
    }
  }

  // Scale the existing value font rather than switching typefaces. Two-times
  // scaling is the largest size that still leaves room for a three-digit SOC.
  const int16_t socX = x + w * 3 / 4;
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(Theme::text, Theme::card);
  snprintf(buffer, sizeof(buffer), battery.connected ? "%.0f" : "--", battery.soc);
  tft.setTextSize(2);
  tft.drawString(buffer, socX + 18, iconY, 4);
  tft.setTextSize(1);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(Theme::textMuted, Theme::card);
  tft.drawString("%", socX + 23, iconY + 5, 2);

  const int16_t dividerY = y + h * 70 / 100;
  tft.drawFastHLine(x + 14, dividerY, w - 28, Theme::cardBorder);
  const int16_t valueY = y + h - 16;

  tft.setTextDatum(MC_DATUM);
  snprintf(buffer, sizeof(buffer), "%.2fV", battery.voltage);
  tft.setTextColor(Theme::text, Theme::card);
  tft.drawString(buffer, x + w * 1 / 6, valueY, 4);

  snprintf(buffer, sizeof(buffer), "%+.1fA", battery.current);
  tft.setTextColor(flowColour, Theme::card);
  tft.drawString(buffer, x + w * 3 / 6, valueY, 4);

  snprintf(buffer, sizeof(buffer), "%+.0fW", battery.power);
  tft.drawString(buffer, x + w * 5 / 6, valueY, 4);
  tft.setTextDatum(TL_DATUM);
}

void Widgets::drawValueCard(TFT_eSPI &tft, int16_t x, int16_t y,
                            int16_t w, int16_t h, const char *label,
                            const char *value, uint16_t accent) {
  drawCard(tft, x, y, w, h, label);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(accent, Theme::card);
  tft.drawString(value, x + w / 2, y + h / 2 + 9, 4);
  tft.setTextDatum(TL_DATUM);
}

void Widgets::drawPowerFlowCard(TFT_eSPI &tft, int16_t x, int16_t y,
                                int16_t w, int16_t h,
                                float solarWatts, float solarAmps,
                                float batteryWatts, float loadWatts, float loadAmps) {
  drawCard(tft, x, y, w, h, "POWER FLOW");

  const int16_t iconY = y + h / 2 + 5;
  const int16_t arrowY = y + h - 15;
  const int16_t solarX = x + 42;
  const int16_t batteryX = x + w / 2;
  const int16_t houseX = x + w - 42;
  const int16_t solarValueX = (solarX + batteryX) / 2;
  const int16_t loadValueX = (batteryX + houseX) / 2;

  const uint16_t solarColour = solarWatts > 1.0f ? Theme::solar : Theme::textDim;
  const uint16_t loadColour = loadWatts > 1.0f ? Theme::load : Theme::textDim;
  const uint16_t batteryFlowColour = batteryWatts >= 0.0f ? Theme::charging : Theme::discharging;

  drawSolarIcon(tft, solarX, iconY, solarColour);
  drawBatteryIcon(tft, batteryX, iconY, 52, 30,
                  batteryWatts >= 0.0f ? 100.0f : 35.0f, batteryFlowColour);
  drawHouseIcon(tft, houseX, iconY, loadColour);
  drawFlowArrow(tft, solarX + 20, batteryX - 30, arrowY, true, solarColour);
  drawFlowArrow(tft, batteryX + 31, houseX - 20, arrowY, true, loadColour);

  char text[24];
  tft.setTextDatum(MC_DATUM);

  snprintf(text, sizeof(text), "%.0fW", solarWatts);
  tft.setTextColor(solarColour, Theme::card);
  tft.drawString(text, solarValueX, y + 34, 4);
  snprintf(text, sizeof(text), "%.1fA", solarAmps);
  tft.drawString(text, solarValueX, y + 57, 2);

  snprintf(text, sizeof(text), "%.0fW", loadWatts);
  tft.setTextColor(loadColour, Theme::card);
  tft.drawString(text, loadValueX, y + 34, 4);
  snprintf(text, sizeof(text), "%.1fA", loadAmps);
  tft.drawString(text, loadValueX, y + 57, 2);
  tft.setTextDatum(TL_DATUM);
}

void Widgets::drawMenuRow(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h,
                          const char *label, const char *value, uint16_t statusColour) {
  tft.fillRoundRect(x, y, w, h, Theme::cornerRadius, Theme::card);
  tft.drawRoundRect(x, y, w, h, Theme::cornerRadius, Theme::cardBorder);
  tft.fillCircle(x + 14, y + h / 2, 4, statusColour);

  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(Theme::text, Theme::card);
  tft.drawString(label, x + 25, y + h / 2, 2);

  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(Theme::textMuted, Theme::card);
  tft.drawString(value, x + w - 22, y + h / 2, 2);
  tft.setTextColor(Theme::navActive, Theme::card);
  tft.drawString(">", x + w - 8, y + h / 2, 2);
  tft.setTextDatum(TL_DATUM);
}

#include "Theme.h"
#include <Preferences.h>

namespace {
  Preferences prefs;
  bool prefsReady = false;

  // The Freenove ST7796 setup deliberately runs with invertDisplay(true).
  // Define the colour we want the user to SEE, then invert the RGB565 word
  // before sending it to TFT_eSPI. This keeps Light/Dark/Blue visually correct.
  constexpr uint16_t visible(uint16_t colour) {
    return static_cast<uint16_t>(~colour);
  }

  constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r & 0xF8) << 8) |
                                 ((g & 0xFC) << 3) |
                                 (b >> 3));
  }

  void applyCommonColours() {
    Theme::batteryGood     = visible(rgb565(30, 190, 75));
    Theme::batteryBlue     = visible(rgb565(35, 115, 230));
    Theme::batteryYellow   = visible(rgb565(245, 205, 45));
    Theme::batteryWarning  = visible(rgb565(255, 155, 35));
    Theme::batteryCritical = visible(rgb565(225, 45, 45));
    Theme::discharging     = visible(rgb565(255, 120, 35));
  }
}

namespace Theme {
  ThemeId current = THEME_LIGHT;

  // Safe initial values. Theme::begin() replaces these before DisplayManager::begin().
  uint16_t background = visible(rgb565(250, 250, 250));
  uint16_t header = visible(rgb565(232, 236, 240));
  uint16_t card = visible(rgb565(244, 246, 248));
  uint16_t cardBorder = visible(rgb565(188, 194, 200));
  uint16_t nav = visible(rgb565(232, 236, 240));
  uint16_t navActive = visible(rgb565(30, 105, 220));
  uint16_t text = visible(rgb565(18, 22, 26));
  uint16_t textMuted = visible(rgb565(72, 78, 84));
  uint16_t textDim = visible(rgb565(135, 140, 146));
  uint16_t batteryGood = visible(rgb565(30, 190, 75));
  uint16_t batteryBlue = visible(rgb565(35, 115, 230));
  uint16_t batteryYellow = visible(rgb565(245, 205, 45));
  uint16_t batteryWarning = visible(rgb565(255, 155, 35));
  uint16_t batteryCritical = visible(rgb565(225, 45, 45));
  uint16_t solar = visible(rgb565(225, 125, 20));
  uint16_t load = visible(rgb565(30, 105, 220));
  uint16_t charging = visible(rgb565(30, 120, 225));
  uint16_t discharging = visible(rgb565(255, 120, 35));
  uint8_t cornerRadius = 8;

  const char *name(ThemeId id) {
    switch (id) {
      case THEME_LIGHT: return "Light";
      case THEME_DARK: return "Dark";
      case THEME_VICTRON: return "Victron";
      case THEME_GLASS: return "Glass";
      default: return "?";
    }
  }

  void begin() {
    uint8_t saved = static_cast<uint8_t>(THEME_LIGHT);
    prefsReady = prefs.begin("cm-theme", false);
    if (prefsReady) saved = prefs.getUChar("theme", static_cast<uint8_t>(THEME_LIGHT));
    if (saved >= static_cast<uint8_t>(THEME_COUNT)) saved = static_cast<uint8_t>(THEME_LIGHT);
    apply(static_cast<ThemeId>(saved), false);
  }

  void apply(ThemeId id, bool save) {
    if (id >= THEME_COUNT) id = THEME_LIGHT;
    current = id;
    applyCommonColours();

    switch (id) {
      case THEME_DARK:
        background = visible(rgb565(0, 0, 0));
        header      = visible(rgb565(18, 18, 18));
        card        = visible(rgb565(32, 34, 38));
        cardBorder  = visible(rgb565(82, 86, 92));
        nav         = visible(rgb565(16, 17, 20));
        navActive   = visible(rgb565(70, 170, 255));
        text        = visible(rgb565(245, 245, 245));
        textMuted   = visible(rgb565(190, 194, 200));
        textDim     = visible(rgb565(112, 116, 122));
        solar       = visible(rgb565(255, 205, 55));
        load        = visible(rgb565(75, 190, 255));
        charging    = visible(rgb565(60, 185, 255));
        break;

      case THEME_VICTRON:
        background = visible(rgb565(8, 22, 40));
        header      = visible(rgb565(10, 45, 78));
        card        = visible(rgb565(17, 42, 66));
        cardBorder  = visible(rgb565(42, 91, 130));
        nav         = visible(rgb565(8, 30, 52));
        navActive   = visible(rgb565(60, 175, 255));
        text        = visible(rgb565(245, 250, 255));
        textMuted   = visible(rgb565(178, 202, 220));
        textDim     = visible(rgb565(95, 128, 150));
        solar       = visible(rgb565(255, 205, 55));
        load        = visible(rgb565(75, 200, 255));
        charging    = visible(rgb565(55, 185, 255));
        break;

      case THEME_GLASS:
        background = visible(rgb565(10, 11, 13));
        header      = visible(rgb565(10, 11, 13));
        card        = visible(rgb565(10, 11, 13));
        cardBorder  = visible(rgb565(115, 122, 130));
        nav         = visible(rgb565(10, 11, 13));
        navActive   = visible(rgb565(80, 175, 255));
        text        = visible(rgb565(245, 245, 248));
        textMuted   = visible(rgb565(185, 190, 198));
        textDim     = visible(rgb565(100, 105, 112));
        solar       = visible(rgb565(255, 205, 55));
        load        = visible(rgb565(80, 190, 255));
        charging    = visible(rgb565(60, 185, 255));
        break;

      case THEME_LIGHT:
      default:
        current = THEME_LIGHT;
        background = visible(rgb565(250, 250, 250));
        header      = visible(rgb565(232, 236, 240));
        card        = visible(rgb565(244, 246, 248));
        cardBorder  = visible(rgb565(188, 194, 200));
        nav         = visible(rgb565(232, 236, 240));
        navActive   = visible(rgb565(30, 105, 220));
        text        = visible(rgb565(18, 22, 26));
        textMuted   = visible(rgb565(72, 78, 84));
        textDim     = visible(rgb565(135, 140, 146));
        solar       = visible(rgb565(225, 125, 20));
        load        = visible(rgb565(30, 105, 220));
        charging    = visible(rgb565(30, 120, 225));
        break;
    }

    if (save && prefsReady) prefs.putUChar("theme", static_cast<uint8_t>(current));
  }
}

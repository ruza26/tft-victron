#pragma once
#include <TFT_eSPI.h>

enum ThemeId : uint8_t {
  THEME_LIGHT = 0,
  THEME_DARK,
  THEME_VICTRON,
  THEME_GLASS,
  THEME_COUNT
};

namespace Theme {
  extern ThemeId current;

  extern uint16_t background;
  extern uint16_t header;
  extern uint16_t card;
  extern uint16_t cardBorder;
  extern uint16_t nav;
  extern uint16_t navActive;

  extern uint16_t text;
  extern uint16_t textMuted;
  extern uint16_t textDim;

  extern uint16_t batteryGood;
  extern uint16_t batteryBlue;
  extern uint16_t batteryYellow;
  extern uint16_t batteryWarning;
  extern uint16_t batteryCritical;

  extern uint16_t solar;
  extern uint16_t load;
  extern uint16_t charging;
  extern uint16_t discharging;

  extern uint8_t cornerRadius;

  // Load the saved theme from ESP32 Preferences and apply it before the TFT starts.
  void begin();

  // Apply a theme. By default it is also stored so it survives power loss.
  void apply(ThemeId id, bool save = true);

  const char *name(ThemeId id);
}

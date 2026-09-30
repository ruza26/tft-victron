#pragma once
#include <Arduino.h>

#define APP_NAME        "CAMP MONITOR"
#define APP_VERSION     "1.5.2"

#define DISPLAY_ROTATION 3

// Freenove FNK0103S: TFT_eSPI handles the touch controller.
// Rotation 3 is upright; the touch X axis is mirrored and corrected in TouchManager.
#define TOUCH_MIRROR_X  1

// The FNK0103S backlight is not exposed as the old CYD GPIO21 PWM output.
// Screen sleep uses ST7796 display on/off commands instead.
#define BACKLIGHT_PIN   -1

#define MAX_VICTRON_DEVICES 8
#define DEVICES_PER_PAGE 2
#define NAV_ITEM_COUNT 8

#define HEADER_HEIGHT   38
#define NAV_HEIGHT      68
#define CONTENT_TOP     HEADER_HEIGHT

#define SIM_UPDATE_MS   500
#define UI_UPDATE_MS    250

#define DEV_MODE        0


#define BLE_SCAN_SECONDS       3
#define BLE_SCAN_PAUSE_MS      250
#define VICTRON_COMPANY_ID     0x02E1

#define SETUP_PORTAL_TIMEOUT_MS  900000UL

#define LIVE_DATA_TIMEOUT_MS       10000UL
#define DEMO_FALLBACK_TIMEOUT_MS   300000UL
#define SCREEN_SLEEP_TIMEOUT_MS    120000UL

// On-board external I2C connector (manual schematic: SDA=IO32, SCL=IO25).
#define RTC_SDA_PIN 32
#define RTC_SCL_PIN 25
#define DEFAULT_ENERGY_ROLLOVER_HOUR 6

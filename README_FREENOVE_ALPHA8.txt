CampMonitor Freenove 4-inch Alpha 8
===================================

Hardware target:
- Freenove FNK0103S
- ESP32-WROOM-32E
- ST7796 320x480 display, landscape rotation 3
- TFT_BL GPIO27, active HIGH

Changes from Test 2:
- Backlight PWM attaches only after tft.init().
- Uses ESP32 Core 3.x ledcAttach/ledcWrite on TFT_BL (GPIO27).
- True backlight off during automatic sleep.
- Dashboard rendering is suspended while asleep.
- A fresh touch wakes the screen and forces a full redraw.
- Two-second hold cycles real PWM brightness.
- Brief resistive-touch dropouts are tolerated during a hold.
- Touch X mirroring remains enabled for rotation 3.

Victron BLE decoding, setup portal, key storage and data model are unchanged.

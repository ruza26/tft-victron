# Changelog

## v1.5.4

- Added a custom Home-screen heading configurable from the maintenance webpage.
- Added first-run detection so a fresh installation automatically starts the CampMonitor setup Wi-Fi after a short startup check.
- Existing configured installations are migrated without unnecessarily opening the setup AP when a saved Victron key is detected.
- Added a Factory Reset Setup action that clears the display heading and Victron provisioning while leaving RTC/history/theme data intact.
- Added the public GitHub Pages Web Flasher for USB installation without Arduino IDE.
- Web Flasher uses the merged firmware binary; normal CampMonitor OTA updates continue to use the application `.ino.bin`.

## v1.5.3

- Added charging/discharging time information to the Chargers page.
- Charging estimate uses SmartShunt net battery current so charging sources outside the Victron charger network are still reflected.
- Corrected the Chargers-page card border theme reference.

## v1.5.2

- Renamed the Loads-page total label to **Consumption**.
- Renamed the Energy-page battery-use label to **Battery Used Today**.

## v1.5.1

- Changed the screen-saver timeout to 2 minutes.
- Theme selection now returns directly to the Home page.
- Removed the extra Back to Status action from the theme selector.

## v1.5.0

- Added persistent Light, Dark, Victron and Glass themes.
- Added touchscreen theme selection.
- Added browser-based maintenance and OTA workflow improvements.

## v1.4.x

- Added larger clock/date presentation and RTC improvements.
- Added browser OTA support without requiring permanent property/home Wi-Fi credentials.
- Continued CYD test builds alongside the Freenove 4-inch target.

## v1.3.x

- Added DS3231 RTC support and touchscreen clock/date setting.
- Added 30-day daily-energy history using the AT24C32 EEPROM commonly fitted to the RTC module.
- Added 06:00 daily rollover so overnight loads remain with the previous energy day.

## v1.2.x and earlier

- Established Victron BLE discovery, Instant Readout decoding and runtime encryption-key provisioning.
- Added SmartShunt battery monitoring, SmartSolar charger monitoring, Loads and Home power-flow pages.
- Added touchscreen navigation, brightness control, sleep/wake behaviour and demo/search/lost states.
- Added battery SOC colours, charging indication and other field-display refinements.

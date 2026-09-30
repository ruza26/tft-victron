CampMonitor Freenove 4in Alpha 9.5

Changes from Alpha 9.4:
- Fixed PageManager::updateLoads() compile error by obtaining the TFT reference from DisplayManager before drawing.
- Doubled the bottom navigation bar height from 34 to 68 pixels for easier finger operation.
- Increased navigation label font size and active-button area.
- Expanded dashboard, charger, load, battery, and status cards to use the full 480-pixel landscape width.
- Enlarged the battery and power-flow widgets and their key readings.
- Increased charger/load row height and changed those pages to two larger device rows per page.
- Reworked dashboard and battery incremental redraws so wider responsive widgets do not rely on old 320-pixel fixed coordinates.

Target hardware:
Freenove FNK0103S / ESP32-WROOM-32E / ST7796 4-inch TFT.

Build note:
The project folder and main .ino filename match, as required by Arduino IDE.

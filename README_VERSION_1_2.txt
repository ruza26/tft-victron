CampMonitor Version 1.2
=======================

New hardware
------------
DS3231 RTC module connected to the display board's external I2C socket:

  RTC VCC -> 3.3V
  RTC GND -> GND
  RTC SDA -> board SDA (ESP32 GPIO32)
  RTC SCL -> board SCL (ESP32 GPIO25)

Leave the RTC module's SQW and 32K pins disconnected.

Required Arduino library
------------------------
Install "RTClib" by Adafruit using Arduino IDE Library Manager.
The ESP32 core supplies Wire and Preferences.

Clock settings
--------------
A new Clock page sets the DS3231 directly from the touchscreen. CampMonitor does
not need home Wi-Fi credentials or internet access.

Tap the minus or plus side of each row to adjust:
- day
- month
- year
- hour
- minute
- energy-day start hour

Tap SAVE CLOCK to write the selected local date and time to the DS3231.
Seconds are set to zero when Save is pressed.

Daily energy period
-------------------
The default energy day starts at 06:00, not midnight. This keeps overnight loads
such as Starlink, heating and device charging with the preceding camping day.
The start hour can be changed from 00:00 to 23:00 on the Clock page.

Energy Today page
-----------------
- Solar Generated: integrated SmartSolar output current in Ah.
- Battery Charged: integrated positive SmartShunt current in Ah.
- Battery Used: integrated negative SmartShunt current in Ah.
- Battery Net: Battery Charged minus Battery Used.
- Start SOC: SmartShunt SOC recorded at the beginning of the energy day.

Only LIVE Victron data is accumulated. SEARCHING, LOST and DEMO data are ignored.
Counters are retained in ESP32 Preferences and periodically saved, so a restart
should not discard the current day's totals. A new period is started automatically
when the DS3231 crosses the configured 06:00 boundary.

Accuracy notes
--------------
The totals are calculated by numerical integration of the BLE current readings.
They are intended as practical daily energy figures. Brief missed BLE packets are
normal; long gaps and non-live periods are deliberately excluded rather than
inventing data.


Version 1.2.2 display update
----------------------------
The current DS3231 time is displayed in the top header as HH:MM:SS on every page. It updates once per second without redrawing the full screen.

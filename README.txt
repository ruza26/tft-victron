CampMonitor V0.8 Alpha 4.0 - Live SmartShunt

TEST BUILD
- Keeps the working Alpha 3.2 open setup AP and NVS key storage.
- Loads the saved key by detected BLE address.
- Decrypts Victron Instant Readout battery-monitor advertisements using AES-128-CTR.
- Updates dashboard voltage, current, power, SOC, consumed Ah and time remaining.
- Stops simulator updates after the first successfully decoded packet.
- Marks live data offline after 10 seconds without a valid packet.

Arduino settings used successfully for Alpha 3.x:
- Board: ESP32 Dev Module
- Partition Scheme: Huge APP (3MB No OTA / 1MB SPIFFS)

Expected test sequence:
1. Upload without erasing flash so the stored key remains.
2. Keep VictronConnect disconnected from the SmartShunt.
3. Wait up to 10 seconds for live values.
4. System/BLE page should show decoded packets increasing.

Notes:
- The Victron AES-CTR format has no packet MIC. This build validates keys using the clear key-check byte plus decoded-value plausibility.
- Device-key matching remains BLE-address based in this Alpha.

V1.3.0 NOTES
-------------
The DS3231 is never automatically set to the firmware compile time. Set it from the Clock page.
Daily history uses the AT24C32 EEPROM at 0x57 and stores up to 30 completed energy days.
History rollover defaults to 06:00 and follows the DAY START value on the Clock page.


VERSION 1.4.0 - CLOCK, DATE AND WEB OTA
----------------------------------------
- Enlarged centred 12-hour clock with date underneath.
- The clock/date area refreshes once per minute to avoid flicker.
- Status page now starts a temporary CampMonitor maintenance Wi-Fi network.
- Connect to that network, open http://192.168.4.1 and choose Firmware Update.
- Upload the exported Arduino firmware .bin; no property Wi-Fi credentials are stored.
- A local partitions.csv provides two OTA application slots on 4 MB flash.
- Keep the complete sketch folder together so Arduino uses the included partition table.

To create an update file in Arduino IDE: Sketch > Export Compiled Binary.
Use the main sketch .bin file, not the bootloader or partitions binary.

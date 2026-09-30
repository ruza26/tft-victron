CampMonitor Freenove 4-inch Test 1
===================================

Target: Freenove FNK0103S, ESP32-WROOM-32E, ST7796 320x480 panel.
Landscape orientation: TFT_eSPI rotation 3 (480x320).

Changes from Alpha 7.1:
- Removed XPT2046_Touchscreen dependency and all CYD touch pins/calibration.
- Touch is read through the Freenove TFT_eSPI tft.getTouch() API.
- Mirrors touch X after reading, matching the successful drawing test.
- Display sleep/wake uses ST7796 DISPOFF/DISPON rather than CYD GPIO21 PWM.
- Splash screen is centred for 480x320.
- Victron BLE, setup portal, data model, decoding and stored keys are unchanged.

Important:
- Use the same Freenove TFT_eSPI library/configuration that runs the Rainbow and touch tests.
- Select the same ESP32 board settings used for those working examples.
- This is a functional port for immediate shunt testing. The data-page layout is still the Alpha 7.1 layout and will be redesigned for the full 480-pixel width after hardware validation.

CampMonitor Version 1.1
=======================

Changes from Alpha 10:
- Animated demo mode starts after five minutes without valid live Victron data,
  including startup and post-disconnect conditions.
- Valid BLE data immediately restores LIVE mode.
- Header status badge now shows SEARCHING, LIVE, LOST, or DEMO.
- Screen sleep timeout reduced from ten minutes to five minutes.
- Battery SOC uses the existing font at 2x scale for maximum readability.
- Total Consumption now uses SmartShunt discharge only. SmartSolar load-output
  readings remain visible separately and are not double-counted.

Hardware target: Freenove FNK0103S / ESP32-WROOM-32E / ST7796 480x320.

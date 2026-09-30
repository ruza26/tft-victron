CampMonitor Freenove 4-inch Test 2

Changes from Test 1:
- Sleep now fills the display black instead of issuing ST7796 DISPOFF, which made this panel turn white.
- Wake requests a complete redraw of the active page.
- Added a 140 ms touch release grace period so brief gaps in resistive touch readings do not break a 2-second hold.
- Brightness uses the ST7796 0x51 display brightness command instead of the CYD backlight GPIO.

Test:
1. Confirm normal touch navigation.
2. Hold a finger steadily for at least 2 seconds. Brightness should cycle.
3. Let the sleep timer expire and confirm the screen becomes black.
4. Touch once and confirm the current page redraws.

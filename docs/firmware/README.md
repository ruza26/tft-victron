# Firmware file for the web flasher

Place the **merged** Arduino ESP32 binary here and name it:

`CampMonitor-v1.5.4-merged.bin`

With current Arduino-ESP32 cores, the build process creates a file ending in `.merged.bin` that is ready to flash at offset `0x0`.

Do **not** use the ordinary `.ino.bin` here. That application-only binary is for CampMonitor's existing OTA update page, not for a blank-device web install.

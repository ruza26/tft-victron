# CampMonitor

CampMonitor is a standalone ESP32 touchscreen monitor for small off-grid, camping, vehicle and caravan power systems using Victron Smart devices. It listens for Victron Bluetooth Low Energy advertisements, decodes configured devices locally, and displays battery, charging, load, clock and daily-energy information.

The current tested release is **v1.5.4** for the **Freenove 4-inch 480x320 ESP32 display**.

> CampMonitor is an independent community project and is not affiliated with or endorsed by Victron Energy.

## Easy installation

For a new monitor, use the **CampMonitor Web Flasher**:

https://ruza26.github.io/tft-victron/

Connect the compatible Freenove 4-inch ESP32 display by USB and follow the browser installer. No Arduino IDE or compiling is required.

### Web Flasher quick guide

1. Use a computer with a Web Serial-capable browser such as Chrome or Edge.
2. Connect the Freenove display to the computer with a USB data cable.
3. Open the CampMonitor Web Flasher above and click **Install CampMonitor**.
4. Select the ESP32 serial port when prompted and complete the installation.
5. Allow the display to restart after flashing.
6. On a fresh installation, connect to the automatically created `CampMonitor-XXXXXX` Wi-Fi network.
7. Open `http://192.168.4.1` and add your Victron devices/encryption keys and preferred display heading.

For an existing CampMonitor, download the normal application `.ino.bin` from the latest GitHub Release and upload it through CampMonitor's maintenance/OTA webpage. **Do not use the merged binary for OTA updates.**

## Features

- Victron BLE discovery with friendly device names and MAC addresses
- Browser-based Victron Instant Readout encryption-key setup
- SmartShunt battery data and SmartSolar charger data
- Home power-flow, Chargers, Loads and Energy pages
- DS3231 real-time clock with date/time setting
- Daily energy accounting with a 06:00 day boundary
- Rolling 30-day history in the AT24C32 EEPROM on the RTC module
- Light, Dark, Victron and Glass themes
- Customisable Home-screen heading from the web setup page
- Two-minute screen saver and touch wake
- Touch brightness control
- Temporary maintenance Wi-Fi access point
- Browser OTA firmware updates without storing property/home Wi-Fi credentials
- Automatic setup Wi-Fi on a fresh installation
- Factory-reset option for provisioning/setup data

## First-run setup

After a fresh installation, CampMonitor automatically starts its setup Wi-Fi after boot. Connect a phone, tablet or computer to the network named similar to `CampMonitor-XXXXXX`, then open `http://192.168.4.1`.

Use the setup page to add Victron devices and their 32-character Instant Readout encryption keys and to customise the display heading. Once configured, later boots start normally. The maintenance Wi-Fi can still be started manually when configuration or OTA updates are needed.

Encryption keys are stored locally in ESP32 Preferences. **Do not put personal Victron keys or device secrets into source files before publishing or sharing a fork.**

## Hardware

Tested hardware:

- Freenove/LCDWiki-style 4-inch 480x320 ESP32 display with ST7796 TFT and resistive touch
- ESP32-WROOM/ESP32-32E class module
- DS3231 RTC module (HW-084 or similar) with AT24C32 EEPROM
- Victron SmartShunt and/or compatible Victron SmartSolar devices broadcasting Instant Readout data

### 3D printed enclosure

Printable enclosure files are in [`3d-print/`](3d-print/):

- [`case-bottom-plainv15.stl`](3d-print/case-bottom-plainv15.stl)
- [`freenove back case (1).stl`](3d-print/freenove%20back%20case%20(1).stl)

They are intended for the supported Freenove 4-inch display and form a serviceable enclosure that can be opened again if a component needs replacement. See the README inside the `3d-print` folder for printing notes.

### RTC wiring

| RTC | Display board |
| --- | --- |
| VCC | 3.3 V |
| GND | GND |
| SDA | GPIO 32 / SDA |
| SCL | GPIO 25 / SCL |
| 32K | Not connected |
| SQW | Not connected |

The DS3231 normally appears at I2C address `0x68`; the EEPROM normally appears at `0x57`.

### TFT_eSPI configuration

Tested display configuration:

- ST7796 driver
- MOSI GPIO 13
- MISO GPIO 12
- SCK GPIO 14
- TFT CS GPIO 15
- TFT DC GPIO 2
- Touch CS GPIO 33
- Backlight GPIO 27, active HIGH

A reference setup is included at `extras/User_Setup_Freenove_4in.h`.

## Building from source

For the exact source used for the current **v1.5.4** release, use the source ZIP attached to the **v1.5.4 GitHub Release**. This avoids accidentally compiling an older working-tree snapshot.

External Arduino libraries:

- **TFT_eSPI** (tested with 2.5.43)
- **RTClib** by Adafruit

The ESP32 Arduino core supplies BLE, Wi-Fi, WebServer, Preferences, Update, Wire and mbedTLS support.

The tested build uses a 4 MB ESP32 with the Huge APP partition selection. Keep `partitions.csv` with the sketch when compiling so browser OTA support remains available.

## OTA updates

In Arduino IDE choose **Sketch -> Export Compiled Binary**.

For CampMonitor's own OTA webpage, use the normal application file `CampMonitor....ino.bin`. Do not upload `.merged.bin`, `.bootloader.bin` or `.partitions.bin` through the OTA page.

The **merged binary** is used by the public USB Web Flasher for fresh installations.

## Daily energy history

The CampMonitor energy day rolls over at **06:00**, keeping overnight loads with the preceding camping/off-grid day instead of splitting them at midnight. Completed days are stored in a 30-record circular history in the RTC module EEPROM; after 30 records, the oldest is overwritten.

## Themes and display

CampMonitor includes Light, Dark, Victron and Glass themes. Theme selection is saved in ESP32 Preferences and returns directly to the Home page. The Home heading can be renamed from the setup webpage for installations in a car, van, caravan, shed or other system.

The screen saver starts after **2 minutes** without touch. BLE processing and energy monitoring continue while the display is asleep.

## Supported display

The currently tested and supported target is the **Freenove 4-inch 480x320 display**. The UI is not currently claimed to scale automatically to arbitrary screen sizes.

## Repository layout

```text
CampMonitor.ino          Main application entry point
Config.h                 Build-time configuration and timeouts
DataModel.*              Shared live-data model
VictronBleScanner.*      BLE discovery and packet handling
VictronDecoder.*         Victron Instant Readout decoding
SetupPortal.*            Device provisioning, setup and browser OTA
RtcManager.*             DS3231 clock support
EnergyManager.*          Current-day energy accounting
HistoryManager.*         30-day AT24C32 history
DisplayManager.*         TFT initialization/display control
TouchManager.*           Touch, wake and brightness handling
PageManager.*            Page navigation and UI rendering
Widgets.*                Reusable display widgets
Theme.*                  Display palettes
partitions.csv           OTA-capable partition layout
docs/                    GitHub Pages Web Flasher
3d-print/                Printable enclosure files
extras/                   Board/library reference configuration
```

## Contributing

Issues, testing reports and pull requests are welcome. For hardware problems, include the display board/revision, ESP32 Arduino core version, TFT_eSPI version and the complete compile error or serial output.

## License

CampMonitor is released under the [MIT License](LICENSE).

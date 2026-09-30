// TFT_eSPI setup used by CampMonitor on the Freenove/LCDWiki-style 4.0" ESP32 display.
// Copy the relevant definitions into TFT_eSPI/User_Setup.h, or select this setup
// from User_Setup_Select.h according to your TFT_eSPI installation.

#define USER_SETUP_INFO "CampMonitor Freenove 4in ST7796"

#define ST7796_DRIVER

#define TFT_WIDTH  320
#define TFT_HEIGHT 480

#define TFT_MOSI 13
#define TFT_MISO 12
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1

#define TOUCH_CS 33

// Board backlight pin confirmed on the tested hardware.
#define TFT_BL 27
#define TFT_BACKLIGHT_ON HIGH

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY 2500000

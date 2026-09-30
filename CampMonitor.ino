#include <TFT_eSPI.h>

#include "Config.h"
#include "Theme.h"
#include "DataModel.h"
#include "DisplayManager.h"
#include "PageManager.h"
#include "Simulator.h"
#include "TouchManager.h"
#include "VictronBleScanner.h"
#include "SetupPortal.h"
#include "RtcManager.h"
#include "EnergyManager.h"
#include "HistoryManager.h"

TFT_eSPI tft = TFT_eSPI();

DisplayManager display(tft);
PageManager pages(display);
Simulator simulator;
TouchManager touchManager(tft, pages);
VictronBleScanner victronBleScanner;

void setup() {
  Serial.begin(115200);

  Theme::begin();
  display.begin();
  touchManager.begin();
  display.drawSplash();
  CampData::beginWaitingValues();
  rtcManager.begin();
  historyManager.begin();
  energyManager.begin();

  pages.begin();
  simulator.begin();
  setupPortal.begin();
  victronBleScanner.begin();

  delay(600);
  pages.show(PAGE_DASHBOARD);
}

void loop() {
  victronBleScanner.update();
  energyManager.update();
  setupPortal.update();
  simulator.update();
  touchManager.update();
  if (touchManager.isScreenAwake()) {
    pages.update();
  }
  delay(10);
}

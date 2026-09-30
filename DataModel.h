#pragma once
#include <Arduino.h>
#include "Config.h"

enum DataSourceMode : uint8_t {
  DATA_SOURCE_WAITING = 0,
  DATA_SOURCE_DEMO,
  DATA_SOURCE_LIVE,
  DATA_SOURCE_LOST
};

struct BatteryData {
  float voltage;
  float current;
  float power;
  float soc;
  float consumedAh;
  int remainingMinutes;
  bool connected;
};

struct ChargerData {
  bool provisioned;
  bool connected;
  bool hasChargerData;
  bool hasLoadData;
  char name[24];
  char address[20];
  float inputVoltage;
  float batteryVoltage;
  float chargeCurrent;
  float chargePower;
  uint8_t chargeState;
  float loadCurrent;
  float loadPower;
  bool loadOutputOn;
};

struct SystemStatus {
  bool bleConnected;
  bool wifiConnected;
  uint8_t brightness;
  uint32_t rejectedPackets;
  DataSourceMode dataSource;
  uint32_t lastLiveDataMs;
};

struct CampData {
  BatteryData battery;
  ChargerData chargers[MAX_VICTRON_DEVICES];
  SystemStatus system;
  float totalChargerPower;
  float totalChargerCurrent;
  float estimatedLoadPower;
  float estimatedLoadCurrent;

  static CampData current;
  static void beginWaitingValues();
  static void beginDemoValues();
  static void recalculateTotals();
  static uint8_t chargerCount();
  static uint8_t loadMonitorCount();
};

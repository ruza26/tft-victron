#include "DataModel.h"
#include <string.h>

CampData CampData::current;

void CampData::beginWaitingValues() {
  memset(&current, 0, sizeof(current));
  current.battery.remainingMinutes = -1;
  current.system.brightness = 100;
  current.system.dataSource = DATA_SOURCE_WAITING;
}

void CampData::beginDemoValues() {
  memset(&current, 0, sizeof(current));

  current.battery.voltage = 13.42f;
  current.battery.current = 18.2f;
  current.battery.power = current.battery.voltage * current.battery.current;
  current.battery.soc = 97.0f;
  current.battery.consumedAh = 12.4f;
  current.battery.remainingMinutes = 2310;
  current.battery.connected = true;

  ChargerData &demo = current.chargers[0];
  demo.provisioned = true;
  demo.connected = true;
  demo.hasChargerData = true;
  demo.hasLoadData = true;
  strlcpy(demo.name, "SmartSolar MPPT", sizeof(demo.name));
  demo.inputVoltage = 38.6f;
  demo.batteryVoltage = 13.5f;
  demo.chargeCurrent = 24.0f;
  demo.chargePower = 324.0f;
  demo.chargeState = 1;
  demo.loadCurrent = 6.1f;
  demo.loadPower = 82.0f;
  demo.loadOutputOn = true;

  current.system.bleConnected = true;
  current.system.wifiConnected = false;
  current.system.brightness = 100;
  current.system.rejectedPackets = 0;
  current.system.dataSource = DATA_SOURCE_DEMO;
  current.system.lastLiveDataMs = 0;

  recalculateTotals();
}

void CampData::recalculateTotals() {
  current.totalChargerPower = 0.0f;
  current.totalChargerCurrent = 0.0f;

  for (uint8_t i = 0; i < MAX_VICTRON_DEVICES; ++i) {
    const ChargerData &device = current.chargers[i];
    if (!device.provisioned || !device.connected || !device.hasChargerData) continue;
    current.totalChargerPower += max(0.0f, device.chargePower);
    current.totalChargerCurrent += max(0.0f, device.chargeCurrent);
  }

  // Version 1.1: the consumption total uses the SmartShunt discharge only.
  // A SmartSolar load output is still displayed as a separate measured circuit,
  // but is not added here because that current normally already passes through
  // the SmartShunt and would otherwise be counted twice. Charging or zero shunt
  // current contributes zero to the consumption total.
  current.estimatedLoadCurrent = current.battery.connected
    ? max(0.0f, -current.battery.current)
    : 0.0f;
  current.estimatedLoadPower = current.battery.connected
    ? max(0.0f, -current.battery.power)
    : 0.0f;
}

uint8_t CampData::chargerCount() {
  uint8_t count = 0;
  for (uint8_t i = 0; i < MAX_VICTRON_DEVICES; ++i) {
    if (current.chargers[i].provisioned && current.chargers[i].hasChargerData) ++count;
  }
  return count;
}

uint8_t CampData::loadMonitorCount() {
  uint8_t count = current.battery.connected ? 1 : 0;
  for (uint8_t i = 0; i < MAX_VICTRON_DEVICES; ++i) {
    if (current.chargers[i].provisioned && current.chargers[i].hasLoadData) ++count;
  }
  return count;
}

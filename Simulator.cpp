#include "Simulator.h"
#include "Config.h"
#include "DataModel.h"
#include "VictronBleScanner.h"
#include <math.h>

void Simulator::begin() {
  _lastUpdateMs = millis();
  _phase = 0.0f;
}

void Simulator::update() {
  // Demo animation is a production fallback in Version 1.1, not only a
  // developer feature. Any valid Victron packet immediately returns to LIVE.
  if (CampData::current.system.dataSource != DATA_SOURCE_DEMO) return;
  uint32_t now = millis();
  if (now - _lastUpdateMs < SIM_UPDATE_MS) return;

  _lastUpdateMs = now;
  _phase += 0.19f;

  CampData &camp = CampData::current;
  ChargerData &demo = camp.chargers[0];

  demo.chargePower = 310.0f + sinf(_phase) * 65.0f;
  demo.chargeCurrent = demo.chargePower / 13.5f;
  demo.inputVoltage = 38.0f + sinf(_phase * 0.7f) * 1.8f;

  demo.loadPower = 95.0f + sinf(_phase * 1.4f) * 38.0f;
  demo.loadCurrent = demo.loadPower / max(camp.battery.voltage, 1.0f);

  float batteryPower = demo.chargePower - demo.loadPower;

  camp.battery.voltage = 13.35f + sinf(_phase * 0.5f) * 0.12f;
  camp.battery.power = batteryPower;
  camp.battery.current = batteryPower / max(camp.battery.voltage, 1.0f);

  camp.battery.soc += (camp.battery.current >= 0.0f ? 0.02f : -0.02f);
  if (camp.battery.soc > 100.0f) camp.battery.soc = 100.0f;
  if (camp.battery.soc < 0.0f) camp.battery.soc = 0.0f;

  CampData::recalculateTotals();
}

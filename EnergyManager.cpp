#include "EnergyManager.h"
#include <Preferences.h>
#include <math.h>
#include "Config.h"
#include "DataModel.h"
#include "RtcManager.h"
#include "HistoryManager.h"

namespace {
Preferences prefs;
constexpr uint32_t SAVE_INTERVAL_MS = 300000UL;
constexpr uint32_t MAX_INTEGRATION_GAP_MS = 30000UL;
}

EnergyManager energyManager;

void EnergyManager::begin() {
  load();
  _lastSampleMs = millis();
  _lastSaveMs = _lastSampleMs;
  checkRollover();
}

void EnergyManager::load() {
  prefs.begin("camp-energy", true);
  _rolloverHour = constrain(prefs.getUChar("rollHour", DEFAULT_ENERGY_ROLLOVER_HOUR), 0, 23);
  _today.dayKey = prefs.getUInt("dayKey", 0);
  _today.solarGeneratedAh = prefs.getFloat("solarAh", 0.0f);
  _today.batteryChargedAh = prefs.getFloat("chargeAh", 0.0f);
  _today.batteryUsedAh = prefs.getFloat("usedAh", 0.0f);
  _today.startSoc = prefs.getFloat("startSoc", NAN);
  prefs.end();
}

void EnergyManager::save() {
  prefs.begin("camp-energy", false);
  prefs.putUChar("rollHour", _rolloverHour);
  prefs.putUInt("dayKey", _today.dayKey);
  prefs.putFloat("solarAh", _today.solarGeneratedAh);
  prefs.putFloat("chargeAh", _today.batteryChargedAh);
  prefs.putFloat("usedAh", _today.batteryUsedAh);
  prefs.putFloat("startSoc", _today.startSoc);
  prefs.end();
  _lastSaveMs = millis();
}

uint32_t EnergyManager::logicalDayKey() const {
  if (!rtcManager.available()) return 0;
  DateTime now = rtcManager.now();
  if (now.hour() < _rolloverHour) now = DateTime(now.unixtime() - 86400UL);
  return static_cast<uint32_t>(now.year()) * 10000UL +
         static_cast<uint32_t>(now.month()) * 100UL + now.day();
}

void EnergyManager::checkRollover() {
  const uint32_t key = logicalDayKey();
  if (key == 0) return;
  if (_today.dayKey == 0 || _today.dayKey != key) {
    // Archive the completed logical day before resetting its counters.
    if (_today.dayKey != 0 && historyManager.available()) {
      HistoryRecord record;
      record.dayKey = _today.dayKey;
      record.solarGeneratedAh = _today.solarGeneratedAh;
      record.batteryChargedAh = _today.batteryChargedAh;
      record.batteryUsedAh = _today.batteryUsedAh;
      record.startSoc = isnan(_today.startSoc) ? 255 :
                        static_cast<uint8_t>(constrain(lroundf(_today.startSoc), 0L, 100L));
      record.endSoc = CampData::current.battery.connected ?
                      static_cast<uint8_t>(constrain(lroundf(CampData::current.battery.soc), 0L, 100L)) : 255;
      historyManager.append(record);
    }

    _today.solarGeneratedAh = 0.0f;
    _today.batteryChargedAh = 0.0f;
    _today.batteryUsedAh = 0.0f;
    _today.startSoc = CampData::current.battery.connected ? CampData::current.battery.soc : NAN;
    _today.dayKey = key;
    save();
  } else if (isnan(_today.startSoc) && CampData::current.battery.connected) {
    _today.startSoc = CampData::current.battery.soc;
    save();
  }
}

void EnergyManager::update() {
  const uint32_t nowMs = millis();
  uint32_t elapsedMs = nowMs - _lastSampleMs;
  _lastSampleMs = nowMs;

  checkRollover();

  // Ignore demo/search/lost periods and large gaps caused by boot, sleep or stalls.
  if (elapsedMs == 0 || elapsedMs > MAX_INTEGRATION_GAP_MS ||
      CampData::current.system.dataSource != DATA_SOURCE_LIVE) {
    return;
  }

  const float hours = elapsedMs / 3600000.0f;
  _today.solarGeneratedAh += max(0.0f, CampData::current.totalChargerCurrent) * hours;

  if (CampData::current.battery.connected) {
    const float shuntCurrent = CampData::current.battery.current;
    if (shuntCurrent >= 0.0f) _today.batteryChargedAh += shuntCurrent * hours;
    else _today.batteryUsedAh += (-shuntCurrent) * hours;
  }

  if (nowMs - _lastSaveMs >= SAVE_INTERVAL_MS) save();
}

const DailyEnergyData &EnergyManager::today() const { return _today; }
float EnergyManager::batteryNetAh() const { return _today.batteryChargedAh - _today.batteryUsedAh; }
uint8_t EnergyManager::rolloverHour() const { return _rolloverHour; }

void EnergyManager::setRolloverHour(uint8_t hour) {
  _rolloverHour = constrain(hour, 0, 23);
  checkRollover();
  save();
}

void EnergyManager::onClockChanged() {
  checkRollover();
  save();
}

void EnergyManager::resetToday() {
  _today.dayKey = 0;
  checkRollover();
}

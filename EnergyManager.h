#pragma once

#include <Arduino.h>

struct DailyEnergyData {
  float solarGeneratedAh = 0.0f;
  float batteryChargedAh = 0.0f;
  float batteryUsedAh = 0.0f;
  float startSoc = NAN;
  uint32_t dayKey = 0;
};

class EnergyManager {
public:
  void begin();
  void update();
  const DailyEnergyData &today() const;
  float batteryNetAh() const;
  uint8_t rolloverHour() const;
  void setRolloverHour(uint8_t hour);
  void onClockChanged();
  void resetToday();

private:
  DailyEnergyData _today;
  uint8_t _rolloverHour = 6;
  uint32_t _lastSampleMs = 0;
  uint32_t _lastSaveMs = 0;

  uint32_t logicalDayKey() const;
  void checkRollover();
  void load();
  void save();
};

extern EnergyManager energyManager;

#pragma once

#include <Arduino.h>

struct HistoryRecord {
  uint32_t dayKey = 0;
  float solarGeneratedAh = 0.0f;
  float batteryChargedAh = 0.0f;
  float batteryUsedAh = 0.0f;
  uint8_t startSoc = 255;
  uint8_t endSoc = 255;
};

class HistoryManager {
public:
  static constexpr uint8_t CAPACITY = 30;

  void begin();
  bool available() const;
  uint8_t count() const;
  bool append(const HistoryRecord &record);
  bool getNewest(uint8_t newestIndex, HistoryRecord &record) const;

private:
  static constexpr uint8_t EEPROM_ADDRESS = 0x57;
  static constexpr uint16_t HEADER_ADDRESS = 0;
  static constexpr uint16_t RECORDS_ADDRESS = 16;

  bool _available = false;
  uint8_t _writeIndex = 0;
  uint8_t _count = 0;

  bool readBytes(uint16_t address, uint8_t *data, size_t length) const;
  bool writeBytes(uint16_t address, const uint8_t *data, size_t length);
  void loadHeader();
  bool saveHeader();
};

extern HistoryManager historyManager;

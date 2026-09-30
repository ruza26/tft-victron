#include "HistoryManager.h"
#include <Wire.h>
#include <math.h>

namespace {
constexpr uint32_t HISTORY_MAGIC = 0x434D4831UL; // CMH1
constexpr uint8_t HISTORY_VERSION = 1;

struct __attribute__((packed)) StoredHeader {
  uint32_t magic;
  uint8_t version;
  uint8_t writeIndex;
  uint8_t count;
  uint8_t crc;
};

struct __attribute__((packed)) StoredRecord {
  uint32_t dayKey;
  uint16_t solarTenths;
  uint16_t chargedTenths;
  uint16_t usedTenths;
  uint8_t startSoc;
  uint8_t endSoc;
  uint8_t reserved[3];
  uint8_t crc;
};

static_assert(sizeof(StoredRecord) == 16, "History record must be 16 bytes");

uint8_t crc8(const uint8_t *data, size_t length) {
  uint8_t crc = 0;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit)
      crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x07) : static_cast<uint8_t>(crc << 1);
  }
  return crc;
}

uint16_t toTenths(float value) {
  if (!isfinite(value) || value <= 0.0f) return 0;
  return static_cast<uint16_t>(constrain(lroundf(value * 10.0f), 0L, 65535L));
}

uint8_t toSoc(float value) {
  if (!isfinite(value)) return 255;
  return static_cast<uint8_t>(constrain(lroundf(value), 0L, 100L));
}
}

HistoryManager historyManager;

void HistoryManager::begin() {
  Wire.beginTransmission(EEPROM_ADDRESS);
  _available = Wire.endTransmission() == 0;
  if (_available) loadHeader();
}

bool HistoryManager::available() const { return _available; }
uint8_t HistoryManager::count() const { return _count; }

bool HistoryManager::readBytes(uint16_t address, uint8_t *data, size_t length) const {
  if (!_available) return false;
  size_t done = 0;
  while (done < length) {
    const uint8_t chunk = min(static_cast<size_t>(28), length - done);
    Wire.beginTransmission(EEPROM_ADDRESS);
    Wire.write(static_cast<uint8_t>((address + done) >> 8));
    Wire.write(static_cast<uint8_t>((address + done) & 0xFF));
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(EEPROM_ADDRESS, chunk) != chunk) return false;
    for (uint8_t i = 0; i < chunk; ++i) data[done + i] = Wire.read();
    done += chunk;
  }
  return true;
}

bool HistoryManager::writeBytes(uint16_t address, const uint8_t *data, size_t length) {
  if (!_available) return false;
  size_t done = 0;
  while (done < length) {
    const uint16_t current = address + done;
    const uint8_t pageRemaining = 32 - (current & 31);
    const uint8_t chunk = min(static_cast<size_t>(min(pageRemaining, static_cast<uint8_t>(28))), length - done);
    Wire.beginTransmission(EEPROM_ADDRESS);
    Wire.write(static_cast<uint8_t>(current >> 8));
    Wire.write(static_cast<uint8_t>(current & 0xFF));
    Wire.write(data + done, chunk);
    if (Wire.endTransmission() != 0) return false;
    delay(6);
    done += chunk;
  }
  return true;
}

void HistoryManager::loadHeader() {
  StoredHeader header{};
  if (!readBytes(HEADER_ADDRESS, reinterpret_cast<uint8_t *>(&header), sizeof(header)) ||
      header.magic != HISTORY_MAGIC || header.version != HISTORY_VERSION ||
      header.writeIndex >= CAPACITY || header.count > CAPACITY ||
      header.crc != crc8(reinterpret_cast<const uint8_t *>(&header), sizeof(header) - 1)) {
    _writeIndex = 0;
    _count = 0;
    saveHeader();
    return;
  }
  _writeIndex = header.writeIndex;
  _count = header.count;
}

bool HistoryManager::saveHeader() {
  StoredHeader header{HISTORY_MAGIC, HISTORY_VERSION, _writeIndex, _count, 0};
  header.crc = crc8(reinterpret_cast<const uint8_t *>(&header), sizeof(header) - 1);
  return writeBytes(HEADER_ADDRESS, reinterpret_cast<const uint8_t *>(&header), sizeof(header));
}

bool HistoryManager::append(const HistoryRecord &record) {
  if (!_available || record.dayKey == 0) return false;

  // A clock correction can call rollover twice. Do not store the same day twice.
  HistoryRecord newest;
  if (_count > 0 && getNewest(0, newest) && newest.dayKey == record.dayKey) return true;

  StoredRecord stored{};
  stored.dayKey = record.dayKey;
  stored.solarTenths = toTenths(record.solarGeneratedAh);
  stored.chargedTenths = toTenths(record.batteryChargedAh);
  stored.usedTenths = toTenths(record.batteryUsedAh);
  stored.startSoc = record.startSoc;
  stored.endSoc = record.endSoc;
  stored.crc = crc8(reinterpret_cast<const uint8_t *>(&stored), sizeof(stored) - 1);

  const uint16_t address = RECORDS_ADDRESS + static_cast<uint16_t>(_writeIndex) * sizeof(StoredRecord);
  if (!writeBytes(address, reinterpret_cast<const uint8_t *>(&stored), sizeof(stored))) return false;

  _writeIndex = (_writeIndex + 1) % CAPACITY;
  if (_count < CAPACITY) ++_count;
  return saveHeader();
}

bool HistoryManager::getNewest(uint8_t newestIndex, HistoryRecord &record) const {
  if (!_available || newestIndex >= _count) return false;
  const uint8_t slot = (_writeIndex + CAPACITY - 1 - newestIndex) % CAPACITY;
  StoredRecord stored{};
  const uint16_t address = RECORDS_ADDRESS + static_cast<uint16_t>(slot) * sizeof(StoredRecord);
  if (!readBytes(address, reinterpret_cast<uint8_t *>(&stored), sizeof(stored))) return false;
  if (stored.crc != crc8(reinterpret_cast<const uint8_t *>(&stored), sizeof(stored) - 1)) return false;

  record.dayKey = stored.dayKey;
  record.solarGeneratedAh = stored.solarTenths / 10.0f;
  record.batteryChargedAh = stored.chargedTenths / 10.0f;
  record.batteryUsedAh = stored.usedTenths / 10.0f;
  record.startSoc = stored.startSoc;
  record.endSoc = stored.endSoc;
  return true;
}

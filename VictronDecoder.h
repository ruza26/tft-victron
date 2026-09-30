#pragma once
#include <Arduino.h>

enum VictronRecordType : uint8_t {
  VICTRON_RECORD_SOLAR_CHARGER = 0x01,
  VICTRON_RECORD_BATTERY_MONITOR = 0x02,
  VICTRON_RECORD_INVERTER = 0x03,
  VICTRON_RECORD_DCDC_CONVERTER = 0x04,
  VICTRON_RECORD_SMART_LITHIUM = 0x05,
  VICTRON_RECORD_INVERTER_RS = 0x06,
  VICTRON_RECORD_GX_DEVICE = 0x07,
  VICTRON_RECORD_AC_CHARGER = 0x08,
  VICTRON_RECORD_SMART_BATTERY_PROTECT = 0x09,
  VICTRON_RECORD_SMART_BATTERY_SENSE = 0x0A,
  VICTRON_RECORD_BMS = 0x0B,
  VICTRON_RECORD_MULTI_RS = 0x0C,
  VICTRON_RECORD_VEBUS = 0x0D,
  VICTRON_RECORD_DC_ENERGY_METER = 0x0E,
  VICTRON_RECORD_ORION_XS = 0x0F
};

struct SmartShuntReading {
  bool valid = false, voltageAvailable = false, currentAvailable = false;
  bool socAvailable = false, consumedAhAvailable = false, remainingTimeAvailable = false;
  float voltage = 0, current = 0, soc = 0, consumedAh = 0;
  uint16_t remainingMinutes = 0, alarms = 0;
  uint8_t auxMode = 3;
};

struct VictronChargerReading {
  bool valid = false;
  uint8_t recordType = 0, state = 0xFF, error = 0xFF;
  bool inputVoltageAvailable = false, outputVoltageAvailable = false;
  bool outputCurrentAvailable = false, inputCurrentAvailable = false;
  bool solarPowerAvailable = false, yieldTodayAvailable = false;
  bool loadCurrentAvailable = false, outputStateAvailable = false;
  float inputVoltage = 0, outputVoltage = 0, outputCurrent = 0, inputCurrent = 0;
  float solarPower = 0, yieldTodayWh = 0, loadCurrent = 0;
  bool outputOn = false;
  uint32_t offReason = 0;
};

class VictronDecoder {
public:
  static bool decodeSmartShunt(const uint8_t*, size_t, const String&, SmartShuntReading&);
  static bool decodeCharger(const uint8_t*, size_t, const String&, VictronChargerReading&);
  static const char *recordTypeName(uint8_t recordType);
private:
  static bool hexToBytes(const String&, uint8_t*, size_t);
  static int32_t signExtend(uint32_t, uint8_t);
  static bool decryptPayload(const uint8_t*, size_t, const String&, uint8_t*, size_t&);
};

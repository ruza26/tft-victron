#include "VictronBleScanner.h"
#include "Config.h"
#include "DataModel.h"
#include "SetupPortal.h"
#include "VictronDecoder.h"
#include <string.h>

VictronBleStatus victronBleStatus;

namespace {
volatile bool scanFinished = false;

class VictronAdvertisedCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    String address = advertisedDevice.getAddress().toString();
    address.toUpperCase();

    // Active-scan responses often carry the user-assigned name separately from
    // the Victron manufacturer packet. Update an already known device whenever
    // such a name arrives.
    if (advertisedDevice.haveName()) {
      String advertisedName = advertisedDevice.getName();
      setupPortal.notifyBleName(advertisedName.c_str(), address.c_str());
    }

    if (!advertisedDevice.haveManufacturerData()) return;
    String data = advertisedDevice.getManufacturerData();
    if (data.length() < 3) return;
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(data.c_str());
    const uint16_t companyId = static_cast<uint16_t>(bytes[0]) |
                               (static_cast<uint16_t>(bytes[1]) << 8);
    if (companyId != VICTRON_COMPANY_ID) return;

    victronBleStatus.deviceSeen = true;
    victronBleStatus.rssi = advertisedDevice.getRSSI();
    victronBleStatus.packetCount++;
    victronBleStatus.lastSeenMs = millis();

    strlcpy(victronBleStatus.address, address.c_str(), sizeof(victronBleStatus.address));

    if (advertisedDevice.haveName()) {
      String name = advertisedDevice.getName();
      strlcpy(victronBleStatus.name, name.c_str(), sizeof(victronBleStatus.name));
    } else {
      strlcpy(victronBleStatus.name, "Victron device", sizeof(victronBleStatus.name));
    }

    setupPortal.notifyVictronDevice(victronBleStatus.name, victronBleStatus.address, victronBleStatus.rssi);

    if (bytes[2] == 0x10) {
      victronBleStatus.instantReadoutSeen = true;
      setupPortal.noteInstantReadout(victronBleStatus.address);
      String key = setupPortal.keyForAddress(victronBleStatus.address);
      victronBleStatus.keyLoaded = key.length() == 32;
      if (victronBleStatus.keyLoaded) {
        bool decoded = false;
        const uint8_t recordType = data.length() > 6 ? bytes[6] : 0xFF;
        bool keyIdentifierMatch = false;
        if (key.length() == 32) {
          char firstByte[3] = { key.charAt(0), key.charAt(1), 0 };
          char *end = nullptr;
          long value = strtol(firstByte, &end, 16);
          keyIdentifierMatch = end && *end == 0 && value >= 0 && value <= 255 && bytes[9] == static_cast<uint8_t>(value);
        }
        setupPortal.notePacketDetails(victronBleStatus.address, recordType, data.length(), keyIdentifierMatch);
        CampData &camp = CampData::current;

        if (recordType == VICTRON_RECORD_BATTERY_MONITOR) {
          SmartShuntReading reading;
          if (VictronDecoder::decodeSmartShunt(bytes, data.length(), key, reading)) {
            if (camp.system.dataSource == DATA_SOURCE_DEMO) {
              for (uint8_t i = 0; i < MAX_VICTRON_DEVICES; ++i) camp.chargers[i] = ChargerData{};
              camp.totalChargerPower = 0.0f;
              camp.totalChargerCurrent = 0.0f;
            }
            camp.battery.voltage = reading.voltage;
            camp.battery.current = reading.current;
            camp.battery.power = reading.voltage * reading.current;
            if (reading.socAvailable) camp.battery.soc = reading.soc;
            if (reading.consumedAhAvailable) camp.battery.consumedAh = reading.consumedAh;
            camp.battery.remainingMinutes = reading.remainingTimeAvailable ? reading.remainingMinutes : -1;
            camp.battery.connected = true;
            CampData::recalculateTotals();
            decoded = true;
          }
        } else {
          VictronChargerReading reading;
          if (VictronDecoder::decodeCharger(bytes, data.length(), key, reading)) {
            if (camp.system.dataSource == DATA_SOURCE_DEMO) {
              for (uint8_t i = 0; i < MAX_VICTRON_DEVICES; ++i) camp.chargers[i] = ChargerData{};
              camp.totalChargerPower = 0.0f;
              camp.totalChargerCurrent = 0.0f;
            }
            int slot = -1;
            for (uint8_t i = 0; i < MAX_VICTRON_DEVICES; ++i) {
              if (camp.chargers[i].provisioned && strcmp(camp.chargers[i].address, victronBleStatus.address) == 0) { slot = i; break; }
            }
            if (slot < 0) for (uint8_t i = 0; i < MAX_VICTRON_DEVICES; ++i) if (!camp.chargers[i].provisioned) { slot = i; break; }
            if (slot >= 0) {
              ChargerData &device = camp.chargers[slot];
              device.provisioned = true;
              device.connected = true;
              strlcpy(device.address, victronBleStatus.address, sizeof(device.address));
              const char *typeName = VictronDecoder::recordTypeName(reading.recordType);
              strlcpy(device.name, typeName, sizeof(device.name));
              device.chargeState = reading.state;
              device.inputVoltage = reading.inputVoltageAvailable ? reading.inputVoltage : 0.0f;
              device.batteryVoltage = reading.outputVoltageAvailable ? reading.outputVoltage : 0.0f;
              device.chargeCurrent = reading.outputCurrentAvailable ? reading.outputCurrent : 0.0f;
              if (reading.solarPowerAvailable) device.chargePower = reading.solarPower;
              else if (reading.outputVoltageAvailable && reading.outputCurrentAvailable) device.chargePower = reading.outputVoltage * reading.outputCurrent;
              else device.chargePower = 0.0f;
              device.hasChargerData = reading.recordType == VICTRON_RECORD_SOLAR_CHARGER ||
                                      reading.recordType == VICTRON_RECORD_DCDC_CONVERTER ||
                                      reading.recordType == VICTRON_RECORD_ORION_XS ||
                                      reading.recordType == VICTRON_RECORD_AC_CHARGER;
              device.hasLoadData = reading.loadCurrentAvailable || reading.recordType == VICTRON_RECORD_SMART_BATTERY_PROTECT;
              if (reading.loadCurrentAvailable) {
                device.loadCurrent = reading.loadCurrent;
                device.loadPower = reading.loadCurrent * (reading.outputVoltageAvailable ? reading.outputVoltage : camp.battery.voltage);
              } else {
                device.loadCurrent = 0.0f;
                device.loadPower = 0.0f;
              }
              device.loadOutputOn = reading.outputStateAvailable ? reading.outputOn : reading.loadCurrentAvailable;
              CampData::recalculateTotals();
              decoded = true;
            }
          }
        }

        setupPortal.noteDecodeResult(victronBleStatus.address, decoded);
        if (decoded) {
          camp.system.bleConnected = true;
          camp.system.rejectedPackets = victronBleStatus.rejectedPacketCount;
          camp.system.dataSource = DATA_SOURCE_LIVE;
          camp.system.lastLiveDataMs = millis();
          victronBleStatus.liveData = true;
          victronBleStatus.keyRejected = false;
          victronBleStatus.decodedPacketCount++;
          victronBleStatus.lastDecodedMs = millis();
        } else {
          victronBleStatus.rejectedPacketCount++;
          camp.system.rejectedPackets = victronBleStatus.rejectedPacketCount;
          if (victronBleStatus.rejectedPacketCount >= 3 && victronBleStatus.decodedPacketCount == 0) victronBleStatus.keyRejected = true;
        }
      }
    }

#if DEV_MODE
    Serial.printf("Victron: %s RSSI %d packets %lu decoded %lu key %s\n",
                  victronBleStatus.address, victronBleStatus.rssi,
                  static_cast<unsigned long>(victronBleStatus.packetCount),
                  static_cast<unsigned long>(victronBleStatus.decodedPacketCount),
                  victronBleStatus.keyLoaded ? (victronBleStatus.keyRejected ? "rejected" : "loaded") : "missing");
#endif
  }
};

VictronAdvertisedCallbacks advertisedCallbacks;
void scanCompleteCallback(BLEScanResults results) {
  BLEDevice::getScan()->clearResults();
  scanFinished = true;
}
}

void VictronBleScanner::begin() {
  BLEDevice::init("CampMonitor");
  _scan = BLEDevice::getScan();
  if (_scan == nullptr) {
    victronBleStatus.scannerReady = false;
    strlcpy(victronBleStatus.name, "BLE init failed", sizeof(victronBleStatus.name));
    return;
  }
  _scan->setAdvertisedDeviceCallbacks(&advertisedCallbacks, true);
  _scan->setActiveScan(true);
  _scan->setInterval(160);
  _scan->setWindow(144);
  victronBleStatus.scannerReady = true;
  startScan();
}

void VictronBleScanner::startScan() {
  if (_scan == nullptr || _scanRunning) return;
  scanFinished = false;
  _scanRunning = true;
  _scan->start(BLE_SCAN_SECONDS, scanCompleteCallback, false);
}

void VictronBleScanner::update() {
  if (_scan == nullptr) return;
  if (_scanRunning && scanFinished) {
    _scanRunning = false;
    _lastScanFinishedMs = millis();
  }
  const uint32_t now = millis();
  if (victronBleStatus.liveData && now - victronBleStatus.lastDecodedMs > LIVE_DATA_TIMEOUT_MS) {
    victronBleStatus.liveData = false;
    // Keep the last valid readings visible briefly while reconnecting.
    CampData::current.system.bleConnected = false;
    CampData::current.system.dataSource = DATA_SOURCE_LOST;
  }

  // After five minutes without decoded live data, switch to the animated demo.
  // This applies both at startup and after a disconnect. Valid packets always
  // replace the demo immediately in the advertisement callback above.
  const DataSourceMode mode = CampData::current.system.dataSource;
  const uint32_t noLiveForMs = CampData::current.system.lastLiveDataMs == 0
    ? now
    : now - CampData::current.system.lastLiveDataMs;
  if ((mode == DATA_SOURCE_WAITING || mode == DATA_SOURCE_LOST) &&
      noLiveForMs >= DEMO_FALLBACK_TIMEOUT_MS) {
    CampData::beginDemoValues();
  }

  if (!_scanRunning && now - _lastScanFinishedMs >= BLE_SCAN_PAUSE_MS) startScan();
}

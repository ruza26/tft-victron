#pragma once
#include <Arduino.h>
#include <BLEDevice.h>

struct VictronBleStatus {
  bool scannerReady = false;
  bool deviceSeen = false;
  bool instantReadoutSeen = false;
  bool liveData = false;
  bool keyLoaded = false;
  bool keyRejected = false;
  char name[32] = "Searching...";
  char address[20] = "--";
  int rssi = -127;
  uint32_t packetCount = 0;
  uint32_t decodedPacketCount = 0;
  uint32_t rejectedPacketCount = 0;
  uint32_t lastSeenMs = 0;
  uint32_t lastDecodedMs = 0;
};

extern VictronBleStatus victronBleStatus;

class VictronBleScanner {
public:
  void begin();
  void update();

private:
  BLEScan *_scan = nullptr;
  uint32_t _lastScanFinishedMs = 0;
  bool _scanRunning = false;
  void startScan();
};

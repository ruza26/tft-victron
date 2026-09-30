#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <FS.h>
using fs::FS;
#include <WebServer.h>
#include <Update.h>
#include "Config.h"

struct ProvisionedVictronDevice {
  bool used = false;
  char name[32] = "Victron device";
  char address[20] = "";
  int rssi = -127;
  bool instantReadoutSeen = false;
  bool keyStored = false;
  bool liveData = false;
  bool keyRejected = false;
  uint32_t packetCount = 0;
  uint32_t decodedPacketCount = 0;
  uint32_t rejectedPacketCount = 0;
  uint32_t lastSeenMs = 0;
  uint32_t lastDecodedMs = 0;
  uint8_t recordType = 0xFF;
  uint8_t packetLength = 0;
  bool keyIdentifierMatch = false;
};

class SetupPortal {
public:
  void begin();
  void update();
  void notifyVictronDevice(const char *name, const char *address, int rssi);
  void notifyBleName(const char *name, const char *address);
  void noteInstantReadout(const char *address);
  void notePacketDetails(const char *address, uint8_t recordType, size_t packetLength, bool keyIdentifierMatch);
  void noteDecodeResult(const char *address, bool success);
  void start();
  void stop();

  bool active() const { return _active; }
  String keyForAddress(const char *address);
  const char *ssid() const { return _ssid.c_str(); }
  String statusText();
  bool otaInProgress() const { return _otaInProgress; }
  uint8_t otaProgress() const { return _otaProgress; }
  uint8_t deviceCount() const;
  const ProvisionedVictronDevice *deviceAt(uint8_t index) const;

private:
  WebServer _server{80};
  Preferences _preferences;
  bool _active = false;
  bool _routesReady = false;
  bool _autoStartedThisBoot = false;
  uint32_t _startedMs = 0;
  String _ssid;
  String _lastMessage;
  bool _otaInProgress = false;
  bool _restartPending = false;
  bool _uploadFailed = false;
  uint8_t _otaProgress = 0;
  uint32_t _restartAtMs = 0;
  ProvisionedVictronDevice _devices[MAX_VICTRON_DEVICES];

  void configureRoutes();
  void handleRoot();
  void handleUpdatePage();
  void handleUpdateUpload();
  void handleUpdateFinished();
  void handleSave();
  void handleRemove();
  void handleNotFound();
  int findDevice(const char *address) const;
  int ensureDevice(const char *address);
  String deviceStorageKey(const String &address) const;
  String legacyStorageKey(const String &address) const;
  String nameStorageKey(const String &address) const;
  static String htmlEscape(const String &value);
  static String normaliseHexKey(String value);
  static String normaliseAddress(String value);
};

extern SetupPortal setupPortal;

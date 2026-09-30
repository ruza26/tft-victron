#include "SetupPortal.h"
#include "Config.h"
#include "DataModel.h"
#include "VictronDecoder.h"
#include <WiFi.h>
#include <esp_system.h>
#include <ctype.h>
#include <string.h>

SetupPortal setupPortal;

namespace {
const char PAGE_HEAD[] PROGMEM = R"HTML(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>CampMonitor Maintenance</title><style>
body{font-family:system-ui,sans-serif;background:#101419;color:#f3f6f8;margin:0;padding:18px}
main{max-width:680px;margin:auto}h1{margin:0 0 8px;font-size:1.55rem}.muted{color:#aeb9c4}.device{background:#1c242c;border:1px solid #34404b;border-radius:14px;padding:16px;margin:14px 0}
.name{font-size:1.18rem;font-weight:750}.meta{margin-top:5px;color:#aeb9c4}.status{display:inline-block;margin-top:9px;padding:5px 9px;border-radius:999px;background:#26343e}.live{background:#193b2c}.warn{background:#4a3b1f}.bad{background:#492b2b}
label{display:block;margin:14px 0 7px}input{box-sizing:border-box;width:100%;font:inherit;padding:12px;border-radius:9px;border:1px solid #687582;background:#0c1116;color:white}
.actions{display:grid;grid-template-columns:1fr auto;gap:9px;align-items:end}button,.button{display:inline-block;padding:12px 15px;border:0;border-radius:9px;font:inherit;font-weight:700;background:#35b779;color:#07140d;text-decoration:none}.remove{background:#7d3b3b;color:white}.ota{background:#2f86d6;color:white}.notice{margin:14px 0;padding:11px;border-radius:8px;background:#26343e}code{overflow-wrap:anywhere}@media(max-width:480px){.actions{grid-template-columns:1fr}.remove{width:100%}}
</style></head><body><main>
)HTML";
}

void SetupPortal::begin() {
  _preferences.begin("victron", false);
  uint32_t chip = static_cast<uint32_t>(ESP.getEfuseMac());
  char suffix[7];
  snprintf(suffix, sizeof(suffix), "%06lX", static_cast<unsigned long>(chip & 0xFFFFFF));
  _ssid = String("CampMonitor-") + suffix;
}

String SetupPortal::normaliseAddress(String value) {
  value.trim();
  value.toUpperCase();
  return value;
}

int SetupPortal::findDevice(const char *address) const {
  if (!address || !address[0]) return -1;
  String wanted = normaliseAddress(String(address));
  for (uint8_t i = 0; i < MAX_VICTRON_DEVICES; ++i) {
    if (_devices[i].used && wanted.equalsIgnoreCase(_devices[i].address)) return i;
  }
  return -1;
}

int SetupPortal::ensureDevice(const char *address) {
  int existing = findDevice(address);
  if (existing >= 0) return existing;
  for (uint8_t i = 0; i < MAX_VICTRON_DEVICES; ++i) {
    if (!_devices[i].used) {
      _devices[i] = ProvisionedVictronDevice{};
      _devices[i].used = true;
      String a = normaliseAddress(String(address));
      strlcpy(_devices[i].address, a.c_str(), sizeof(_devices[i].address));
      String savedName = _preferences.getString(nameStorageKey(a).c_str(), "");
      if (!savedName.isEmpty()) strlcpy(_devices[i].name, savedName.c_str(), sizeof(_devices[i].name));
      _devices[i].keyStored = keyForAddress(a.c_str()).length() == 32;
      return i;
    }
  }
  return -1;
}

void SetupPortal::notifyVictronDevice(const char *name, const char *address, int rssi) {
  if (!address || !address[0]) return;
  int index = ensureDevice(address);
  if (index < 0) return;
  ProvisionedVictronDevice &d = _devices[index];
  d.rssi = rssi;
  d.packetCount++;
  d.lastSeenMs = millis();
  if (name && name[0] && strcmp(name, "Victron device") != 0) {
    strlcpy(d.name, name, sizeof(d.name));
    _preferences.putString(nameStorageKey(String(d.address)).c_str(), d.name);
  }
  d.keyStored = keyForAddress(d.address).length() == 32;

  if (!_active && !_autoStartedThisBoot && !d.keyStored) {
    _autoStartedThisBoot = true;
    start();
  }
}

void SetupPortal::notifyBleName(const char *name, const char *address) {
  if (!name || !name[0] || !address || !address[0]) return;
  int index = findDevice(address);
  if (index < 0) return;
  strlcpy(_devices[index].name, name, sizeof(_devices[index].name));
  _preferences.putString(nameStorageKey(String(_devices[index].address)).c_str(), _devices[index].name);
}

void SetupPortal::noteInstantReadout(const char *address) {
  int index = ensureDevice(address);
  if (index >= 0) _devices[index].instantReadoutSeen = true;
}


void SetupPortal::notePacketDetails(const char *address, uint8_t recordType, size_t packetLength, bool keyIdentifierMatch) {
  int index = ensureDevice(address);
  if (index < 0) return;
  ProvisionedVictronDevice &d = _devices[index];
  d.recordType = recordType;
  d.packetLength = packetLength > 255 ? 255 : static_cast<uint8_t>(packetLength);
  d.keyIdentifierMatch = keyIdentifierMatch;
}

void SetupPortal::noteDecodeResult(const char *address, bool success) {
  int index = ensureDevice(address);
  if (index < 0) return;
  ProvisionedVictronDevice &d = _devices[index];
  d.keyStored = keyForAddress(address).length() == 32;
  if (success) {
    d.liveData = true;
    d.keyRejected = false;
    d.decodedPacketCount++;
    d.lastDecodedMs = millis();
  } else {
    d.rejectedPacketCount++;
    if (d.keyStored && d.rejectedPacketCount >= 3 && d.decodedPacketCount == 0) d.keyRejected = true;
  }
}

void SetupPortal::start() {
  if (_active) return;
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(_ssid.c_str())) {
    _lastMessage = "Could not start setup Wi-Fi.";
    return;
  }
  configureRoutes();
  _server.begin();
  _startedMs = millis();
  _active = true;
  CampData::current.system.wifiConnected = true;
  _lastMessage = "Setup portal ready.";
  Serial.printf("Open setup AP: %s address: http://192.168.4.1\n", _ssid.c_str());
}

void SetupPortal::stop() {
  if (!_active) return;
  _server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  _active = false;
  CampData::current.system.wifiConnected = false;
}

void SetupPortal::update() {
  if (_restartPending && static_cast<int32_t>(millis() - _restartAtMs) >= 0) {
    ESP.restart();
  }
  if (!_active) return;
  _server.handleClient();
  if (!_otaInProgress && millis() - _startedMs >= SETUP_PORTAL_TIMEOUT_MS) stop();
  for (uint8_t i = 0; i < MAX_VICTRON_DEVICES; ++i) {
    if (_devices[i].used && _devices[i].liveData && millis() - _devices[i].lastDecodedMs > LIVE_DATA_TIMEOUT_MS) _devices[i].liveData = false;
  }
}

String SetupPortal::keyForAddress(const char *address) {
  if (!address || !address[0]) return "";
  String a = normaliseAddress(String(address));
  String key = _preferences.getString(deviceStorageKey(a).c_str(), "");
  if (key.length() == 32) return key;
  // Alpha 8 used only the last 11 MAC nibbles. Migrate it once when found.
  String legacy = _preferences.getString(legacyStorageKey(a).c_str(), "");
  if (legacy.length() == 32) {
    _preferences.putString(deviceStorageKey(a).c_str(), legacy);
    return legacy;
  }
  return "";
}

uint8_t SetupPortal::deviceCount() const {
  uint8_t count = 0;
  for (uint8_t i = 0; i < MAX_VICTRON_DEVICES; ++i) if (_devices[i].used) ++count;
  return count;
}

const ProvisionedVictronDevice *SetupPortal::deviceAt(uint8_t index) const {
  uint8_t seen = 0;
  for (uint8_t i = 0; i < MAX_VICTRON_DEVICES; ++i) {
    if (!_devices[i].used) continue;
    if (seen++ == index) return &_devices[i];
  }
  return nullptr;
}

String SetupPortal::statusText() {
  if (_active) return String("AP active - ") + String(deviceCount()) + " device(s)";
  if (deviceCount() > 0) return String(deviceCount()) + " Victron device(s) detected";
  return _lastMessage.length() ? _lastMessage : "Waiting for Victron devices";
}

void SetupPortal::configureRoutes() {
  if (_routesReady) return;
  _server.on("/", HTTP_GET, [this]() { handleRoot(); });
  _server.on("/save", HTTP_POST, [this]() { handleSave(); });
  _server.on("/remove", HTTP_POST, [this]() { handleRemove(); });
  _server.on("/update", HTTP_GET, [this]() { handleUpdatePage(); });
  _server.on("/update", HTTP_POST,
    [this]() { handleUpdateFinished(); },
    [this]() { handleUpdateUpload(); });
  _server.on("/generate_204", HTTP_ANY, [this]() { _server.sendHeader("Location", "/", true); _server.send(302, "text/plain", ""); });
  _server.on("/hotspot-detect.html", HTTP_ANY, [this]() { handleRoot(); });
  _server.onNotFound([this]() { handleNotFound(); });
  _routesReady = true;
}

void SetupPortal::handleRoot() {
  String page = FPSTR(PAGE_HEAD);
  page += "<h1>CampMonitor Maintenance</h1><p class=muted>Configure Victron keys or install a firmware update without storing Wi-Fi credentials.</p>";
  page += "<section class=device><div class=name>Firmware " APP_VERSION "</div><p class=muted>Upload an exported Arduino .bin file. CampMonitor will restart automatically after a successful update.</p><a class=\"button ota\" href=/update>Open firmware update</a></section>";
  if (_lastMessage.length()) page += "<div class=notice>" + htmlEscape(_lastMessage) + "</div>";
  const uint8_t count = deviceCount();
  if (count == 0) {
    page += "<div class=device><div class=name>Scanning...</div><p class=muted>No Victron devices detected yet. Keep this page open.</p></div>";
  }
  for (uint8_t i = 0; i < count; ++i) {
    const ProvisionedVictronDevice *d = deviceAt(i);
    if (!d) continue;
    page += "<section class=device><div class=name>" + htmlEscape(String(d->name)) + "</div>";
    page += "<div class=meta><code>" + htmlEscape(String(d->address)) + "</code> &nbsp; RSSI " + String(d->rssi) + " dBm</div>";
    if (d->recordType != 0xFF) {
      page += "<div class=meta>Type: " + htmlEscape(String(VictronDecoder::recordTypeName(d->recordType))) + " (0x";
      if (d->recordType < 16) page += "0";
      page += String(d->recordType, HEX) + ") &nbsp; Packet: " + String(d->packetLength) + " bytes</div>";
      if (d->keyStored) page += String("<div class=meta>Key identifier: ") + (d->keyIdentifierMatch ? "matched" : "not matched") + "</div>";
    }
    String statusClass = "status";
    String status = "Detected";
    if (d->liveData) { statusClass += " live"; status = "Live data"; }
    else if (d->keyRejected) { statusClass += " bad"; status = "Key rejected"; }
    else if (d->keyStored) { statusClass += " warn"; status = "Key stored - waiting for decode"; }
    else if (d->instantReadoutSeen) { statusClass += " warn"; status = "Key required"; }
    page += "<div class=\"" + statusClass + "\">" + status + "</div>";
    page += "<form method=post action=/save><input type=hidden name=address value=\"" + htmlEscape(String(d->address)) + "\">";
    page += "<label>Instant Readout encryption key</label><div class=actions><input name=key inputmode=text autocomplete=off autocapitalize=off spellcheck=false maxlength=80 placeholder=\"32 hexadecimal characters\" required>";
    page += "<button type=submit>" + String(d->keyStored ? "Replace key" : "Save key") + "</button></div></form>";
    if (d->keyStored) {
      page += "<form method=post action=/remove><input type=hidden name=address value=\"" + htmlEscape(String(d->address)) + "\"><button class=remove type=submit>Remove saved key</button></form>";
    }
    page += "</section>";
  }
  page += "<p class=muted>Device names such as Camp Solar and Camp Shunt are read from BLE advertising when available. Refresh the page to update the list.</p></main></body></html>";
  _server.send(200, "text/html", page);
}


void SetupPortal::handleUpdatePage() {
  String page = FPSTR(PAGE_HEAD);
  page += "<h1>CampMonitor Firmware Update</h1>";
  page += "<p class=muted>Current firmware: " APP_VERSION "</p>";
  page += "<section class=device><form method=POST action=/update enctype=multipart/form-data>";
  page += "<label>Select exported firmware (.bin)</label><input type=file name=firmware accept=.bin,application/octet-stream required>";
  page += "<p class=muted>Do not remove power during the upload. The monitor will reboot when finished.</p>";
  page += "<button class=ota type=submit>Install update</button></form></section>";
  page += "<p><a class=button href=/>Back to maintenance</a></p></main></body></html>";
  _server.send(200, "text/html", page);
}

void SetupPortal::handleUpdateUpload() {
  HTTPUpload &upload = _server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    _otaInProgress = true;
    _uploadFailed = false;
    _otaProgress = 0;
    _lastMessage = "Firmware upload started.";
    Serial.printf("OTA upload: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      _uploadFailed = true;
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!_uploadFailed) {
      const size_t written = Update.write(upload.buf, upload.currentSize);
      if (written != upload.currentSize) {
        _uploadFailed = true;
        Update.printError(Serial);
      }
      if (upload.totalSize > 0) {
        // Typical firmware is under two megabytes; this provides useful progress
        // without relying on the browser supplying a content length.
        const uint32_t estimate = 1900000UL;
        const uint32_t percent = (upload.totalSize * 100UL) / estimate;
        _otaProgress = percent > 99UL ? 99U : static_cast<uint8_t>(percent);
      }
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (!_uploadFailed && Update.end(true)) {
      _otaProgress = 100;
      _lastMessage = "Firmware installed. Restarting...";
      Serial.printf("OTA complete: %u bytes\n", static_cast<unsigned>(upload.totalSize));
    } else {
      _uploadFailed = true;
      _lastMessage = "Firmware update failed.";
      Update.printError(Serial);
    }
    _otaInProgress = false;
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    _uploadFailed = true;
    _otaInProgress = false;
    _lastMessage = "Firmware upload cancelled.";
  }
}

void SetupPortal::handleUpdateFinished() {
  const bool success = !_uploadFailed && !Update.hasError() && _otaProgress == 100;
  String page = FPSTR(PAGE_HEAD);
  if (success) {
    page += "<h1>Update complete</h1><div class=notice>CampMonitor is restarting with the new firmware.</div>";
    page += "<p class=muted>You can disconnect from the CampMonitor Wi-Fi network.</p>";
    _server.send(200, "text/html", page + "</main></body></html>");
    _restartPending = true;
    _restartAtMs = millis() + 1500UL;
  } else {
    page += "<h1>Update failed</h1><div class=notice>The firmware was not installed. Check that you selected the correct exported .bin file.</div>";
    page += "<p><a class=button href=/update>Try again</a></p>";
    _server.send(500, "text/html", page + "</main></body></html>");
  }
}

void SetupPortal::handleSave() {
  if (!_server.hasArg("address") || !_server.hasArg("key")) {
    _lastMessage = "Device address or key missing.";
  } else {
    String address = normaliseAddress(_server.arg("address"));
    String key = normaliseHexKey(_server.arg("key"));
    int index = findDevice(address.c_str());
    if (index < 0) _lastMessage = "That device is no longer in the detected list.";
    else if (key.length() != 32) _lastMessage = "Invalid key: expected exactly 32 hexadecimal characters.";
    else {
      _preferences.putString(deviceStorageKey(address).c_str(), key);
      _devices[index].keyStored = true;
      _devices[index].keyRejected = false;
      _devices[index].rejectedPacketCount = 0;
      _lastMessage = String("Key saved for ") + _devices[index].name + " (" + address + ").";
    }
  }
  _server.sendHeader("Location", "/", true);
  _server.send(303, "text/plain", "");
}

void SetupPortal::handleRemove() {
  if (_server.hasArg("address")) {
    String address = normaliseAddress(_server.arg("address"));
    _preferences.remove(deviceStorageKey(address).c_str());
    _preferences.remove(legacyStorageKey(address).c_str());
    int index = findDevice(address.c_str());
    if (index >= 0) {
      _devices[index].keyStored = false;
      _devices[index].keyRejected = false;
      _devices[index].liveData = false;
      _devices[index].decodedPacketCount = 0;
      _devices[index].rejectedPacketCount = 0;
      _lastMessage = String("Saved key removed for ") + _devices[index].name + ".";
    }
  }
  _server.sendHeader("Location", "/", true);
  _server.send(303, "text/plain", "");
}

void SetupPortal::handleNotFound() {
  _server.sendHeader("Location", "/", true);
  _server.send(302, "text/plain", "");
}

String SetupPortal::deviceStorageKey(const String &address) const {
  String compact = address;
  compact.replace(":", ""); compact.toLowerCase();
  return "k" + compact; // 13 characters, within the ESP32 Preferences limit.
}

String SetupPortal::legacyStorageKey(const String &address) const {
  String compact = address;
  compact.replace(":", ""); compact.toLowerCase();
  if (compact.length() > 11) compact = compact.substring(compact.length() - 11);
  return "k" + compact;
}

String SetupPortal::nameStorageKey(const String &address) const {
  String compact = address;
  compact.replace(":", ""); compact.toLowerCase();
  return "n" + compact;
}

String SetupPortal::htmlEscape(const String &value) {
  String escaped = value;
  escaped.replace("&", "&amp;"); escaped.replace("<", "&lt;"); escaped.replace(">", "&gt;"); escaped.replace("\"", "&quot;");
  return escaped;
}

String SetupPortal::normaliseHexKey(String value) {
  value.trim(); value.replace(" ", ""); value.replace(":", ""); value.replace("-", "");
  if (value.startsWith("0x") || value.startsWith("0X")) value.remove(0, 2);
  value.toLowerCase();
  for (size_t i = 0; i < value.length(); ++i) if (!isxdigit(static_cast<unsigned char>(value.charAt(i)))) return "";
  return value;
}

#include "PageManager.h"
#include "Config.h"
#include "Theme.h"
#include "Widgets.h"
#include "DataModel.h"
#include <math.h>
#include "VictronBleScanner.h"
#include "SetupPortal.h"
#include "RtcManager.h"
#include "EnergyManager.h"
#include "HistoryManager.h"

PageManager::PageManager(DisplayManager &display)
  : _display(display), _currentPage(PAGE_DASHBOARD),
    _needsFullRedraw(true), _lastRefreshMs(0), _chargerPage(0), _loadPage(0), _historyPage(0),
    _lastDataSource(DATA_SOURCE_WAITING), _lastOverlaySecond(UINT32_MAX),
    _lastPageSignature(0), _lastBatteryBlinkOn(true),
    _lastSolarAh(NAN), _lastChargedAh(NAN), _lastUsedAh(NAN),
    _lastClockSecond(UINT32_MAX), _lastHeaderSecond(UINT32_MAX), _editYear(2026), _editMonth(1), _editDay(1),
    _editHour(0), _editMinute(0), _editRolloverHour(6), _clockEditing(false), _showThemePage(false) {
  resetCache();
}

void PageManager::begin() {
  resetCache();
  _needsFullRedraw = true;
}

void PageManager::resetCache() {
  _lastSoc = NAN;
  _lastVoltage = NAN;
  _lastCurrent = NAN;
  _lastBatteryPower = NAN;
  _lastChargerPower = NAN;
  _lastChargerCurrent = NAN;
  _lastLoadPower = NAN;
  _lastLoadCurrent = NAN;
  _lastConsumedAh = NAN;
  _lastRemainingMinutes = -1;
  _lastBatteryBlinkOn = true;
  _lastDataSource = CampData::current.system.dataSource;
  _lastOverlaySecond = UINT32_MAX;
  _lastPageSignature = 0;
  _lastPageTotalPower = NAN;
  _lastPageTotalCurrent = NAN;
  _lastSolarAh = NAN;
  _lastChargedAh = NAN;
  _lastUsedAh = NAN;
  _lastClockSecond = UINT32_MAX;
  _lastHeaderSecond = UINT32_MAX;
  resetDeviceRowCaches();
}

void PageManager::show(PageId page) {
  if (page >= PAGE_COUNT) return;
  _currentPage = page;
  _showThemePage = false;
  if (page == PAGE_CLOCK) beginClockEdit();
  if (page == PAGE_HISTORY) _historyPage = 0;
  resetCache();
  _needsFullRedraw = true;
}

PageId PageManager::currentPage() const { return _currentPage; }

void PageManager::requestRedraw() {
  resetCache();
  _needsFullRedraw = true;
}

void PageManager::update() {
  const uint32_t now = millis();
  const DataSourceMode mode = CampData::current.system.dataSource;

  // Changing between SEARCHING/LIVE/LOST/DEMO changes the status badge, so repaint once.
  if (mode != _lastDataSource) {
    _lastDataSource = mode;
    _needsFullRedraw = true;
  }

  if (_needsFullRedraw) {
    _display.screen().startWrite();
    drawCurrentPage();
    _display.screen().endWrite();
    _needsFullRedraw = false;
    _lastRefreshMs = now;
    _lastPageSignature = currentPageSignature();
    return;
  }

  if (now - _lastRefreshMs < UI_UPDATE_MS) return;
  _lastRefreshMs = now;

  TFT_eSPI &tft = _display.screen();
  tft.startWrite();

  if (_currentPage == PAGE_DASHBOARD) {
    updateDashboard();
  } else if (_currentPage == PAGE_CHARGERS) {
    updateChargers();
  } else if (_currentPage == PAGE_LOADS) {
    updateLoads();
  } else if (_currentPage == PAGE_BATTERY) {
    updateBattery();
  } else if (_currentPage == PAGE_ENERGY) {
    updateEnergy();
  } else if (_currentPage == PAGE_HISTORY) {
    // History is EEPROM-backed and changes only at rollover or page navigation.
  } else if (_currentPage == PAGE_CLOCK) {
    updateClock();
  } else {
    if (_currentPage == PAGE_STATUS && _showThemePage) {
      tft.endWrite();
      return;
    }
    // Status is diagnostic-only, so a full repaint is acceptable when its
    // meaningful state changes. Fast counters/RSSI remain excluded.
    const uint32_t signature = currentPageSignature();
    if (signature != _lastPageSignature) {
      drawStatus();
      _lastPageSignature = signature;
    }
  }

  // Refresh the enlarged clock and date once per minute without repainting
  // the rest of the page.
  if (rtcManager.available()) {
    const DateTime headerNow = rtcManager.now();
    const uint32_t headerMinute = headerNow.unixtime() / 60UL;
    if (headerMinute != _lastHeaderSecond) {
      Widgets::drawHeaderTime(tft);
      _lastHeaderSecond = headerMinute;
    }
  }

  // LOST elapsed time changes once per second; the other badges are static.
  const uint32_t overlaySecond = now / 1000UL;
  if (mode == DATA_SOURCE_LOST && overlaySecond != _lastOverlaySecond) {
    drawModeOverlay();
    _lastOverlaySecond = overlaySecond;
  }

  tft.endWrite();
}
void PageManager::handleTouch(int16_t x, int16_t y) {
  int16_t navTop = _display.height() - NAV_HEIGHT;

  // Status -> Display Theme subpage. The four previews apply immediately and
  // are saved to Preferences so the choice survives a full power cycle.
  if (_currentPage == PAGE_STATUS && _showThemePage && y < navTop) {
    if (y >= 50 && y < 116) {
      Theme::apply(x < (_display.width() / 2) ? THEME_LIGHT : THEME_DARK);
      show(PAGE_DASHBOARD);
      return;
    }
    if (y >= 122 && y < 188) {
      Theme::apply(x < (_display.width() / 2) ? THEME_VICTRON : THEME_GLASS);
      show(PAGE_DASHBOARD);
      return;
    }
    return;
  }

  if (_currentPage == PAGE_STATUS && !_showThemePage && y >= 208 && y <= 246) {
    _showThemePage = true;
    _needsFullRedraw = true;
    return;
  }

  if (_currentPage == PAGE_CLOCK && y < navTop) {
    // Six compact rows: day, month, year, hour, minute and 06:00 rollover.
    if (y >= 64 && y < 226) {
      const uint8_t field = constrain((y - 64) / 27, 0, 5);
      if (x <= 92) adjustClockField(field, -1);
      else if (x >= _display.width() - 92) adjustClockField(field, 1);
      _needsFullRedraw = true;
      return;
    }
    if (y >= 228 && y < navTop) {
      saveClockEdit();
      _needsFullRedraw = true;
      return;
    }
  }

  if (_currentPage == PAGE_HISTORY && y < navTop) {
    const uint8_t pageCount = historyManager.count() == 0 ? 1 : (historyManager.count() + 4) / 5;
    _historyPage = (_historyPage + 1) % pageCount;
    _needsFullRedraw = true;
    return;
  }

  if (_currentPage == PAGE_STATUS && !_showThemePage && y >= 158 && y <= 202) {
    if (setupPortal.active()) setupPortal.stop();
    else setupPortal.start();
    _needsFullRedraw = true;
    return;
  }

  if (y < navTop) {
    if (_currentPage == PAGE_CHARGERS && CampData::chargerCount() > DEVICES_PER_PAGE) {
      uint8_t pages = (CampData::chargerCount() + DEVICES_PER_PAGE - 1) / DEVICES_PER_PAGE;
      _chargerPage = (_chargerPage + 1) % pages;
      _needsFullRedraw = true;
    } else if (_currentPage == PAGE_LOADS && CampData::loadMonitorCount() > DEVICES_PER_PAGE) {
      uint8_t pages = (CampData::loadMonitorCount() + DEVICES_PER_PAGE - 1) / DEVICES_PER_PAGE;
      _loadPage = (_loadPage + 1) % pages;
      _needsFullRedraw = true;
    }
    return;
  }

  int16_t itemWidth = _display.width() / PAGE_COUNT;
  int16_t touchedIndex = constrain(x / itemWidth, 0, PAGE_COUNT - 1);
  show(static_cast<PageId>(touchedIndex));
}

void PageManager::drawCurrentPage() {
  switch (_currentPage) {
    case PAGE_DASHBOARD: drawDashboard(); break;
    case PAGE_CHARGERS: drawChargers(); break;
    case PAGE_LOADS: drawLoads(); break;
    case PAGE_BATTERY: drawBattery(); break;
    case PAGE_ENERGY: drawEnergy(); break;
    case PAGE_HISTORY: drawHistory(); break;
    case PAGE_CLOCK: drawClock(); break;
    case PAGE_STATUS: if (_showThemePage) drawThemePage(); else drawStatus(); break;
    default: drawDashboard(); break;
  }
  drawModeOverlay();
}

void PageManager::drawModeOverlay() {
  TFT_eSPI &tft = _display.screen();
  DataSourceMode mode = CampData::current.system.dataSource;

  if (mode == DATA_SOURCE_DEMO) {
    tft.fillRoundRect(tft.width() - 75, HEADER_HEIGHT + 3, 67, 18, 5, Theme::batteryWarning);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(Theme::background, Theme::batteryWarning);
    tft.drawString("DEMO", tft.width() - 42, HEADER_HEIGHT + 12, 2);
    tft.setTextDatum(TL_DATUM);
  } else if (mode == DATA_SOURCE_LIVE) {
    tft.fillRoundRect(tft.width() - 75, HEADER_HEIGHT + 3, 67, 18, 5, Theme::batteryGood);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(Theme::background, Theme::batteryGood);
    tft.drawString("LIVE", tft.width() - 42, HEADER_HEIGHT + 12, 2);
    tft.setTextDatum(TL_DATUM);
  } else if (mode == DATA_SOURCE_WAITING) {
    tft.fillRoundRect(tft.width() - 91, HEADER_HEIGHT + 3, 83, 18, 5, Theme::solar);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(Theme::background, Theme::solar);
    tft.drawString("SEARCHING", tft.width() - 50, HEADER_HEIGHT + 12, 1);
    tft.setTextDatum(TL_DATUM);
  } else if (mode == DATA_SOURCE_LOST) {
    tft.fillRect(0, HEADER_HEIGHT, tft.width(), 18, Theme::batteryCritical);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(Theme::text, Theme::batteryCritical);
    uint32_t seconds = CampData::current.system.lastLiveDataMs == 0 ? 0 :
      (millis() - CampData::current.system.lastLiveDataMs) / 1000UL;
    char text[48];
    snprintf(text, sizeof(text), "LIVE DATA LOST - %lus", static_cast<unsigned long>(seconds));
    tft.drawString(text, tft.width() / 2, HEADER_HEIGHT + 9, 1);
    tft.setTextDatum(TL_DATUM);
  }
}

void PageManager::drawDashboard() {
  TFT_eSPI &tft = _display.screen();
  tft.fillScreen(Theme::background);
  Widgets::drawHeader(tft, APP_NAME);
  if (CampData::current.system.dataSource == DATA_SOURCE_WAITING) {
    Widgets::drawCard(tft, 8, 50, tft.width() - 16, 116, "WAITING FOR LIVE DATA");
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(Theme::solar, Theme::card);
    tft.drawString("Scanning for Victron devices", tft.width() / 2, 88, 2);
    tft.setTextColor(Theme::textMuted, Theme::card);
    char line[48];
    snprintf(line, sizeof(line), "%u detected  •  %u live", setupPortal.deviceCount(),
             victronBleStatus.decodedPacketCount > 0 ? 1 : 0);
    tft.drawString(line, tft.width() / 2, 114, 2);
    tft.drawString("Open Status to configure keys", tft.width() / 2, 142, 1);
    tft.setTextDatum(TL_DATUM);
    Widgets::drawNavigation(tft, _currentPage);
    return;
  }
  Widgets::drawBatteryCard(tft, 8, 44, tft.width() - 16, 102, CampData::current.battery);
  Widgets::drawPowerFlowCard(tft, 8, 152, tft.width() - 16, 92,
    CampData::current.totalChargerPower,
    CampData::current.totalChargerCurrent,
    CampData::current.battery.power,
    CampData::current.estimatedLoadPower,
    CampData::current.estimatedLoadCurrent);
  Widgets::drawNavigation(tft, _currentPage);
  _lastSoc = CampData::current.battery.soc;
  _lastVoltage = CampData::current.battery.voltage;
  _lastCurrent = CampData::current.battery.current;
  _lastBatteryPower = CampData::current.battery.power;
  _lastChargerPower = CampData::current.totalChargerPower;
  _lastChargerCurrent = CampData::current.totalChargerCurrent;
  _lastLoadPower = CampData::current.estimatedLoadPower;
  _lastLoadCurrent = CampData::current.estimatedLoadCurrent;
}

void PageManager::updateDashboard() {
  if (CampData::current.system.dataSource == DATA_SOURCE_WAITING) return;
  TFT_eSPI &tft = _display.screen();
  CampData &camp = CampData::current;

  // Redraw the battery card as one bounded region whenever any displayed
  // SmartShunt value changes. This keeps the layout responsive to the wider
  // 480-pixel display without fragile fixed text coordinates.
  const bool batteryBlinkOn = camp.battery.soc >= 20.0f || ((millis() / 500UL) & 1U) == 0U;
  const bool batteryChanged =
    batteryBlinkOn != _lastBatteryBlinkOn ||
    isnan(_lastSoc) || fabsf(camp.battery.soc - _lastSoc) >= 0.5f ||
    isnan(_lastVoltage) || fabsf(camp.battery.voltage - _lastVoltage) >= 0.01f ||
    isnan(_lastCurrent) || fabsf(camp.battery.current - _lastCurrent) >= 0.1f ||
    isnan(_lastBatteryPower) || fabsf(camp.battery.power - _lastBatteryPower) >= 1.0f;

  if (batteryChanged) {
    Widgets::drawBatteryCard(tft, 8, 44, tft.width() - 16, 102, camp.battery);
    _lastSoc = camp.battery.soc;
    _lastVoltage = camp.battery.voltage;
    _lastCurrent = camp.battery.current;
    _lastBatteryPower = camp.battery.power;
    _lastBatteryBlinkOn = batteryBlinkOn;
  }

  // Redraw the compact flow card only when one of its displayed values changes.
  // Solar values are the raw charger totals. Red load values are direct
  // SmartShunt discharge only; individual controller load outputs are shown separately.
  const bool flowChanged =
    isnan(_lastChargerPower) || fabsf(camp.totalChargerPower - _lastChargerPower) >= 1.0f ||
    isnan(_lastChargerCurrent) || fabsf(camp.totalChargerCurrent - _lastChargerCurrent) >= 0.1f ||
    isnan(_lastLoadPower) || fabsf(camp.estimatedLoadPower - _lastLoadPower) >= 1.0f ||
    isnan(_lastLoadCurrent) || fabsf(camp.estimatedLoadCurrent - _lastLoadCurrent) >= 0.1f ||
    batteryChanged;

  if (flowChanged) {
    Widgets::drawPowerFlowCard(tft, 8, 152, tft.width() - 16, 92,
      camp.totalChargerPower,
      camp.totalChargerCurrent,
      camp.battery.power,
      camp.estimatedLoadPower,
      camp.estimatedLoadCurrent);
    _lastChargerPower = camp.totalChargerPower;
    _lastChargerCurrent = camp.totalChargerCurrent;
    _lastLoadPower = camp.estimatedLoadPower;
    _lastLoadCurrent = camp.estimatedLoadCurrent;
  }

}
void PageManager::resetDeviceRowCaches() {
  for (uint8_t i = 0; i < DEVICES_PER_PAGE; ++i) {
    _chargerRows[i].deviceIndex = -1;
    _chargerRows[i].connected = false;
    _chargerRows[i].outputOn = false;
    _chargerRows[i].power = NAN;
    _chargerRows[i].current = NAN;
    _chargerRows[i].voltage = NAN;
    _chargerRows[i].state = 0xFF;
    _loadRows[i] = _chargerRows[i];
  }
}

uint8_t PageManager::collectVisibleChargers(int8_t indices[DEVICES_PER_PAGE]) const {
  for (uint8_t i = 0; i < DEVICES_PER_PAGE; ++i) indices[i] = -1;
  const uint8_t first = _chargerPage * DEVICES_PER_PAGE;
  uint8_t seen = 0;
  uint8_t count = 0;
  for (uint8_t i = 0; i < MAX_VICTRON_DEVICES && count < DEVICES_PER_PAGE; ++i) {
    const ChargerData &d = CampData::current.chargers[i];
    if (!d.provisioned || !d.hasChargerData) continue;
    if (seen++ < first) continue;
    indices[count++] = static_cast<int8_t>(i);
  }
  return count;
}

uint8_t PageManager::collectVisibleLoads(int8_t indices[DEVICES_PER_PAGE]) const {
  for (uint8_t i = 0; i < DEVICES_PER_PAGE; ++i) indices[i] = -1;
  const uint8_t first = _loadPage * DEVICES_PER_PAGE;
  uint8_t seen = 0;
  uint8_t count = 0;

  // -2 is a virtual row for the SmartShunt's directly reported discharge.
  if (CampData::current.battery.connected) {
    if (seen++ >= first && count < DEVICES_PER_PAGE) indices[count++] = -2;
  }

  for (uint8_t i = 0; i < MAX_VICTRON_DEVICES && count < DEVICES_PER_PAGE; ++i) {
    const ChargerData &d = CampData::current.chargers[i];
    if (!d.provisioned || !d.hasLoadData) continue;
    if (seen++ < first) continue;
    indices[count++] = static_cast<int8_t>(i);
  }
  return count;
}

void PageManager::drawChargerRowValue(uint8_t row, const ChargerData &d) {
  TFT_eSPI &tft = _display.screen();
  const int16_t y = 44 + row * 84;
  char line[64];
  if (!d.connected) snprintf(line, sizeof(line), "Disconnected");
  else snprintf(line, sizeof(line), "%.0fW %.1fA  Load %.1fA  %s", d.chargePower,
                d.chargeCurrent, d.loadCurrent, Widgets::chargeStateName(d.chargeState));
  tft.setTextDatum(MR_DATUM);
  tft.setTextPadding(tft.width() - 36);
  tft.setTextColor(d.connected ? Theme::solar : Theme::batteryCritical, Theme::card);
  tft.drawString(line, tft.width() - 18, y + 48, 2);
  tft.setTextPadding(0);
  tft.setTextDatum(TL_DATUM);
}

void PageManager::drawLoadRowValue(uint8_t row, const ChargerData &d) {
  TFT_eSPI &tft = _display.screen();
  const int16_t y = 44 + row * 84;
  char line[64];
  if (!d.connected) snprintf(line, sizeof(line), "Disconnected");
  else snprintf(line, sizeof(line), "%s   %.1fA   %.0fW", d.loadOutputOn ? "ON" : "OFF",
                d.loadCurrent, d.loadPower);
  tft.setTextDatum(MR_DATUM);
  tft.setTextPadding(tft.width() - 36);
  tft.setTextColor(d.connected && d.loadOutputOn ? Theme::load : Theme::textDim, Theme::card);
  tft.drawString(line, tft.width() - 18, y + 48, 2);
  tft.setTextPadding(0);
  tft.setTextDatum(TL_DATUM);
}

void PageManager::drawChargersFooter() {
  TFT_eSPI &tft = _display.screen();
  const uint8_t total = CampData::chargerCount();
  const uint8_t pageCount = total == 0 ? 1 : (total + DEVICES_PER_PAGE - 1) / DEVICES_PER_PAGE;
  char footer[48];
  if (pageCount > 1) snprintf(footer, sizeof(footer), "TOTAL %.1fA %.0fW  PAGE %u/%u - TAP",
    CampData::current.totalChargerCurrent, CampData::current.totalChargerPower, _chargerPage + 1, pageCount);
  else snprintf(footer, sizeof(footer), "INTO BATTERY %.1fA  %.0fW",
    CampData::current.totalChargerCurrent, CampData::current.totalChargerPower);
  tft.setTextDatum(MC_DATUM);
  tft.setTextPadding(tft.width() - 16);
  tft.setTextColor(Theme::solar, Theme::background);
  tft.drawString(footer, tft.width() / 2, tft.height() - NAV_HEIGHT - 13, pageCount > 1 ? 2 : 4);
  tft.setTextPadding(0);
  tft.setTextDatum(TL_DATUM);
}

void PageManager::drawLoadsFooter() {
  TFT_eSPI &tft = _display.screen();
  const uint8_t total = CampData::loadMonitorCount();
  const uint8_t pageCount = total == 0 ? 1 : (total + DEVICES_PER_PAGE - 1) / DEVICES_PER_PAGE;
  char footer[56];
  if (pageCount > 1) snprintf(footer, sizeof(footer), "%.1fA  %.0fW   PAGE %u/%u - TAP TO ADVANCE",
    CampData::current.estimatedLoadCurrent, CampData::current.estimatedLoadPower, _loadPage + 1, pageCount);
  else snprintf(footer, sizeof(footer), "CONSUMPTION %.1fA  %.0fW", CampData::current.estimatedLoadCurrent,
    CampData::current.estimatedLoadPower);
  tft.setTextDatum(MC_DATUM);
  tft.setTextPadding(tft.width() - 16);
  tft.setTextColor(Theme::load, Theme::background);
  tft.drawString(footer, tft.width() / 2, tft.height() - NAV_HEIGHT - 13, pageCount > 1 ? 2 : 4);
  tft.setTextPadding(0);
  tft.setTextDatum(TL_DATUM);
}

void PageManager::drawChargers() {
  TFT_eSPI &tft = _display.screen();
  tft.fillScreen(Theme::background);
  Widgets::drawHeader(tft, "CHARGERS");
  resetDeviceRowCaches();
  const uint8_t total = CampData::chargerCount();
  if (total == 0) {
    Widgets::drawCard(tft, 8, 54, tft.width() - 16, 110, "NO CHARGERS CONFIGURED");
    tft.setTextColor(Theme::textMuted, Theme::card);
    tft.drawString("Provision a compatible Victron charger", 20, 88, 2);
    tft.drawString("and it will appear here automatically.", 20, 108, 1);
  } else {
    int8_t indices[DEVICES_PER_PAGE];
    const uint8_t count = collectVisibleChargers(indices);
    for (uint8_t row = 0; row < count; ++row) {
      const ChargerData &d = CampData::current.chargers[indices[row]];
      const int16_t y = 44 + row * 84;
      Widgets::drawCard(tft, 8, y, tft.width() - 16, 80, d.name);
      drawChargerRowValue(row, d);
      _chargerRows[row] = {indices[row], d.connected, false, d.chargePower,
                           d.chargeCurrent, d.inputVoltage, d.chargeState};
    }
    drawChargersFooter();
  }
  Widgets::drawNavigation(tft, _currentPage);
  _lastPageTotalPower = CampData::current.totalChargerPower;
}

void PageManager::updateChargers() {
  if (CampData::chargerCount() == 0) {
    if (_chargerRows[0].deviceIndex >= 0) _needsFullRedraw = true;
    return;
  }
  int8_t indices[DEVICES_PER_PAGE];
  const uint8_t count = collectVisibleChargers(indices);
  for (uint8_t row = 0; row < DEVICES_PER_PAGE; ++row) {
    if (row >= count || indices[row] < 0) continue;
    const ChargerData &d = CampData::current.chargers[indices[row]];
    DeviceRowCache &c = _chargerRows[row];
    if (c.deviceIndex != indices[row]) {
      _needsFullRedraw = true;
      return;
    }
    if (c.connected != d.connected || isnan(c.power) || fabsf(c.power - d.chargePower) >= 1.0f ||
        isnan(c.current) || fabsf(c.current - d.chargeCurrent) >= 0.1f ||
        isnan(c.voltage) || fabsf(c.voltage - d.inputVoltage) >= 0.1f || c.state != d.chargeState) {
      drawChargerRowValue(row, d);
      c.connected = d.connected;
      c.power = d.chargePower;
      c.current = d.chargeCurrent;
      c.voltage = d.inputVoltage;
      c.state = d.chargeState;
    }
  }
  if (isnan(_lastPageTotalPower) || fabsf(_lastPageTotalPower - CampData::current.totalChargerPower) >= 1.0f) {
    drawChargersFooter();
    _lastPageTotalPower = CampData::current.totalChargerPower;
  }
}

void PageManager::drawLoads() {
  TFT_eSPI &tft = _display.screen();
  tft.fillScreen(Theme::background);
  Widgets::drawHeader(tft, "LOADS");
  resetDeviceRowCaches();
  const uint8_t total = CampData::loadMonitorCount();
  if (total == 0) {
    Widgets::drawCard(tft, 8, 54, tft.width() - 16, 110, "NO LOAD MONITORS CONFIGURED");
    tft.setTextColor(Theme::textMuted, Theme::card);
    tft.drawString("Devices with load-output data", 20, 88, 2);
    tft.drawString("will appear here automatically.", 20, 108, 1);
  } else {
    int8_t indices[DEVICES_PER_PAGE];
    const uint8_t count = collectVisibleLoads(indices);
    for (uint8_t row = 0; row < count; ++row) {
      const int16_t y = 44 + row * 84;
      if (indices[row] == -2) {
        const float shuntCurrent = max(0.0f, -CampData::current.battery.current);
        const float shuntPower = max(0.0f, -CampData::current.battery.power);
        Widgets::drawCard(tft, 8, y, tft.width() - 16, 80, "CAMP SHUNT");
        tft.setTextDatum(MR_DATUM);
        tft.setTextColor(Theme::load, Theme::card);
        char line[48];
        snprintf(line, sizeof(line), "OUT   %.1fA   %.0fW", shuntCurrent, shuntPower);
        tft.drawString(line, tft.width() - 18, y + 48, 2);
        tft.setTextDatum(TL_DATUM);
        _loadRows[row] = {-2, true, shuntCurrent > 0.0f, shuntPower, shuntCurrent, 0.0f, 0};
      } else {
        const ChargerData &d = CampData::current.chargers[indices[row]];
        Widgets::drawCard(tft, 8, y, tft.width() - 16, 80, d.name);
        drawLoadRowValue(row, d);
        _loadRows[row] = {indices[row], d.connected, d.loadOutputOn, d.loadPower,
                          d.loadCurrent, 0.0f, 0};
      }
    }
    drawLoadsFooter();
  }
  Widgets::drawNavigation(tft, _currentPage);
  _lastPageTotalPower = CampData::current.estimatedLoadPower;
  _lastPageTotalCurrent = CampData::current.estimatedLoadCurrent;
}

void PageManager::updateLoads() {
  TFT_eSPI &tft = _display.screen();
  if (CampData::loadMonitorCount() == 0) {
    if (_loadRows[0].deviceIndex >= 0) _needsFullRedraw = true;
    return;
  }
  int8_t indices[DEVICES_PER_PAGE];
  const uint8_t count = collectVisibleLoads(indices);
  for (uint8_t row = 0; row < DEVICES_PER_PAGE; ++row) {
    if (row >= count || indices[row] == -1) continue;
    DeviceRowCache &c = _loadRows[row];
    if (c.deviceIndex != indices[row]) {
      _needsFullRedraw = true;
      return;
    }
    if (indices[row] == -2) {
      const float shuntCurrent = max(0.0f, -CampData::current.battery.current);
      const float shuntPower = max(0.0f, -CampData::current.battery.power);
      if (isnan(c.power) || fabsf(c.power - shuntPower) >= 1.0f ||
          isnan(c.current) || fabsf(c.current - shuntCurrent) >= 0.1f) {
        const int16_t y = 44 + row * 84;
        tft.setTextDatum(MR_DATUM);
        tft.setTextPadding(tft.width() - 36);
        tft.setTextColor(Theme::load, Theme::card);
        char line[48];
        snprintf(line, sizeof(line), "OUT   %.1fA   %.0fW", shuntCurrent, shuntPower);
        tft.drawString(line, tft.width() - 18, y + 48, 2);
        tft.setTextPadding(0);
        tft.setTextDatum(TL_DATUM);
        c.power = shuntPower;
        c.current = shuntCurrent;
      }
      continue;
    }
    const ChargerData &d = CampData::current.chargers[indices[row]];
    if (c.connected != d.connected || c.outputOn != d.loadOutputOn ||
        isnan(c.power) || fabsf(c.power - d.loadPower) >= 1.0f ||
        isnan(c.current) || fabsf(c.current - d.loadCurrent) >= 0.1f) {
      drawLoadRowValue(row, d);
      c.connected = d.connected;
      c.outputOn = d.loadOutputOn;
      c.power = d.loadPower;
      c.current = d.loadCurrent;
    }
  }
  if (isnan(_lastPageTotalPower) || fabsf(_lastPageTotalPower - CampData::current.estimatedLoadPower) >= 1.0f ||
      isnan(_lastPageTotalCurrent) || fabsf(_lastPageTotalCurrent - CampData::current.estimatedLoadCurrent) >= 0.1f) {
    drawLoadsFooter();
    _lastPageTotalPower = CampData::current.estimatedLoadPower;
    _lastPageTotalCurrent = CampData::current.estimatedLoadCurrent;
  }
}

void PageManager::drawBattery() {
  TFT_eSPI &tft = _display.screen();
  tft.fillScreen(Theme::background);
  Widgets::drawHeader(tft, "BATTERY");
  Widgets::drawBatteryCard(tft, 8, 44, tft.width() - 16, 120, CampData::current.battery);
  char usedText[24];
  snprintf(usedText, sizeof(usedText), "%.2fAh", CampData::current.battery.consumedAh);
  char timeText[24];
  const int totalMinutes = CampData::current.battery.remainingMinutes;
  if (totalMinutes > 0) {
    snprintf(timeText, sizeof(timeText), "%.2fh", totalMinutes / 60.0f);
  } else snprintf(timeText, sizeof(timeText), "--");
  const int16_t halfW = (tft.width() - 24) / 2;
  Widgets::drawValueCard(tft, 8, 172, halfW, 70, "USED", usedText, Theme::text);
  Widgets::drawValueCard(tft, 16 + halfW, 172, halfW, 70, "TIME LEFT", timeText, Theme::text);
  Widgets::drawNavigation(tft, _currentPage);
  _lastSoc = CampData::current.battery.soc;
  _lastVoltage = CampData::current.battery.voltage;
  _lastCurrent = CampData::current.battery.current;
  _lastBatteryPower = CampData::current.battery.power;
  _lastConsumedAh = CampData::current.battery.consumedAh;
  _lastRemainingMinutes = CampData::current.battery.remainingMinutes;
}

void PageManager::updateBattery() {
  TFT_eSPI &tft = _display.screen();
  CampData &camp = CampData::current;

  const bool batteryBlinkOn = camp.battery.soc >= 20.0f || ((millis() / 500UL) & 1U) == 0U;
  const bool mainChanged =
    batteryBlinkOn != _lastBatteryBlinkOn ||
    isnan(_lastSoc) || fabsf(camp.battery.soc - _lastSoc) >= 0.5f ||
    isnan(_lastVoltage) || fabsf(camp.battery.voltage - _lastVoltage) >= 0.01f ||
    isnan(_lastCurrent) || fabsf(camp.battery.current - _lastCurrent) >= 0.1f ||
    isnan(_lastBatteryPower) || fabsf(camp.battery.power - _lastBatteryPower) >= 1.0f;
  if (mainChanged) {
    Widgets::drawBatteryCard(tft, 8, 44, tft.width() - 16, 120, camp.battery);
    _lastSoc = camp.battery.soc;
    _lastVoltage = camp.battery.voltage;
    _lastCurrent = camp.battery.current;
    _lastBatteryPower = camp.battery.power;
    _lastBatteryBlinkOn = batteryBlinkOn;
  }

  const int16_t halfW = (tft.width() - 24) / 2;
  if (isnan(_lastConsumedAh) || fabsf(camp.battery.consumedAh - _lastConsumedAh) >= 0.1f) {
    char usedText[24];
    snprintf(usedText, sizeof(usedText), "%.2fAh", camp.battery.consumedAh);
    Widgets::drawValueCard(tft, 8, 172, halfW, 70, "USED", usedText, Theme::text);
    _lastConsumedAh = camp.battery.consumedAh;
  }

  if (camp.battery.remainingMinutes != _lastRemainingMinutes) {
    char timeText[24];
    if (camp.battery.remainingMinutes > 0) {
      snprintf(timeText, sizeof(timeText), "%.2fh", camp.battery.remainingMinutes / 60.0f);
    } else snprintf(timeText, sizeof(timeText), "--");
    Widgets::drawValueCard(tft, 16 + halfW, 172, halfW, 70, "TIME LEFT", timeText, Theme::text);
    _lastRemainingMinutes = camp.battery.remainingMinutes;
  }
}


void PageManager::drawEnergy() {
  TFT_eSPI &tft = _display.screen();
  const DailyEnergyData &e = energyManager.today();
  tft.fillScreen(Theme::background);
  Widgets::drawHeader(tft, "ENERGY TODAY");

  char value[24];
  const int16_t halfW = (tft.width() - 24) / 2;
  snprintf(value, sizeof(value), "%.2fAh", e.solarGeneratedAh);
  Widgets::drawValueCard(tft, 8, 44, halfW, 70, "SOLAR GENERATED", value, Theme::solar);
  snprintf(value, sizeof(value), "%.2fAh", e.batteryUsedAh);
  Widgets::drawValueCard(tft, 16 + halfW, 44, halfW, 70, "BATTERY USED TODAY", value, Theme::load);

  snprintf(value, sizeof(value), "%.2fAh", e.batteryChargedAh);
  Widgets::drawValueCard(tft, 8, 120, halfW, 70, "BATTERY CHARGED", value, Theme::charging);
  snprintf(value, sizeof(value), "%+.2fAh", energyManager.batteryNetAh());
  Widgets::drawValueCard(tft, 16 + halfW, 120, halfW, 70, "BATTERY NET", value,
                         energyManager.batteryNetAh() >= 0.0f ? Theme::charging : Theme::discharging);

  Widgets::drawCard(tft, 8, 198, tft.width() - 16, 46, "ENERGY DAY");
  char footer[80];
  if (isnan(e.startSoc)) {
    snprintf(footer, sizeof(footer), "Starts %02u:00   Start SOC --   Current %.0f%%",
             energyManager.rolloverHour(), CampData::current.battery.soc);
  } else {
    snprintf(footer, sizeof(footer), "Starts %02u:00   Start SOC %.0f%%   Current %.0f%%",
             energyManager.rolloverHour(), e.startSoc, CampData::current.battery.soc);
  }
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(Theme::textMuted, Theme::card);
  tft.drawString(footer, tft.width() / 2, 224, 1);
  tft.setTextDatum(TL_DATUM);

  Widgets::drawNavigation(tft, _currentPage);
  _lastSolarAh = e.solarGeneratedAh;
  _lastChargedAh = e.batteryChargedAh;
  _lastUsedAh = e.batteryUsedAh;
}

void PageManager::updateEnergy() {
  const DailyEnergyData &e = energyManager.today();
  if (isnan(_lastSolarAh) || fabsf(_lastSolarAh - e.solarGeneratedAh) >= 0.01f ||
      isnan(_lastChargedAh) || fabsf(_lastChargedAh - e.batteryChargedAh) >= 0.01f ||
      isnan(_lastUsedAh) || fabsf(_lastUsedAh - e.batteryUsedAh) >= 0.01f) {
    drawEnergy();
    drawModeOverlay();
  }
}

void PageManager::drawHistory() {
  TFT_eSPI &tft = _display.screen();
  tft.fillScreen(Theme::background);
  Widgets::drawHeader(tft, "30 DAY HISTORY");

  if (!historyManager.available()) {
    Widgets::drawCard(tft, 8, 42, tft.width() - 16, 120, "EEPROM NOT FOUND");
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(Theme::batteryCritical, Theme::card);
    tft.drawString("Check AT24C32 at I2C 0x57", tft.width() / 2, 100, 2);
    tft.setTextDatum(TL_DATUM);
    Widgets::drawNavigation(tft, _currentPage);
    return;
  }

  const uint8_t total = historyManager.count();
  if (total == 0) {
    Widgets::drawCard(tft, 8, 42, tft.width() - 16, 120, "NO COMPLETED DAYS YET");
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(Theme::textMuted, Theme::card);
    tft.drawString("First record is saved at the 06:00 rollover", tft.width() / 2, 100, 2);
    tft.setTextDatum(TL_DATUM);
    Widgets::drawNavigation(tft, _currentPage);
    return;
  }

  const uint8_t pageCount = (total + 4) / 5;
  if (_historyPage >= pageCount) _historyPage = 0;
  const uint8_t first = _historyPage * 5;

  for (uint8_t row = 0; row < 5 && first + row < total; ++row) {
    HistoryRecord r;
    if (!historyManager.getNewest(first + row, r)) continue;
    const uint8_t day = r.dayKey % 100;
    const uint8_t month = (r.dayKey / 100) % 100;
    const uint16_t year = r.dayKey / 10000;
    const int16_t y = 46 + row * 38;
    tft.fillRoundRect(8, y, tft.width() - 16, 30, 5, Theme::card);
    tft.drawRoundRect(8, y, tft.width() - 16, 30, 5, Theme::cardBorder);

    char dateText[16];
    snprintf(dateText, sizeof(dateText), "%02u/%02u/%02u", day, month, year % 100);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(Theme::text, Theme::card);
    tft.drawString(dateText, 16, y + 15, 2);

    char energyText[80];
    snprintf(energyText, sizeof(energyText), "Solar %.1f  Charged %.1f  Used %.1f Ah",
             r.solarGeneratedAh, r.batteryChargedAh, r.batteryUsedAh);
    tft.setTextColor(Theme::textMuted, Theme::card);
    tft.drawString(energyText, 112, y + 10, 1);

    char socText[28];
    if (r.startSoc == 255 || r.endSoc == 255) snprintf(socText, sizeof(socText), "SOC --");
    else snprintf(socText, sizeof(socText), "SOC %u%% > %u%%", r.startSoc, r.endSoc);
    tft.drawString(socText, 112, y + 21, 1);
  }

  char footer[48];
  snprintf(footer, sizeof(footer), "PAGE %u/%u - TAP LIST FOR NEXT", _historyPage + 1, pageCount);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(Theme::textMuted, Theme::background);
  tft.drawString(footer, tft.width() / 2, 245, 1);
  tft.setTextDatum(TL_DATUM);
  Widgets::drawNavigation(tft, _currentPage);
}

uint8_t PageManager::daysInMonth(uint16_t year, uint8_t month) const {
  static const uint8_t days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (month == 2) {
    const bool leap = ((year % 4) == 0 && (year % 100) != 0) || (year % 400) == 0;
    return leap ? 29 : 28;
  }
  return days[constrain(month, 1, 12) - 1];
}

void PageManager::beginClockEdit() {
  if (rtcManager.available()) {
    DateTime now = rtcManager.now();
    _editYear = now.year();
    _editMonth = now.month();
    _editDay = now.day();
    _editHour = now.hour();
    _editMinute = now.minute();
  }
  _editRolloverHour = energyManager.rolloverHour();
  _clockEditing = true;
}

void PageManager::adjustClockField(uint8_t field, int8_t delta) {
  if (!_clockEditing) beginClockEdit();
  switch (field) {
    case 0: {
      int value = _editDay + delta;
      const uint8_t maxDay = daysInMonth(_editYear, _editMonth);
      if (value < 1) value = maxDay;
      if (value > maxDay) value = 1;
      _editDay = value;
      break;
    }
    case 1: {
      int value = _editMonth + delta;
      if (value < 1) value = 12;
      if (value > 12) value = 1;
      _editMonth = value;
      _editDay = min(_editDay, daysInMonth(_editYear, _editMonth));
      break;
    }
    case 2: {
      int value = static_cast<int>(_editYear) + delta;
      if (value < 2024) value = 2099;
      if (value > 2099) value = 2024;
      _editYear = value;
      _editDay = min(_editDay, daysInMonth(_editYear, _editMonth));
      break;
    }
    case 3: _editHour = (_editHour + delta + 24) % 24; break;
    case 4: _editMinute = (_editMinute + delta + 60) % 60; break;
    case 5: _editRolloverHour = (_editRolloverHour + delta + 24) % 24; break;
  }
}

void PageManager::saveClockEdit() {
  if (rtcManager.setDateTime(_editYear, _editMonth, _editDay, _editHour, _editMinute, 0)) {
    energyManager.setRolloverHour(_editRolloverHour);
    energyManager.onClockChanged();
  }
  beginClockEdit();
}

void PageManager::drawClock() {
  TFT_eSPI &tft = _display.screen();
  tft.fillScreen(Theme::background);
  Widgets::drawHeader(tft, "CLOCK SETTINGS");

  if (!rtcManager.available()) {
    Widgets::drawCard(tft, 8, 42, tft.width() - 16, 120, "RTC NOT FOUND");
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(Theme::batteryCritical, Theme::card);
    tft.drawString("Check SDA 32 / SCL 25 wiring", tft.width() / 2, 100, 2);
    tft.setTextDatum(TL_DATUM);
    Widgets::drawNavigation(tft, _currentPage);
    return;
  }

  DateTime actual = rtcManager.now();
  drawClockActualTime();

  const char *labels[] = {"DAY", "MONTH", "YEAR", "HOUR", "MINUTE", "DAY START"};
  char values[6][16];
  snprintf(values[0], sizeof(values[0]), "%02u", _editDay);
  snprintf(values[1], sizeof(values[1]), "%02u", _editMonth);
  snprintf(values[2], sizeof(values[2]), "%04u", _editYear);
  snprintf(values[3], sizeof(values[3]), "%02u", _editHour);
  snprintf(values[4], sizeof(values[4]), "%02u", _editMinute);
  snprintf(values[5], sizeof(values[5]), "%02u:00", _editRolloverHour);

  for (uint8_t row = 0; row < 6; ++row) {
    const int16_t y = 64 + row * 27;
    tft.fillRoundRect(8, y, tft.width() - 16, 24, 5, Theme::card);
    tft.drawRoundRect(8, y, tft.width() - 16, 24, 5, Theme::cardBorder);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(Theme::textMuted, Theme::card);
    tft.drawString(labels[row], 104, y + 12, 1);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(Theme::text, Theme::card);
    tft.drawString(values[row], tft.width() / 2, y + 12, 2);
    tft.setTextColor(Theme::navActive, Theme::card);
    tft.drawString("-", 48, y + 12, 2);
    tft.drawString("+", tft.width() - 48, y + 12, 2);
  }

  tft.fillRoundRect(120, 228, tft.width() - 240, 21, 5, Theme::navActive);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(Theme::background, Theme::navActive);
  tft.drawString("SAVE CLOCK", tft.width() / 2, 238, 1);
  tft.setTextDatum(TL_DATUM);
  Widgets::drawNavigation(tft, _currentPage);
  _lastClockSecond = actual.unixtime();
}

void PageManager::drawClockActualTime() {
  if (!rtcManager.available()) return;
  TFT_eSPI &tft = _display.screen();
  const DateTime actual = rtcManager.now();
  char current[48];
  snprintf(current, sizeof(current), "RTC %02u/%02u/%04u  %02u:%02u:%02u",
           actual.day(), actual.month(), actual.year(), actual.hour(), actual.minute(), actual.second());
  tft.fillRect(0, HEADER_HEIGHT, tft.width(), 25, Theme::background);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(rtcManager.needsSetting() ? Theme::batteryWarning : Theme::text, Theme::background);
  tft.setTextPadding(tft.width());
  tft.drawString(current, tft.width() / 2, HEADER_HEIGHT + 12, 2);
  tft.setTextPadding(0);
  tft.setTextDatum(TL_DATUM);
  _lastClockSecond = actual.unixtime();
}

void PageManager::updateClock() {
  if (!rtcManager.available()) return;
  const uint32_t second = rtcManager.now().unixtime();
  if (second != _lastClockSecond) drawClockActualTime();
}

uint32_t PageManager::currentPageSignature() const {
  // Lightweight change detector for pages that otherwise caused a full-screen
  // repaint every 250 ms. FNV-style mixing is adequate for UI invalidation.
  uint32_t h = 2166136261UL;
  auto mix = [&h](uint32_t v) { h ^= v; h *= 16777619UL; };
  mix(static_cast<uint32_t>(_currentPage));
  mix(static_cast<uint32_t>(CampData::current.system.dataSource));
  mix(static_cast<uint32_t>(CampData::current.system.bleConnected));
  mix(static_cast<uint32_t>(CampData::current.system.wifiConnected));

  if (_currentPage == PAGE_CHARGERS || _currentPage == PAGE_LOADS) {
    mix(_currentPage == PAGE_CHARGERS ? _chargerPage : _loadPage);
    for (uint8_t i = 0; i < MAX_VICTRON_DEVICES; ++i) {
      const ChargerData &d = CampData::current.chargers[i];
      mix(d.provisioned); mix(d.connected); mix(d.hasChargerData); mix(d.hasLoadData);
      mix(static_cast<uint32_t>(lroundf(d.chargePower)));
      mix(static_cast<uint32_t>(lroundf(d.chargeCurrent * 10.0f)));
      mix(static_cast<uint32_t>(lroundf(d.inputVoltage * 10.0f)));
      mix(d.chargeState);
      mix(static_cast<uint32_t>(lroundf(d.loadPower)));
      mix(static_cast<uint32_t>(lroundf(d.loadCurrent * 10.0f)));
      mix(d.loadOutputOn);
    }
  } else if (_currentPage == PAGE_STATUS) {
    mix(victronBleStatus.scannerReady); mix(victronBleStatus.deviceSeen);
    mix(victronBleStatus.liveData); mix(victronBleStatus.keyRejected);
    mix(victronBleStatus.keyLoaded); mix(victronBleStatus.instantReadoutSeen);
    // Packet counters and RSSI change continuously; excluding them prevents
    // the Status page from flashing on every advertisement.
    mix(setupPortal.active());
    mix(setupPortal.deviceCount());
    for (uint8_t i = 0; i < setupPortal.deviceCount(); ++i) {
      const ProvisionedVictronDevice *d = setupPortal.deviceAt(i);
      if (!d) continue;
      mix(d->keyStored); mix(d->keyRejected); mix(d->liveData);
    }
  }
  return h;
}

void PageManager::drawThemePage() {
  TFT_eSPI &tft = _display.screen();
  tft.fillScreen(Theme::background);
  Widgets::drawHeader(tft, "DISPLAY THEME");

  struct Preview {
    ThemeId id;
    int16_t x;
    int16_t y;
    const char *label;
  };

  const int16_t half = tft.width() / 2;
  const int16_t previewW = half - 12;
  const Preview previews[] = {
    {THEME_LIGHT,   8,        50,  "LIGHT"},
    {THEME_DARK,    half + 4, 50,  "DARK"},
    {THEME_VICTRON, 8,        122, "VICTRON"},
    {THEME_GLASS,   half + 4, 122, "GLASS"}
  };

  auto V = [](uint16_t c) -> uint16_t { return static_cast<uint16_t>(~c); };
  auto C = [&](uint8_t r, uint8_t g, uint8_t b) -> uint16_t {
    const uint16_t raw = static_cast<uint16_t>(((r & 0xF8) << 8) |
                                                ((g & 0xFC) << 3) |
                                                (b >> 3));
    return V(raw);
  };

  const uint16_t bg[]      = {C(250,250,250), C(0,0,0),       C(8,22,40),    C(10,11,13)};
  const uint16_t cards[]   = {C(244,246,248), C(32,34,38),    C(17,42,66),   C(10,11,13)};
  const uint16_t borders[] = {C(188,194,200), C(82,86,92),    C(42,91,130),  C(115,122,130)};
  const uint16_t texts[]   = {C(18,22,26),    C(245,245,245), C(245,250,255),C(245,245,248)};
  const uint16_t accents[] = {C(30,105,220),  C(70,170,255),  C(60,175,255), C(80,175,255)};

  for (uint8_t i = 0; i < 4; ++i) {
    const int16_t x = previews[i].x;
    const int16_t y = previews[i].y;
    tft.fillRoundRect(x, y, previewW, 64, 8, bg[i]);
    tft.drawRoundRect(x, y, previewW, 64, 8,
                      Theme::current == previews[i].id ? Theme::batteryGood : borders[i]);
    tft.fillRoundRect(x + 8, y + 27, previewW - 16, 28, 5, cards[i]);
    tft.drawRoundRect(x + 8, y + 27, previewW - 16, 28, 5, borders[i]);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(texts[i], bg[i]);
    tft.drawString(previews[i].label, x + previewW / 2, y + 13, 2);
    tft.setTextColor(accents[i], cards[i]);
    tft.drawString("84%   13.24V", x + previewW / 2, y + 41, 2);
  }

  Widgets::drawNavigation(tft, PAGE_STATUS);
}

void PageManager::drawStatus() {
  TFT_eSPI &tft = _display.screen();
  tft.fillScreen(Theme::background);
  Widgets::drawHeader(tft, "STATUS");

  Widgets::drawCard(tft, 8, 44, tft.width() - 16, 108, "VICTRON DEVICES");
  tft.setTextDatum(TL_DATUM);
  if (!victronBleStatus.scannerReady) {
    tft.setTextColor(Theme::batteryCritical, Theme::card);
    tft.drawString("Scanner failed to start", 18, 63, 2);
  } else if (setupPortal.deviceCount() == 0) {
    tft.setTextColor(Theme::solar, Theme::card);
    tft.drawString("Scanning for Victron devices...", 18, 63, 2);
  } else {
    const uint8_t shown = setupPortal.deviceCount() < 3 ? setupPortal.deviceCount() : 3;
    for (uint8_t i = 0; i < shown; ++i) {
      const ProvisionedVictronDevice *d = setupPortal.deviceAt(i);
      if (!d) continue;
      const int16_t y = 62 + i * 27;
      tft.setTextColor(d->liveData ? Theme::batteryGood : (d->keyRejected ? Theme::batteryCritical : Theme::text), Theme::card);
      tft.drawString(d->name, 18, y, 2);
      char line[64];
      const char *state = d->liveData ? "LIVE" : (d->keyRejected ? "BAD KEY" : (d->keyStored ? "KEY SAVED" : "KEY NEEDED"));
      snprintf(line, sizeof(line), "%s  %d dBm  %s", d->address, d->rssi, state);
      tft.setTextColor(Theme::textMuted, Theme::card);
      tft.drawString(line, 18, y + 14, 1);
    }
  }

  // The OTA details now share one card, freeing the final card for Display Theme.
  Widgets::drawCard(tft, 8, 158, tft.width() - 16, 44,
                    setupPortal.active() ? "STOP MAINTENANCE / OTA" : "START MAINTENANCE / OTA");
  tft.setTextColor(setupPortal.active() ? Theme::batteryGood : Theme::textMuted, Theme::card);
  if (setupPortal.active()) {
    String line = String(setupPortal.ssid()) + "  -  192.168.4.1";
    tft.drawString(line, 18, 181, 1);
    if (setupPortal.otaInProgress()) {
      String progress = String("Installing firmware: ") + setupPortal.otaProgress() + "%";
      tft.setTextColor(Theme::solar, Theme::card);
      tft.drawString(progress, 250, 181, 1);
    }
  } else {
    tft.drawString("Tap to open update/config webpage", 18, 181, 1);
  }

  Widgets::drawCard(tft, 8, 208, tft.width() - 16, 38, "DISPLAY THEME");
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(Theme::navActive, Theme::card);
  String themeLine = String(Theme::name(Theme::current)) + "   >";
  tft.drawString(themeLine, tft.width() - 18, 227, 2);
  tft.setTextDatum(TL_DATUM);

  Widgets::drawNavigation(tft, _currentPage);
}

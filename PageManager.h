#pragma once
#include <Arduino.h>
#include "DisplayManager.h"
#include "DataModel.h"

enum PageId : uint8_t {
  PAGE_DASHBOARD = 0,
  PAGE_CHARGERS,
  PAGE_LOADS,
  PAGE_BATTERY,
  PAGE_ENERGY,
  PAGE_HISTORY,
  PAGE_CLOCK,
  PAGE_STATUS,
  PAGE_COUNT
};

class PageManager {
public:
  explicit PageManager(DisplayManager &display);
  void begin();
  void show(PageId page);
  void update();
  void handleTouch(int16_t x, int16_t y);
  PageId currentPage() const;
  void requestRedraw();

private:
  DisplayManager &_display;
  PageId _currentPage;
  bool _needsFullRedraw;
  uint32_t _lastRefreshMs;
  uint8_t _chargerPage;
  uint8_t _loadPage;
  uint8_t _historyPage;

  float _lastSoc;
  float _lastVoltage;
  float _lastCurrent;
  float _lastBatteryPower;
  float _lastChargerPower;
  float _lastChargerCurrent;
  float _lastLoadPower;
  float _lastLoadCurrent;
  float _lastConsumedAh;
  int _lastRemainingMinutes;
  DataSourceMode _lastDataSource;
  uint32_t _lastOverlaySecond;
  uint32_t _lastPageSignature;
  bool _lastBatteryBlinkOn;

  float _lastSolarAh;
  float _lastChargedAh;
  float _lastUsedAh;
  uint32_t _lastClockSecond;
  uint32_t _lastHeaderSecond;
  uint16_t _editYear;
  uint8_t _editMonth;
  uint8_t _editDay;
  uint8_t _editHour;
  uint8_t _editMinute;
  uint8_t _editRolloverHour;
  bool _clockEditing;
  bool _showThemePage;

  struct DeviceRowCache {
    int8_t deviceIndex;
    bool connected;
    bool outputOn;
    float power;
    float current;
    float voltage;
    uint8_t state;
  };
  DeviceRowCache _chargerRows[DEVICES_PER_PAGE];
  DeviceRowCache _loadRows[DEVICES_PER_PAGE];
  float _lastPageTotalPower;
  float _lastPageTotalCurrent;

  void drawCurrentPage();
  void drawDashboard();
  void drawChargers();
  void drawLoads();
  void drawBattery();
  void drawStatus();
  void drawEnergy();
  void drawHistory();
  void drawClock();
  void drawThemePage();
  void updateDashboard();
  void updateChargers();
  void updateLoads();
  void updateBattery();
  void updateEnergy();
  void updateClock();
  void drawClockActualTime();
  void drawModeOverlay();
  void resetCache();
  uint32_t currentPageSignature() const;
  void resetDeviceRowCaches();
  uint8_t collectVisibleChargers(int8_t indices[DEVICES_PER_PAGE]) const;
  uint8_t collectVisibleLoads(int8_t indices[DEVICES_PER_PAGE]) const;
  void drawChargerRowValue(uint8_t row, const ChargerData &d);
  void drawLoadRowValue(uint8_t row, const ChargerData &d);
  void drawChargersFooter();
  void drawLoadsFooter();
  void beginClockEdit();
  void adjustClockField(uint8_t field, int8_t delta);
  void saveClockEdit();
  uint8_t daysInMonth(uint16_t year, uint8_t month) const;
};

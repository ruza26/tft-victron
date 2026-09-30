#include "RtcManager.h"
#include <Wire.h>
#include "Config.h"

RtcManager rtcManager;

void RtcManager::begin() {
  Wire.begin(RTC_SDA_PIN, RTC_SCL_PIN);
  _available = _rtc.begin();

  if (!_available) {
#if DEV_MODE
    Serial.println("RTC: DS3231 not found");
#endif
    return;
  }

  // Never overwrite the clock during boot. A compile-time adjust here caused
  // every cold start to return to the same timestamp. The Clock page is the
  // only place that is allowed to set the DS3231.
  _needsSetting = _rtc.lostPower();

#if DEV_MODE
  Serial.printf("RTC: ready%s\n", _needsSetting ? " (manual setting required)" : "");
#endif
}

bool RtcManager::available() const { return _available; }
bool RtcManager::needsSetting() const { return _needsSetting; }

DateTime RtcManager::now() {
  return _available ? _rtc.now() : DateTime(2000, 1, 1, 0, 0, 0);
}

bool RtcManager::setDateTime(uint16_t year, uint8_t month, uint8_t day,
                             uint8_t hour, uint8_t minute, uint8_t second) {
  if (!_available) return false;

  _rtc.adjust(DateTime(year, month, day, hour, minute, second));
  delay(10);
  const DateTime check = _rtc.now();

  // Allow one second to pass between the write and read-back.
  const bool verified = check.year() == year && check.month() == month &&
                        check.day() == day && check.hour() == hour &&
                        check.minute() == minute &&
                        (check.second() == second || check.second() == ((second + 1) % 60));
  if (verified) _needsSetting = false;
  return verified;
}

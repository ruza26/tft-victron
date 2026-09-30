#pragma once

#include <Arduino.h>
#include <RTClib.h>

class RtcManager {
public:
  void begin();
  bool available() const;
  bool needsSetting() const;
  DateTime now();
  bool setDateTime(uint16_t year, uint8_t month, uint8_t day,
                   uint8_t hour, uint8_t minute, uint8_t second = 0);

private:
  RTC_DS3231 _rtc;
  bool _available = false;
  bool _needsSetting = false;
};

extern RtcManager rtcManager;

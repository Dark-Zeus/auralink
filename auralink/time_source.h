#pragma once

#include <Arduino.h>
#include <time.h> 
#include <RTClib.h>

void updateTimeSourceUI(bool force = false);

class WifiClass;

class TimeSource {
 public:
  TimeSource();
  ~TimeSource();

  bool begin(TwoWire* bus = &Wire, long gmtOffsetSec = 0, const String& ntpServer = "pool.ntp.org");
  bool isAvailable() const;
  uint32_t getEpoch();
  void syncCurrentTime();
  void setCurrentTime(time_t t);

 private:
  RTC_DS3231 _rtc;
  bool _available = false;
  String _ntpServer = "pool.ntp.org";
  long _gmtOffsetSec = 0;
  bool _ntpSynced = false;

  void _updateFromNTPTime();
};

extern TimeSource timeSource;
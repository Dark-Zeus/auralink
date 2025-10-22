#pragma once
#include <Arduino.h>
#include <IPAddress.h>   // for IPAddress

// Forward-declare only; include <WiFi.h> in the .cpp to avoid header collisions
class WifiConnector {
public:
  struct Params {
    const char* hostname        = "AuraLink";
    bool        persistent      = false;     // don't burn credentials to NVS
    bool        autoReconnect   = true;      // let WiFi core auto-retry
    uint32_t    firstRetryMs    = 1000;      // 1s
    uint32_t    maxRetryMs      = 15000;     // 15s cap
    uint32_t    connectTimeoutMs= 10000;     // for waitForConnect()
  };

  WifiConnector();
  WifiConnector(const Params& p);
  ~WifiConnector() = default;
  // Overloads (no default arg here)
  void begin(const char* ssid, const char* pass);

  void loop();

  bool      isConnected() const;
  IPAddress localIP()     const;
  int32_t   rssi()        const;
  String    mac()         const;

  // Returns true if connected before timeout; keeps UI responsive
  bool waitForConnect(uint32_t timeoutMs);

private:
  const char* _ssid = nullptr;
  const char* _pass = nullptr;
  Params      _p;

  uint32_t _nextAttemptAt = 0;
  uint32_t _retryDelay    = 0;

  int _lastStatus = -999;  // store as int to avoid needing wl_status_t in header

  void _kick();                 // attempt one (re)connect
  void _onStatusChange(int s);  // logs + resets backoff on CONNECTED
};

#include "wifi_connector.h"
#include <WiFi.h>   // ESP32 core header (brings wl_status_t, WL_CONNECTED, etc.)

WifiConnector::WifiConnector()
: WifiConnector(Params{}) {}

WifiConnector::WifiConnector(const Params& p)
: _p(p) {
  // nothing to do here yet
}

void WifiConnector::begin(const char* ssid, const char* pass) {
  _ssid = ssid;
  _pass = pass;

  WiFi.persistent(_p.persistent);
  WiFi.mode(WIFI_STA);
#if defined(ESP_ARDUINO_VERSION_MAJOR)
  if (_p.hostname && *_p.hostname) WiFi.setHostname(_p.hostname);
#endif
  WiFi.setAutoReconnect(_p.autoReconnect);

  _retryDelay    = _p.firstRetryMs;
  _nextAttemptAt = millis();  // try immediately
  _lastStatus    = -999;      // force first status log
}

void WifiConnector::loop() {
  int s = (int)WiFi.status();
  if (s != _lastStatus) {
    _onStatusChange(s);
    _lastStatus = s;
  }

  if (s == (int)WL_CONNECTED) return;

  const uint32_t now = millis();
  if ((int32_t)(now - _nextAttemptAt) >= 0) {
    _kick();
    _retryDelay = min<uint32_t>(_retryDelay ? (_retryDelay << 1) : _p.firstRetryMs, _p.maxRetryMs);
    _nextAttemptAt = now + _retryDelay;
  }
}

void WifiConnector::_kick() {
  if (!_ssid || !_pass) return;
  Serial.printf("[WiFi] Connecting to '%s'...\n", _ssid);

  if (WiFi.status() != WL_DISCONNECTED) {
    WiFi.disconnect(true, false); // drop current, keep STA
    delay(50);
  }
  WiFi.begin(_ssid, _pass);
}

bool WifiConnector::waitForConnect(uint32_t timeoutMs) {
  uint32_t start = millis();
  while ((millis() - start) < timeoutMs) {
    loop();
    if (isConnected()) return true;
    delay(10);
  }
  return false;
}

bool WifiConnector::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

IPAddress WifiConnector::localIP() const {
  return WiFi.localIP();
}

int32_t WifiConnector::rssi() const {
  return WiFi.RSSI();
}

String WifiConnector::mac() const {
  return WiFi.macAddress();
}

void WifiConnector::_onStatusChange(int s) {
  const char* name = "UNKNOWN";
  switch ((wl_status_t)s) {
    case WL_IDLE_STATUS:    name = "IDLE"; break;
    case WL_NO_SSID_AVAIL:  name = "NO_SSID"; break;
    case WL_SCAN_COMPLETED: name = "SCAN_DONE"; break;
    case WL_CONNECTED:      name = "CONNECTED"; break;
    case WL_CONNECT_FAILED: name = "CONNECT_FAILED"; break;
    case WL_CONNECTION_LOST:name = "CONNECTION_LOST"; break;
    case WL_DISCONNECTED:   name = "DISCONNECTED"; break;
    default: break;
  }
  Serial.printf("[WiFi] Status: %s\n", name);

  if (s == (int)WL_CONNECTED) {
    _retryDelay = _p.firstRetryMs;  // reset backoff for future drops
    Serial.printf("[WiFi] IP: %s  RSSI: %d dBm  MAC: %s  Hostname: %s\n",
                  WiFi.localIP().toString().c_str(),
                  WiFi.RSSI(),
                  WiFi.macAddress().c_str(),
                  WiFi.getHostname()
    );
  }
}

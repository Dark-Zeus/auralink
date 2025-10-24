#pragma once
#include <Arduino.h>
#include <vector>
#include <IPAddress.h>

void updateMqttUI(bool force = false, bool isConnected = false, bool isSub = false, bool isPub = false);

class Client;

class MqttClient {
public:
  struct Params {
    const char* clientId;
    const char* username;
    const char* password;

    const char* willTopic;
    const char* willPayload;
    uint8_t     willQos;
    bool        willRetain;

    bool     cleanSession;
    uint16_t keepAliveSec;

    uint32_t firstRetryMs;
    uint32_t maxRetryMs;

    Params()
    : clientId(nullptr),
      username(nullptr), password(nullptr),
      willTopic(nullptr), willPayload(nullptr),
      willQos(0), willRetain(false),
      cleanSession(true), keepAliveSec(15),
      firstRetryMs(1000), maxRetryMs(15000) {}
  };

  using MessageHandler = void(*)(const String& topic, const uint8_t* payload, size_t len);

  MqttClient();
  MqttClient(const Params& p);
  ~MqttClient() = default;

  void begin(Client& net, const char* host, uint16_t port);
  void begin(Client& net, const char* host, uint16_t port, const Params& p);

  void loop();

  bool connected() const;
  bool connectNow();
  void disconnect();

  bool subscribe(const char* topic, uint8_t qos = 0);
  bool unsubscribe(const char* topic);
  bool publish(const char* topic, const char* payload, bool retain = false);
  bool publish(const char* topic, const uint8_t* payload, size_t len, bool retain = false);

  void setOnlineMessage(const char* topic, const char* payload, uint8_t qos = 0, bool retain = false);

  void onMessage(MessageHandler cb);

  const char* clientId() const { return _p.clientId; }
  const char* host()     const { return _host; }
  uint16_t    port()     const { return _port; }

private:
  const char* _host = nullptr;
  uint16_t    _port = 1883;

  Client*     _net = nullptr;
  void*       _psClient = nullptr;

  Params      _p{};
  MessageHandler _handler = nullptr;

  std::vector<String> _subscribeTopicBuf;

  uint32_t _nextAttemptAt = 0;
  uint32_t _retryDelay    = 0;

  static MqttClient* _self;
  static void _psCallback(char* topic, uint8_t* payload, unsigned int length);

  String _onlineTopic;
  String _onlinePayload;
  uint8_t _onlineQos = 0;
  bool _onlineRetain = false;

  void _applyServer();
  bool _connectOnce();
  bool _reSubscribe();
};

#include "mqtt.h"
#include "lv_functions.h"

#include <PubSubClient.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <ui.h>

#include "User_Setup.h"

void updateMqttUI(bool force, bool isConnected, bool isSub, bool isPub) {
    static lv_obj_t* ui_sd_MQTTSubIcon = nullptr;
    static lv_obj_t* ui_sd_MQTTPubIcon = nullptr;
    static lv_obj_t* ui_dq_MQTTSubIcon = nullptr;
    static lv_obj_t* ui_dq_MQTTPubIcon = nullptr;
    static lv_obj_t* ui_es_MQTTSubIcon = nullptr;
    static lv_obj_t* ui_es_MQTTPubIcon = nullptr;

    LV_TRY_FIND(ui_es_MQTTPubIcon, ui_ESNotificationBar, UI_COMP_NOTIFICATIONBAR_PUBSUBCONTAINER_PUBICON);
    LV_TRY_FIND(ui_dq_MQTTPubIcon, ui_DQNotificationBar, UI_COMP_NOTIFICATIONBAR_PUBSUBCONTAINER_PUBICON);
    LV_TRY_FIND(ui_sd_MQTTPubIcon, ui_SDNotificationBar, UI_COMP_NOTIFICATIONBAR_PUBSUBCONTAINER_PUBICON);
    LV_TRY_FIND(ui_es_MQTTSubIcon, ui_ESNotificationBar, UI_COMP_NOTIFICATIONBAR_PUBSUBCONTAINER_SUBICON);
    LV_TRY_FIND(ui_dq_MQTTSubIcon, ui_DQNotificationBar, UI_COMP_NOTIFICATIONBAR_PUBSUBCONTAINER_SUBICON);
    LV_TRY_FIND(ui_sd_MQTTSubIcon, ui_SDNotificationBar, UI_COMP_NOTIFICATIONBAR_PUBSUBCONTAINER_SUBICON);

    if (!lv_obj_ok(ui_sd_MQTTSubIcon) && !lv_obj_ok(ui_dq_MQTTSubIcon) && !lv_obj_ok(ui_es_MQTTSubIcon) &&
        !lv_obj_ok(ui_sd_MQTTPubIcon) && !lv_obj_ok(ui_dq_MQTTPubIcon) && !lv_obj_ok(ui_es_MQTTPubIcon)) {
        return;
    }

    const uint32_t now = millis();
    static uint32_t lastSubUpdate = 0;
    static uint32_t lastPubUpdate = 0;
    static uint32_t lastUpdate = 0;

    if (isConnected && isSub) lastSubUpdate = now;
    if (isConnected && isPub) lastPubUpdate = now;

    enum { PULSE_MS = 500, MIN_IDLE_MS = 500 };
    const bool subPulsing = isConnected && (now - lastSubUpdate < PULSE_MS);
    const bool pubPulsing = isConnected && (now - lastPubUpdate < PULSE_MS);
    const bool pulseActive = subPulsing || pubPulsing;

    if (!force && !pulseActive && (now - lastUpdate < MIN_IDLE_MS)) return;

    const lv_color_t base     = isConnected ? lv_color_hex(0xFFFFFF) : lv_color_hex(0xFF0070);
    const lv_color_t colorSub = subPulsing ? lv_color_hex(0x2095F6) : base;
    const lv_color_t colorPub = pubPulsing ? lv_color_hex(0x00FF1B) : base;

    /* Apply safely; no crashes if an icon vanished mid-update */
    LV_ICON_RECOLOR_SAFE(ui_sd_MQTTSubIcon, colorSub);
    LV_ICON_RECOLOR_SAFE(ui_dq_MQTTSubIcon, colorSub);
    LV_ICON_RECOLOR_SAFE(ui_es_MQTTSubIcon, colorSub);

    LV_ICON_RECOLOR_SAFE(ui_sd_MQTTPubIcon, colorPub);
    LV_ICON_RECOLOR_SAFE(ui_dq_MQTTPubIcon, colorPub);
    LV_ICON_RECOLOR_SAFE(ui_es_MQTTPubIcon, colorPub);

    if (!pulseActive) lastUpdate = now;
}

// Store a PubSubClient instance in _psClient via this small wrapper
struct _MqttHolder {
    PubSubClient client;
    _MqttHolder(Client& net)
        : client(net) {}
};

MqttClient* MqttClient::_self = nullptr;

MqttClient::MqttClient()
    : MqttClient(Params{}) {}

MqttClient::MqttClient(const Params& p)
    : _p(p) {}

void MqttClient::begin(Client& net, const char* host, uint16_t port) {
    begin(net, host, port, _p);
}

void MqttClient::begin(Client& net, const char* host, uint16_t port, const Params& p) {
    _net = &net;
    _host = host;
    _port = port;
    _p = p;

    // Create the PubSubClient holder bound to the provided network client
    delete static_cast<_MqttHolder*>(_psClient);
    _psClient = new _MqttHolder(net);

    _applyServer();

    _retryDelay = _p.firstRetryMs ? _p.firstRetryMs : 1000;
    _nextAttemptAt = millis();

    _self = this;
}

void MqttClient::_applyServer() {
    if (!_psClient) return;
    auto& c = static_cast<_MqttHolder*>(_psClient)->client;
    c.setServer(_host, _port);
    c.setCallback(&_psCallback);
    // c.setBufferSize(1024);
}

bool MqttClient::connected() const {
    if (!_psClient) {
        return false;
    }
    auto* holder = static_cast<const _MqttHolder*>(_psClient);
    bool isConnected = const_cast<PubSubClient&>(holder->client).connected();
    return isConnected;
}

void MqttClient::loop() {
    if (!_psClient) return;

    auto& c = static_cast<_MqttHolder*>(_psClient)->client;

    if (connected()) {
        c.loop();  // process incoming / keepalive
        return;
    }

    // retry
    uint32_t now = millis();
    if ((int32_t)(now - _nextAttemptAt) >= 0) {
        _connectOnce();
        // schedule next attempt
        _retryDelay = min<uint32_t>(_retryDelay ? (_retryDelay << 1) : 1000, _p.maxRetryMs ? _p.maxRetryMs : 15000);
        _nextAttemptAt = now + _retryDelay;
    }
}

bool MqttClient::connectNow() {
    return _connectOnce();
}

bool MqttClient::_reSubscribe() {
    if (!connected()) return false;
    // Resubscribe to previously subscribed topics
    Serial.printf("[MQTT] Resubscribing to %u topics...\n", (unsigned int)_subscribeTopicBuf.size());
    for (const auto& topic : _subscribeTopicBuf) {
        Serial.printf("[MQTT] Resubscribing to topic: %s\n", topic.c_str());

        if (static_cast<_MqttHolder*>(_psClient)->client.subscribe(topic.c_str())) {
            Serial.printf("[MQTT] Resubscribed to topic: %s\n", topic.c_str());
        } else {
            Serial.printf("[MQTT] Failed to resubscribe to topic: %s\n", topic.c_str());
            return false;
        }
    }

    return true;
}

bool MqttClient::_connectOnce() {
    if (!_psClient || !_net || !_host || !_port) return false;

    auto& c = static_cast<_MqttHolder*>(_psClient)->client;

    if (_p.keepAliveSec) c.setKeepAlive(_p.keepAliveSec);

    // Build clientId
    char cid[40];
    if (_p.clientId && *_p.clientId) {
        strlcpy(cid, _p.clientId, sizeof(cid));
    } else {
        snprintf(cid, sizeof(cid), "AuraLink-%lu", (unsigned long)ESP.getEfuseMac());
    }

    const char* user = (_p.username && *_p.username) ? _p.username : nullptr;
    const char* pass = (_p.password && *_p.password) ? _p.password : nullptr;
    const bool hasWill = (_p.willTopic && *_p.willTopic);
    const char* willMsg = _p.willPayload ? _p.willPayload : "";

    bool ok = false;

    if (user && hasWill) {
        // (id, user, pass, willTopic, willQos, willRetain, willMessage, cleanSession)
        ok = c.connect(cid, user, pass,
                       _p.willTopic, _p.willQos, (boolean)_p.willRetain, willMsg,
                       (boolean)_p.cleanSession);
    } else if (user && !hasWill) {
        // (id, user, pass)
        ok = c.connect(cid, user, pass);
    } else if (!user && hasWill) {
        // (id, willTopic, willQos, willRetain, willMessage)
        ok = c.connect(cid, _p.willTopic, _p.willQos, (boolean)_p.willRetain, willMsg);
    } else {
        // (id)
        ok = c.connect(cid);
    }

    if (ok) {
        Serial.printf("[MQTT] Connected to %s:%u as '%s'\n", _host, _port, cid);
        _retryDelay = _p.firstRetryMs ? _p.firstRetryMs : 1000;  // reset backoff
    } else {
        Serial.printf("[MQTT] Connect failed, rc=%d\n", c.state());
    }

    if (ok) {
        ok = _reSubscribe();
    }

    // Publish online message if set
    if (ok && !_onlineTopic.isEmpty()) {
        Serial.printf("[MQTT] Publishing online message to topic: %s\n", _onlineTopic.c_str());
        if (c.publish(_onlineTopic.c_str(), _onlinePayload.c_str(), (boolean)_onlineRetain)) {
            Serial.println("[MQTT] Online message published successfully.");
        } else {
            Serial.println("[MQTT] Failed to publish online message.");
        }
    }

    return ok;
}

void MqttClient::disconnect() {
    if (!_psClient) return;
    static_cast<_MqttHolder*>(_psClient)->client.disconnect();
}

bool MqttClient::subscribe(const char* topic, uint8_t qos) {
    _subscribeTopicBuf.push_back(String(topic));
    if (!_psClient) return false;
    bool result = static_cast<_MqttHolder*>(_psClient)->client.subscribe(topic, qos);
    if (result) {
        Serial.printf("[MQTT] Subscribed to topic: %s\n", topic);
    } else {
        Serial.printf("[MQTT] Failed to subscribe to topic: %s\n", topic);
    }
    return result;
}

bool MqttClient::unsubscribe(const char* topic) {
    if (!_psClient) return false;

    // find and remove topic
    for (auto it = _subscribeTopicBuf.begin(); it != _subscribeTopicBuf.end(); ++it) {
        if (*it == topic) {
            _subscribeTopicBuf.erase(it);
            break;
        }
    }

    // unsubscribe via PubSubClient
    return static_cast<_MqttHolder*>(_psClient)->client.unsubscribe(topic);
}

bool MqttClient::publish(const char* topic, const char* payload, bool retain) {
    if (!_psClient) return false;
    return static_cast<_MqttHolder*>(_psClient)->client.publish(topic, payload, retain);
}

bool MqttClient::publish(const char* topic, const uint8_t* payload, size_t len, bool retain) {
    if (!_psClient) return false;
    return static_cast<_MqttHolder*>(_psClient)->client.publish(topic, payload, len, retain);
}

void MqttClient::onMessage(MessageHandler cb) {
    _handler = cb;
}

// Static thunk -> instance callback
void MqttClient::_psCallback(char* topic, uint8_t* payload, unsigned int length) {
    if (!_self || !_self->_handler) return;
    _self->_handler(String(topic), payload, (size_t)length);
}

void MqttClient::setOnlineMessage(const char* topic, const char* payload, uint8_t qos, bool retain) {
    _onlineTopic = String(topic);
    _onlinePayload = String(payload);
    _onlineQos = qos;
    _onlineRetain = retain;
}
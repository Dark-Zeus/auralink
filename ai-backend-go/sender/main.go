package main

import (
	"context"
	"encoding/json"
	"log"
	"os"
	"os/signal"
	"syscall"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
	"github.com/joho/godotenv"

	"ai-backend-go/pkg/common"


)

func main() {
	_ = godotenv.Load()

	host := common.GetenvDefault("MQTT_HOST", "localhost")
	port := common.MustAtoi(common.GetenvDefault("MQTT_PORT", "1883"))
	device := common.GetenvDefault("DEVICE_ID", "pump-01")
	username := common.GetenvDefault("MQTT_USERNAME", "admin") // 👈 added
	password := common.GetenvDefault("MQTT_PASSWORD", "admin") // 👈 added

	topic := "auralink/" + device + "/telemetry"
	broker := common.MqttBrokerURL(host, port)

	opts := mqtt.NewClientOptions().
		AddBroker(broker).
		SetClientID("auralink-sender-go-" + device).
		SetUsername(username).   // 👈 added
		SetPassword(password).   // 👈 added
		SetKeepAlive(60 * time.Second).
		SetPingTimeout(10 * time.Second).
		SetCleanSession(true).
		SetAutoReconnect(true).
		SetConnectRetry(true).
		SetConnectRetryInterval(3 * time.Second)

	client := mqtt.NewClient(opts)
	if tok := client.Connect(); tok.Wait() && tok.Error() != nil {
		log.Fatalf("[SIM] MQTT connect error: %v", tok.Error())
	}
	defer func() {
		client.Disconnect(250)
		log.Println("[SIM] Disconnected.")
	}()

	log.Printf("[SIM] Connected to %s. Sending fake telemetry every 5s → %s\n", broker, topic)

	// graceful stop
	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer stop()

	ticker := time.NewTicker(5 * time.Second)
	defer ticker.Stop()

	i := 0
	for {
		select {
		case <-ctx.Done():
			return
		case <-ticker.C:
			temp := 27.0 + float64(i%12)*0.4 // will cross 30°C
			hum := 55.0 + float64(i%10)*1.8  // will cross 70%
			payload := common.Telemetry{
				TS:       time.Now().UnixMilli(),
				Site:     "kandy-plant",
				DeviceID: device,
				Temp:     common.Round(temp, 2),
				Humidity: common.Round(hum, 1),
			}
			b, err := json.Marshal(payload)
			if err != nil {
				log.Printf("[SIM] JSON marshal error: %v\n", err)
				continue
			}
			pub := client.Publish(topic, 1, false, b)
			pub.Wait()
			if err := pub.Error(); err != nil {
				log.Printf("[SIM] Publish error: %v\n", err)
			} else {
				log.Println("[SIM] Sent:", string(b))
			}
			i++
		}
	}
}

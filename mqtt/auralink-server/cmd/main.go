package main

import (
	"fmt"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

func main() {
	// --- MQTT Broker details ---
	broker := "tcp://10.0.0.79:1883" // or "ws://10.0.0.79:9001" for WebSockets
	clientID := "go-client-auralink"
	username := "admin"
	password := "admin"

	// --- Connection options ---
	opts := mqtt.NewClientOptions()
	opts.AddBroker(broker)
	opts.SetClientID(clientID)
	opts.SetUsername(username)
	opts.SetPassword(password)
	opts.SetKeepAlive(30 * time.Second)
	opts.SetPingTimeout(5 * time.Second)
	opts.SetAutoReconnect(true)
	opts.SetCleanSession(true)

	// --- Message handler ---
	opts.SetDefaultPublishHandler(func(client mqtt.Client, msg mqtt.Message) {
		fmt.Printf("[RX] %s => %s\n", msg.Topic(), msg.Payload())
	})

	client := mqtt.NewClient(opts)
	if token := client.Connect(); token.Wait() && token.Error() != nil {
		panic(token.Error())
	}
	fmt.Println("✅ Connected to broker!")

	// --- Subscribe to command topic ---
	if token := client.Subscribe("auralink/cmd/#", 1, nil); token.Wait() && token.Error() != nil {
		fmt.Println("Subscribe error:", token.Error())
	} else {
		fmt.Println("Subscribed to auralink/cmd/#")
	}

	// --- Publish periodically ---
	for i := 0; ; i++ {
		payload := fmt.Sprintf(`{"msg":"Hello from Go","count":%d}`, i)
		token := client.Publish("auralink/status", 0, false, payload)
		token.Wait()
		fmt.Println("Published:", payload)
		time.Sleep(3 * time.Second)
	}

	// --- Disconnect ---
	client.Disconnect(250)
	fmt.Println("Disconnected")
}

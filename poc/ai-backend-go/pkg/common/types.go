package common

import (
	"fmt"
	"log"
	"os"
	"strconv"
)

// Telemetry published by device/sender
type Telemetry struct {
	TS       int64   `json:"ts"`
	Site     string  `json:"site"`
	DeviceID string  `json:"deviceId"`
	Temp     float64 `json:"temp"`
	Humidity float64 `json:"humidity"`
}

// Reading consumed by backend (temp/hum optional)
type Reading struct {
	Site     string   `json:"site"`
	DeviceID string   `json:"deviceId"`
	TS       int64    `json:"ts"`
	Temp     *float64 `json:"temp,omitempty"`
	Hum      *float64 `json:"hum,omitempty"`
}

// Display payload backend publishes to device
type DisplayOut struct {
	Quote        string `json:"quote"`
	EmailSummary string `json:"email_summary"`
	Priority     string `json:"priority"`
}

func MustEnv(key string) string {
	v := os.Getenv(key)
	if v == "" {
		log.Fatalf("missing %s in env/.env", key)
	}
	return v
}

func GetenvDefault(key, def string) string {
	if v := os.Getenv(key); v != "" {
		return v
	}
	return def
}

func MustAtoi(s string) int {
	n, err := strconv.Atoi(s)
	if err != nil {
		log.Fatalf("invalid int %q: %v", s, err)
	}
	return n
}

func MqttBrokerURL(host string, port int) string {
	return fmt.Sprintf("tcp://%s:%d", host, port)
}

// Priority rules
func ClassifyPriority(temp, hum *float64) string {
	if (temp != nil && *temp >= 30) || (hum != nil && *hum >= 70) {
		return "urgent"
	}
	if (temp != nil && *temp >= 28) || (hum != nil && *hum >= 60) {
		return "attention"
	}
	return "normal"
}

// Simple rounding without importing math
func Round(x float64, digits int) float64 {
	p := pow10(digits)
	return float64(int64(x*p+0.5)) / p
}
func pow10(n int) float64 {
	p := 1.0
	for i := 0; i < n; i++ {
		p *= 10
	}
	return p
}

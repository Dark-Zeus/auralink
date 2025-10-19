package main

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
	"github.com/joho/godotenv"

	"ai-backend-go/pkg/common"
)

type oaInput struct {
	Role    string `json:"role"`
	Content string `json:"content"`
}
type oaReq struct {
	Model       string    `json:"model"`
	Input       []oaInput `json:"input"`
	Temperature float64   `json:"temperature,omitempty"`
}
type oaResp struct {
	OutputText string `json:"output_text"`
}

func makeQuote(ctx context.Context, apiKey string, r common.Reading, siteName string) (string, error) {
	iso := time.UnixMilli(r.TS).UTC().Format(time.RFC3339)

	tStr, hStr := "NA", "NA"
	if r.Temp != nil {
		tStr = fmt.Sprintf("%.2f", *r.Temp)
	}
	if r.Hum != nil {
		hStr = fmt.Sprintf("%.1f", *r.Hum)
	}

	site := r.Site
	if site == "" {
		site = siteName
	}
	contextLine := fmt.Sprintf("Site=%s, Time=%s, Temp=%s°C, Humidity=%s%%.", site, iso, tStr, hStr)

	system := "You are concise and poetic. Write a one- or two-line literature-style quote inspired by the indoor climate. Under 25 words. No emojis."
	user := fmt.Sprintf("Conditions: %s\nQuote:", contextLine)

	body := oaReq{
		Model: "gpt-3o",
		Input: []oaInput{
			{Role: "system", Content: system},
			{Role: "user", Content: user},
		},
		Temperature: 0.7,
	}
	b, _ := json.Marshal(body)

	req, _ := http.NewRequestWithContext(ctx, http.MethodPost, "https://api.openai.com/v1/responses", bytes.NewReader(b))
	req.Header.Set("Authorization", "Bearer "+apiKey)
	req.Header.Set("Content-Type", "application/json")

	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()

	if resp.StatusCode/100 != 2 {
		return "", fmt.Errorf("openai http %d", resp.StatusCode)
	}
	var rb oaResp
	if err := json.NewDecoder(resp.Body).Decode(&rb); err != nil {
		return "", err
	}
	if rb.OutputText == "" {
		return "Soft air drifts; the room keeps its quiet counsel.", nil
	}
	return rb.OutputText, nil
}

func placeholderEmailSummary(r common.Reading, siteName string) string {
	site := r.Site
	if site == "" {
		site = siteName
	}
	t, h := "NA", "NA"
	if r.Temp != nil {
		t = fmt.Sprintf("%.2f", *r.Temp)
	}
	if r.Hum != nil {
		h = fmt.Sprintf("%.1f", *r.Hum)
	}
	return "• Site: " + site + "\n" +
		"• Recent conditions — Temp: " + t + "°C, Humidity: " + h + "%\n" +
		"• No new emails processed (placeholder)\n" +
		"• Next: enable IMAP to summarize unread messages"
}

func main() {

	// at top of main.go
	const openAIKey = ""

	_ = godotenv.Load()

	apiKey := "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"
	host := common.GetenvDefault("MQTT_HOST", "localhost")
	port := common.MustAtoi(common.GetenvDefault("MQTT_PORT", "1883"))
	deviceID := common.GetenvDefault("DEVICE_ID", "pump-01")
	siteName := common.GetenvDefault("SITE_NAME", "kandy-plant")
	username := common.GetenvDefault("MQTT_USERNAME", "admin") // 👈 added
	password := common.GetenvDefault("MQTT_PASSWORD", "admin") // 👈 added

	topicIn := fmt.Sprintf("auralink/%s/telemetry", deviceID)
	topicOut := fmt.Sprintf("auralink/%s/display", deviceID)

	// MQTT options (more robust)
	opts := mqtt.NewClientOptions().
		AddBroker(common.MqttBrokerURL(host, port)).
		SetClientID("auralink-backend-go-" + deviceID).
		SetUsername(username). // 👈 added
		SetPassword(password).
		SetKeepAlive(60 * time.Second).
		SetPingTimeout(10 * time.Second).
		SetCleanSession(true).
		SetAutoReconnect(true).
		SetConnectRetry(true).
		SetConnectRetryInterval(3 * time.Second)

	client := mqtt.NewClient(opts)
	if tok := client.Connect(); tok.Wait() && tok.Error() != nil {
		log.Fatal("MQTT connect error:", tok.Error())
	}
	log.Printf("[MQTT] Connected to %s:%d", host, port)

	// Subscribe
	if tok := client.Subscribe(topicIn, 1, func(_ mqtt.Client, m mqtt.Message) {
		var raw map[string]any
		if err := json.Unmarshal(m.Payload(), &raw); err != nil {
			log.Println("[MQTT] Bad JSON:", err)
			return
		}

		// normalize to Reading
		var r common.Reading
		if s, ok := raw["site"].(string); ok {
			r.Site = s
		}
		if d, ok := raw["deviceId"].(string); ok {
			r.DeviceID = d
		}

		switch v := raw["ts"].(type) {
		case float64:
			r.TS = int64(v)
		case int64:
			r.TS = v
		case string:
			// If ISO string is sent, fallback to "now" for simplicity
			r.TS = time.Now().UnixMilli()
		default:
			r.TS = time.Now().UnixMilli()
		}
		// temp
		if val, ok := raw["temp"]; ok {
			if f, ok := val.(float64); ok {
				r.Temp = &f
			}
		} else if val, ok := raw["temperature"]; ok {
			if f, ok := val.(float64); ok {
				r.Temp = &f
			}
		}
		// hum
		if val, ok := raw["hum"]; ok {
			if f, ok := val.(float64); ok {
				r.Hum = &f
			}
		} else if val, ok := raw["humidity"]; ok {
			if f, ok := val.(float64); ok {
				r.Hum = &f
			}
		}

		log.Printf("[MQTT] IN  %s: %+v", topicIn, r)

		// 1) LLM quote
		ctx, cancel := context.WithTimeout(context.Background(), 20*time.Second)
		defer cancel()
		quote, err := makeQuote(ctx, apiKey, r, siteName)
		if err != nil {
			log.Println("[LLM] Quote error:", err)
			quote = "Quiet air, steady light; the room keeps its calm."
		}

		// 2) Email (placeholder)
		email := placeholderEmailSummary(r, siteName)

		// 3) Priority
		prio := common.ClassifyPriority(r.Temp, r.Hum)

		// 4) Publish DISPLAY
		out := common.DisplayOut{Quote: quote, EmailSummary: email, Priority: prio}
		b, _ := json.Marshal(out)
		pub := client.Publish(topicOut, 1, false, b)
		pub.Wait()
		if err := pub.Error(); err != nil {
			log.Println("[MQTT] Publish error:", err)
		} else {
			log.Printf("[MQTT] OUT %s: %s", topicOut, string(b))
		}
	}); tok.Wait() && tok.Error() != nil {
		log.Fatal("MQTT subscribe error:", tok.Error())
	}
	log.Println("[MQTT] Subscribed to", topicIn)

	// keep running
	select {}
}

package main

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"time"

	"ai-backend-go/pkg/common"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

// ---------- OpenAI structures ----------
type ChatMessage struct {
	Role    string `json:"role"`
	Content string `json:"content"`
}
type ChatRequest struct {
	Model       string        `json:"model"`
	Messages    []ChatMessage `json:"messages"`
	Temperature float64       `json:"temperature,omitempty"`
}
type ChatResponse struct {
	Choices []struct {
		Message struct {
			Content string `json:"content"`
		} `json:"message"`
	} `json:"choices"`
}

var lastQuoteTime time.Time
var quoteCooldown = 20 * time.Second // wait at least 20s between LLM calls

// ---------- makeQuote ----------
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

	reqBody := ChatRequest{
		Model: "gpt-4o-mini",
		Messages: []ChatMessage{
			{Role: "system", Content: "You are concise and poetic. Write a one‑ or two‑line literature‑style quote inspired by the indoor climate. Under 25 words. No emojis."},
			{Role: "user", Content: "Conditions: " + contextLine + "\nQuote:"},
		},
		Temperature: 0.7,
	}
	b, _ := json.Marshal(reqBody)

	req, _ := http.NewRequestWithContext(ctx, http.MethodPost, "https://api.openai.com/v1/chat/completions", bytes.NewReader(b))
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

	var data ChatResponse
	if err := json.NewDecoder(resp.Body).Decode(&data); err != nil {
		return "", err
	}
	if len(data.Choices) == 0 || data.Choices[0].Message.Content == "" {
		return "Soft air drifts; the room keeps its quiet counsel.", nil
	}
	return data.Choices[0].Message.Content, nil
}

// ---------- placeholder email ----------
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
	return fmt.Sprintf("\u2022 Site: %s\n\u2022 Recent conditions — Temp: %s°C, Humidity: %s%%\n\u2022 No new emails processed (placeholder)\n\u2022 Next: enable IMAP to summarize unread messages", site, t, h)
}

// ---------- main ----------
func main() {
	// -------- CONFIG (no .env) --------
	apiKey := ""
	host := "localhost"
	port := 1883
	deviceID := "pump-01"
	siteName := "kandy-plant"
	username := "admin"
	password := "admin"

	topicIn := fmt.Sprintf("auralink/%s/telemetry", deviceID)
	topicOut := fmt.Sprintf("auralink/%s/display", deviceID)

	opts := mqtt.NewClientOptions().
		AddBroker(common.MqttBrokerURL(host, port)).
		SetClientID("auralink-backend-go-" + deviceID).
		SetUsername(username).
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

	if tok := client.Subscribe(topicIn, 1, func(_ mqtt.Client, m mqtt.Message) {
		var raw map[string]any
		if err := json.Unmarshal(m.Payload(), &raw); err != nil {
			log.Println("[MQTT] Bad JSON:", err)
			return
		}

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
		default:
			r.TS = time.Now().UnixMilli()
		}
		if val, ok := raw["temp"].(float64); ok {
			r.Temp = &val
		}
		if val, ok := raw["hum"].(float64); ok {
			r.Hum = &val
		} else if val, ok := raw["humidity"].(float64); ok {
			r.Hum = &val
		}

		tVal, hVal := 0.0, 0.0
		if r.Temp != nil {
			tVal = *r.Temp
		}
		if r.Hum != nil {
			hVal = *r.Hum
		}
		log.Printf("[MQTT] IN  %s: Site=%s Temp=%.2f°C Humidity=%.1f%%", topicIn, r.Site, tVal, hVal)

		if time.Since(lastQuoteTime) < quoteCooldown {
			log.Println("[LLM] Skipped quote (cooldown)")
			return
		}
		lastQuoteTime = time.Now()

		ctx, cancel := context.WithTimeout(context.Background(), 20*time.Second)
		defer cancel()
		quote, err := makeQuote(ctx, apiKey, r, siteName)
		if err != nil {
			log.Println("[LLM] Quote error:", err)
			quote = "Quiet air, steady light; the room keeps its calm."
		}

		email := placeholderEmailSummary(r, siteName)
		prio := common.ClassifyPriority(r.Temp, r.Hum)

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

	select {} // keep alive
}

package main

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"strconv"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
	"github.com/joho/godotenv"
)

// ---------- minimal types ----------
type DisplayOut struct {
	Quote        string `json:"quote"`
	EmailSummary string `json:"email_summary"`
	Priority     string `json:"priority"`
}

// ---------- tiny helpers ----------
func getenvDefault(k, d string) string { if v := os.Getenv(k); v != "" { return v }; return d }
func mustEnv(k string) string          { v := os.Getenv(k); if v == "" { log.Fatalf("missing %s in .env", k) }; return v }

// priority rules
func classifyPriority(temp, hum float64) string {
	if temp >= 30 || hum >= 70 { return "urgent" }
	if temp >= 28 || hum >= 60 { return "attention" }
	return "normal"
}

// OpenAI Responses API (raw HTTP)
type oaInput struct{ Role, Content string `json:"role","content"` }
type oaReq struct {
	Model       string    `json:"model"`
	Input       []oaInput `json:"input"`
	Temperature float64   `json:"temperature,omitempty"`
}
type oaResp struct{ OutputText string `json:"output_text"` }

func makeQuote(ctx context.Context, apiKey, site string, ts int64, temp, hum float64) (string, error) {
	iso := time.UnixMilli(ts).UTC().Format(time.RFC3339)
	contextLine := fmt.Sprintf("Site=%s, Time=%s, Temp=%.2f°C, Humidity=%.1f%%.", site, iso, temp, hum)

	system := "You are concise and poetic. Write a one- or two-line literature-style quote inspired by the indoor climate. Under 25 words. No emojis."
	user := "Conditions: " + contextLine + "\nQuote:"

	reqBody := oaReq{
		Model: "gpt-4o-mini",
		Input: []oaInput{
			{Role: "system", Content: system},
			{Role: "user", Content: user},
		},
		Temperature: 0.7,
	}
	b, _ := json.Marshal(reqBody)

	req, _ := http.NewRequestWithContext(ctx, http.MethodPost, "https://api.openai.com/v1/responses", bytes.NewReader(b))
	req.Header.Set("Authorization", "Bearer "+apiKey)
	req.Header.Set("Content-Type", "application/json")

	resp, err := http.DefaultClient.Do(req)
	if err != nil { return "", err }
	defer resp.Body.Close()
	if resp.StatusCode/100 != 2 { return "", fmt.Errorf("openai http %d", resp.StatusCode) }

	var rb oaResp
	if err := json.NewDecoder(resp.Body).Decode(&rb); err != nil { return "", err }
	if rb.OutputText == "" { return "Soft air drifts; the room keeps its quiet counsel.", nil }
	return rb.OutputText, nil
}

func placeholderEmail(site string, temp, hum float64) string {
	return fmt.Sprintf("• Site: %s\n• Recent conditions — Temp: %.2f°C, Humidity: %.1f%%\n• No new emails processed (placeholder)\n• Next: enable IMAP to summarize unread messages", site, temp, hum)
}

// ---------- main ----------
func main() {
	_ = godotenv.Load()

	// env
	apiKey := mustEnv("OPENAI_API_KEY")
	host := getenvDefault("MQTT_HOST", "10.0.0.79")
	port := getenvDefault("MQTT_PORT", "1883")
	user := getenvDefault("MQTT_USERNAME", "admin")
	pass := getenvDefault("MQTT_PASSWORD", "admin")
	deviceID := getenvDefault("DEVICE_ID", "pump-01")
	siteName := getenvDefault("SITE_NAME", "kandy-plant")

	p, _ := strconv.Atoi(port)
	broker := fmt.Sprintf("tcp://%s:%d", host, p)
	topicOut := fmt.Sprintf("auralink/%s/display", deviceID)

	// MQTT client
	opts := mqtt.NewClientOptions().
		AddBroker(broker).
		SetClientID("auralink-ai-go-"+deviceID).
		SetUsername(user).
		SetPassword(pass).
		SetKeepAlive(60 * time.Second).
		SetPingTimeout(10 * time.Second).
		SetCleanSession(true).
		SetAutoReconnect(true)

	client := mqtt.NewClient(opts)
	if tok := client.Connect(); tok.Wait() && tok.Error() != nil {
		log.Fatal("MQTT connect error:", tok.Error())
	}
	defer client.Disconnect(250)
	log.Println("✅ Connected to broker:", broker)
	log.Println("Publishing to:", topicOut)

	// --- HARD-CODED "sensor" values here ---
	// Change these numbers anytime to test LED/priority.
	temp := 27.8   // set 30.2 to see "urgent"
	hum  := 57.6   // set 71.0 to see "urgent"

	// loop every 10s: build context → ChatGPT quote → priority → publish
	ticker := time.NewTicker(10 * time.Second)
	defer ticker.Stop()

	for {
		now := time.Now().UnixMilli()

		// 1) ask ChatGPT for the quote
		ctx, cancel := context.WithTimeout(context.Background(), 20*time.Second)
		quote, err := makeQuote(ctx, apiKey, siteName, now, temp, hum)
		cancel()
		if err != nil {
			log.Println("[LLM] Quote error:", err)
			quote = "Quiet air, steady light; the room keeps its calm."
		}

		// 2) placeholder email summary
		email := placeholderEmail(siteName, temp, hum)

		// 3) priority
		prio := classifyPriority(temp, hum)

		// 4) publish to display topic
		out := DisplayOut{Quote: quote, EmailSummary: email, Priority: prio}
		b, _ := json.Marshal(out)
		pub := client.Publish(topicOut, 1, false, b)
		pub.Wait()
		if err := pub.Error(); err != nil {
			log.Println("[MQTT] Publish error:", err)
		} else {
			log.Printf("📤 OUT %s: %s\n", topicOut, string(b))
		}

		<-ticker.C

		// (optional) change numbers a bit each cycle so you can see priority flip
		// temp += 0.6 ; hum += 2.5
	}
}

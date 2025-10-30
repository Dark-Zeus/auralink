#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// Use UART2 on ESP32-S3 with custom pins
// RX = 41, TX = 42
HardwareSerial DFSerial(2);
DFRobotDFPlayerMini df;

void printDfError() {
  switch (df.readType()) {
    case TimeOut:       Serial.println("[DFPlayer] Timeout"); break;
    case WrongStack:    Serial.println("[DFPlayer] Wrong Stack"); break;
    case DFPlayerCardInserted:  Serial.println("[DFPlayer] Card inserted"); break;
    case DFPlayerCardRemoved:   Serial.println("[DFPlayer] Card removed"); break;
    case DFPlayerCardOnline:    Serial.println("[DFPlayer] Card online"); break;
    case DFPlayerPlayFinished:  Serial.printf("[DFPlayer] Play finished: %d\n", df.read()); break;
    case DFPlayerError: {
      uint8_t e = df.read();
      Serial.printf("[DFPlayer] Error %d\n", e);
      break;
    }
    default: break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // Start UART2 on custom pins (RX=41, TX=42)
  DFSerial.begin(9600, SERIAL_8N1, 41, 42);
  delay(200);

  Serial.println("[DFPlayer] Initializing...");
  if (!df.begin(DFSerial, /*isACK=*/true, /*doReset=*/true)) {
    Serial.println("[DFPlayer] Init failed. Check wiring, 5V power, and SD (FAT32).");
    while (true) { delay(1000); }
  }

  // Basic setup
  df.setTimeOut(500);
  df.volume(20);           // 0–30
  df.EQ(DFPLAYER_EQ_NORMAL);

  // Play /01/001.mp3
  df.playFolder(1, 1);
  Serial.println("[DFPlayer] Playing /01/001.mp3");
}

void loop() {
  // Handle DFPlayer events (finished, errors, card in/out)
  if (df.available()) {
    printDfError();
  }

  // Example: simple demo actions (comment out if not needed)
  static unsigned long t0 = millis();
  if (millis() - t0 > 10000) {         // every 10s, next track in folder 01
    t0 = millis();
    df.next();
    Serial.println("[DFPlayer] Next track");
  }
}

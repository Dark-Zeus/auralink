#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold12pt7b.h>

// ---- PINS: try set A first; if no flicker, switch to set B ----
// Set A (your current map)
#define EPD_SCK   12
#define EPD_MOSI  11
#define EPD_CS    10
#define EPD_DC    42
#define EPD_RST   41
#define EPD_BUSY  40

/* // Set B (alternate safe pins for control lines on ESP32-S3)
#define EPD_SCK   12
#define EPD_MOSI  11
#define EPD_CS    10
#define EPD_DC    5
#define EPD_RST   6
#define EPD_BUSY  7
*/

// ---- DRIVER SELECT ----
// Most Waveshare 2.7" BW (264x176) work with GxEPD2_270
// If no flicker/output, comment the first line and UNcomment the second to try GDEY027T91.

#define USE_GDEP270   // common
//#define USE_GDEY027T91

#ifdef USE_GDEP270
  #define PANEL_NAME "GxEPD2_270c"
  GxEPD2_BW<GxEPD2_270c, GxEPD2_270c::HEIGHT> display(GxEPD2_270c(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));
#endif
#ifdef USE_GDEY027T91
  #define PANEL_NAME "GxEPD2_270_GDEY027T91"
  GxEPD2_BW<GxEPD2_270_GDEY027T91, GxEPD2_270_GDEY027T91::HEIGHT> display(GxEPD2_270_GDEY027T91(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));
#endif

void hardResetPulse() {
  pinMode(EPD_RST, OUTPUT);
  digitalWrite(EPD_RST, HIGH);
  delay(10);
  digitalWrite(EPD_RST, LOW);   // active low reset
  delay(20);
  digitalWrite(EPD_RST, HIGH);
  delay(200);                   // give power-on settle time
}

void printBusy(const char* tag) {
  pinMode(EPD_BUSY, INPUT);
  int b = digitalRead(EPD_BUSY);
  Serial.print(tag);
  Serial.print(" BUSY=");
  Serial.println(b);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n--- ePaper probe ---");

  // Start SPI (panel is write-only → MISO = -1)
  SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);

  // 1) Hardware reset pulse (you should see a tiny flicker if panel is alive)
  Serial.println("Pulsing RST...");
  hardResetPulse();
  printBusy("After RST");

  // 2) Init display (try a conservative clock inside the driver)
  Serial.print("Init display: "); Serial.println(PANEL_NAME);
  display.init(115200);
  display.setRotation(1);
  display.setFullWindow();

  // 3) Clear to white (full refresh)
  Serial.println("Full clear...");
  display.firstPage();
  do { display.fillScreen(GxEPD_WHITE); } while (display.nextPage());
  printBusy("After clear");

  delay(500);

  // 4) Big text test
  Serial.println("Draw text...");
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(8, 60);
    display.print("HELLO EPAPER");
    display.setCursor(8, 100);
    display.print(PANEL_NAME);
  } while (display.nextPage());
  printBusy("After text");

  Serial.println("Done. If no flicker at all, check pins/driver/power.");
}

void loop() {
  // nothing
}

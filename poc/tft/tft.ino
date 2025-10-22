#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define TFT_CS   12
#define TFT_RST  14
#define TFT_A0   13   // DC pin

#define SPI_SDA  10    // actually MOSI for TFT
#define SPI_SCK  11    // actually SCLK for TFT

// Create SPI object with custom pins
SPIClass spiVspi(1); // use VSPI for ESP32

// Create the display object
Adafruit_ST7735 tft = Adafruit_ST7735(&spiVspi, TFT_CS, TFT_A0, TFT_RST);

// Track cursor for new lines
int cursorY = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("ST7735 TFT test with Serial input");

  // Initialize the custom SPI bus with your chosen pins
  spiVspi.begin(SPI_SCK, -1, SPI_SDA, TFT_CS); 
  // (CLK, MISO not used for display, MOSI, SS)

  // Initialize the display
  tft.initR(INITR_BLACKTAB); 
  tft.setRotation(1);        
  tft.fillScreen(ST77XX_BLACK);

  // Text setup
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(0.5);
  tft.setCursor(0, 0);
  tft.println("Ready...");
  cursorY = 32; // leave first lines for header
}

void loop() {
  // Check if Serial has data
  if (Serial.available()) {
    String incoming = Serial.readStringUntil('\n');
    printLine(incoming);
  }
}

// Helper to print a new line on TFT
void printLine(const String &line) {
  if (cursorY > tft.height() - 16) {
    tft.fillScreen(ST77XX_BLACK);
    cursorY = 0;
  }

  tft.setCursor(0, cursorY);
  tft.println(line);
  cursorY += 16; // adjust spacing based on text size
}
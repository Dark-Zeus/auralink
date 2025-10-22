// ==== Your pins (as requested) ====
#define SPI_MOSI   4   // DIN
#define SPI_SCLK   5   // CLK
#define SPI_MISO  -1   // not used by these EPDs
#define EINK_BUSY  9
#define EINK_CS    10
#define EINK_RST   11
#define EINK_DC    12

// ====== Select panel type (1 = B/W V2, 0 = 3-color GDEW027C44) ======
#define EPD_IS_BW 1

#include <SPI.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>

// ---- Display constructors for 2.7" 264x176 ----
// B/W:   GxEPD2_270  (Good Display GDEH027W3 / Waveshare 2.7 V2)
// 3-col: GxEPD2_270c (GDEW027C44)
#if EPD_IS_BW
  GxEPD2_BW<GxEPD2_270, GxEPD2_270::HEIGHT> display(
    GxEPD2_270(EINK_CS, EINK_DC, EINK_RST, EINK_BUSY)
  );
#else
  GxEPD2_3C<GxEPD2_270c, GxEPD2_270c::HEIGHT> display(
    GxEPD2_270c(EINK_CS, EINK_DC, EINK_RST, EINK_BUSY)
  );
#endif

// ---- Safeguard tunables for clone boards ----
static const uint32_t SPI_SLOW   = 1000000UL; // 1 MHz (very safe)
static const uint32_t SPI_SAFE   = 2000000UL; // 2 MHz (safe)
static const uint32_t SPI_NORMAL = 4000000UL; // 4 MHz (nominal)
static const uint16_t RESET_MS1  = 10;        // longer HW reset pulse
static const uint16_t RESET_MS2  = 2;         // “clever reset” fallback
static const uint32_t BUSY_TIMEOUT_MS = 15000; // hard stop if panel misbehaves

// Small helper: wait for BUSY with timeout so we never hang forever.
bool waitWhileBusy(uint32_t timeout_ms, const char* tag)
{
  uint32_t start = millis();
  while (digitalRead(EINK_BUSY) == HIGH) // HIGH = busy per Waveshare spec
  {
    if (millis() - start > timeout_ms) {
      Serial.print("[TIMEOUT] BUSY stayed HIGH during "); Serial.println(tag);
      return false;
    }
    delay(5);
  }
  return true;
}

void fullPaint(uint16_t color)
{
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(color);
  } while (display.nextPage());
}

void tryInit(uint32_t spi_hz, uint16_t reset_ms, bool verbose)
{
  if (verbose) {
    Serial.print("[Init] SPI="); Serial.print(spi_hz);
    Serial.print(" Hz, reset="); Serial.print(reset_ms);
    Serial.println(" ms");
  }

  // Remap SPI pins where the MCU allows it (ESP32/RP2040/etc.)
#if defined(ESP32)
  SPI.begin(SPI_SCLK, SPI_MISO, SPI_MOSI, EINK_CS);
#elif defined(ARDUINO_ARCH_RP2040)
  // Many cores accept begin(sck, miso, mosi, ss); safe to call:
  SPI.begin(SPI_SCLK, SPI_MISO, SPI_MOSI, EINK_CS);
#else
  SPI.begin(); // classic AVR will ignore custom pins (must use HW pins)
#endif

  // Force the library to use our SPI settings speed/mode.
  display.epd2.selectSPI(SPI, SPISettings(spi_hz, MSBFIRST, SPI_MODE0));

  // Longer reset helps a lot on clone HATs with slow RC reset networks.
  pinMode(EINK_BUSY, INPUT); // many HATs pull this up; HIGH means busy
  display.init(115200, /*initial=*/true, /*reset_duration_ms=*/reset_ms, /*pullDownMosi=*/false);

  // quick sanity: BUSY should go LOW when idle
  int b = digitalRead(EINK_BUSY);
  if (verbose) {
    Serial.print("BUSY (after init) = "); Serial.println(b);
  }
}

bool safeRefreshCheck()
{
  // Before any refresh cycle, ensure BUSY is LOW or we’ll wedge
  if (digitalRead(EINK_BUSY) == HIGH) {
    Serial.println("[Warn] BUSY HIGH before refresh, waiting...");
    if (!waitWhileBusy(BUSY_TIMEOUT_MS, "pre-refresh")) return false;
  }
  return true;
}

void setup()
{
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println("2.7\" E-Paper bring-up with safeguards");

  // --- First attempt: very safe SPI and long reset ---
  tryInit(SPI_SAFE, RESET_MS1, true);

  // If the panel never releases BUSY here, try even slower
  if (digitalRead(EINK_BUSY) == HIGH) {
    Serial.println("[Info] BUSY stayed HIGH, retrying at 1 MHz and 2 ms reset...");
    display.end();
    delay(50);
    tryInit(SPI_SLOW, RESET_MS2, true);
  }

  // If still stuck, we’ll proceed but time out on waits to avoid hangs.

  // --- Make the whole screen BLACK (rotation doesn’t matter) ---
  if (!safeRefreshCheck()) {
    Serial.println("[Abort] Panel not ready to draw.");
    return;
  }
  Serial.println("[Action] Painting full BLACK...");
#if EPD_IS_BW
  fullPaint(GxEPD_BLACK);
#else
  // On tri-color, “black layer” is black; we’re not drawing red here.
  fullPaint(GxEPD_BLACK);
#endif

  // Optional: small pause then go WHITE to prove we can toggle
  delay(1500);
  if (!safeRefreshCheck()) {
    Serial.println("[Warn] Panel not ready for WHITE, skipping.");
  } else {
    Serial.println("[Action] Painting full WHITE...");
    fullPaint(GxEPD_WHITE);
  }

  // Optionally, drop power so ghosting is reduced and clones don’t drain
  display.powerOff();
  Serial.println("Done.");
}

void loop() {}

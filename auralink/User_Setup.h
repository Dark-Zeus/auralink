// ==== Local User_Setup.h (in your sketch folder) ====
#define USER_SETUP_LOADED   // tell TFT_eSPI we provide everything here

// --- Display driver & size ---
#define ST7735_DRIVER
#define TFT_WIDTH  128
#define TFT_HEIGHT 160

// Your module: green tab on ribbon, red PCB
#define ST7735_BLACKTAB    // try GREENTAB2 first
#define TFT_RGB_ORDER TFT_RGB   // fix common color swap on green-tab boards

// --- Pin map (match your working Adafruit sketch) ---
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   6
#define TFT_DC   5
#define TFT_RST  4
// Backlight pin (set HIGH in your sketch)
#define TFT_BL    2

// --- SPI speed & options ---
#define SPI_FREQUENCY  27000000    // start safe; raise to 40 MHz after it works
#define SPI_READ_FREQUENCY 20000000
#define SPI_TOUCH_FREQUENCY 2500000

#define USE_HSPI_PORT

// No touch/SD on this bus:
#undef TOUCH_CS
#undef SD_CS

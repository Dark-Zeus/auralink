#include <Wire.h>
#include <BH1750.h>

// Read from BH1750 light sensor and print to Serial. no user libs
// set 12c bus pins here
constexpr uint8_t I2C_SDA_PIN = 17;
constexpr uint8_t I2C_SCL_PIN = 18;

BH1750 lightMeter;

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);         // 100 kHz
  Wire.setTimeout(20);

  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2, 0x5C, &Wire);
  Serial.println("BH1750 Test");
}

void loop() {
  float lux = lightMeter.readLightLevel();
  Serial.print("Light Level: ");
  Serial.print(lux);
  Serial.println(" lx");
  delay(1000);
}

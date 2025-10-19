// MQ135 Analog Gas Sensor Example (for ESP32)
// Reads analog signal and prints estimated air quality (ppm scale)

const int mq135Pin = 8;  // ADC1_CH0 on ESP32
float RLOAD = 10.0;       // Load resistance on the module (kΩ)
float RZERO = 76.63;      // Calibration constant (to be tuned)

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("MQ135 Air Quality Sensor - ESP32 Example");
}

void loop() {
  // Read analog value (0–4095)
  int raw_adc = analogRead(mq135Pin);

  // Convert to voltage (ESP32 ADC is 12-bit, 0–3.3 V)
  float voltage = (raw_adc / 4095.0) * 3.3;

  // Calculate sensor resistance (Rs)
  // Using: Rs = (Vc / Vout - 1) * Rload
  float Rs = ((3.3 * RLOAD) / voltage) - RLOAD;

  // Estimate ratio Rs/R0
  float ratio = Rs / RZERO;

  // Rough estimation of CO2 concentration (ppm)
  // Formula derived empirically (not perfectly accurate)
  float ppm = 116.6020682 * pow(ratio, -2.769034857);

  Serial.print("ADC: "); Serial.print(raw_adc);
  Serial.print(" | Voltage: "); Serial.print(voltage, 2); Serial.print(" V");
  Serial.print(" | Rs/R0: "); Serial.print(ratio, 2);
  Serial.print(" | Estimated CO2: "); Serial.print(ppm, 1); Serial.println(" ppm");

  delay(1000);
}

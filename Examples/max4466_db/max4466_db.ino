const int MIC_PIN = 9; // analog input
const int SAMPLES = 500; // samples per RMS window

void setup() {
  Serial.begin(115200);
}

void loop() {
  double sumSquares = 0;
  for (int i = 0; i < SAMPLES; i++) {
    int16_t raw = analogRead(MIC_PIN);
    float voltage = (raw / 4095.0) * 3.3;   // convert ADC to volts
    float ac = voltage - 1.65;              // remove DC bias
    sumSquares += ac * ac;
    delayMicroseconds(100);                 // ~10 kHz sample rate
  }

  float Vrms = sqrt(sumSquares / SAMPLES);
  float dB = 20.0 * log10(Vrms / 0.00631);  // 0.00631V = reference ~60dB (tune by calibration)

  Serial.printf("Vrms=%.5f  dB=%.1f\n", Vrms, dB);
  delay(100);
}

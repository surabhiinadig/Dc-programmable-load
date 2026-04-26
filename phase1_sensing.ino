#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;

// Constants
const float SHUNT_RESISTANCE = 1.0;  // 1 ohm current sense resistor
const float VOLTAGE_DIVIDER_RATIO = 1.0; // Adjust if using a voltage divider

void setup() {
  Serial.begin(9600);
  Serial.println("Programmable DC Load - Phase 1: Sensing");

  // Initialize ADS1115
  if (!ads.begin()) {
    Serial.println("ERROR: ADS1115 not found! Check wiring.");
    while (1);
  }

  ads.setGain(GAIN_ONE); // +/- 4.096V range
  Serial.println("ADS1115 initialized successfully.");
  Serial.println("------------------------------------");
  Serial.println("Voltage(V) | Current(A) | Power(W)");
  Serial.println("------------------------------------");
}

void loop() {
  // Read differential voltage across shunt resistor (A0 - A1)
  int16_t shuntRaw = ads.readADC_Differential_0_1();
  float shuntVoltage = ads.computeVolts(shuntRaw);

  // Read supply voltage on A2 (single ended)
  int16_t voltRaw = ads.readADC_SingleEnded(2);
  float supplyVoltage = ads.computeVolts(voltRaw) * VOLTAGE_DIVIDER_RATIO;

  // Calculate current: I = V / R
  float current = shuntVoltage / SHUNT_RESISTANCE;

  // Calculate power: P = V * I
  float power = supplyVoltage * current;

  // Print results
  Serial.print(supplyVoltage, 3);
  Serial.print("V     | ");
  Serial.print(current, 4);
  Serial.print("A    | ");
  Serial.print(power, 4);
  Serial.println("W");

  delay(500);
}

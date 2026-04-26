#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_MCP4725.h>

Adafruit_ADS1115 ads;
Adafruit_MCP4725 dac;

// Constants
const float SHUNT_RESISTANCE = 1.0;   // 1 ohm sense resistor
const float MAX_CURRENT = 2.0;         // Safety limit: 2A max
const float VCC = 5.0;                 // Arduino supply voltage
const int   DAC_MAX = 4095;

// Safety: Max DAC value allowed (limits gate drive = limits current)
// Tune this based on your MOSFET characterisation
const int DAC_SAFE_LIMIT = 2000;

void setup() {
  Serial.begin(9600);
  Serial.println("Programmable DC Load - Phase 2b: MOSFET Gate Control");

  // Initialize ADS1115
  if (!ads.begin()) {
    Serial.println("ERROR: ADS1115 not found!");
    while (1);
  }
  ads.setGain(GAIN_ONE);
  Serial.println("ADS1115 ready.");

  // Initialize MCP4725 DAC
  if (!dac.begin(0x60)) {
    Serial.println("ERROR: MCP4725 not found!");
    while (1);
  }
  Serial.println("MCP4725 ready.");

  // Start with gate off (DAC = 0, MOSFET off)
  dac.setVoltage(0, false);
  Serial.println("Gate set to 0. MOSFET OFF.");
  Serial.println("-------------------------------------------------------");
  Serial.println("DAC Val | Gate V | Current(A) | Voltage(V) | Power(W)");
  Serial.println("-------------------------------------------------------");

  delay(1000);
}

void loop() {
  // Gradually increase DAC value to increase gate voltage and sink more current
  for (int dacValue = 0; dacValue <= DAC_SAFE_LIMIT; dacValue += 100) {

    // Set DAC output -> goes to LM358 IN+ -> LM358 OUT -> 100ohm -> MOSFET Gate
    dac.setVoltage(dacValue, false);

    delay(100); // Allow circuit to settle

    // Read shunt voltage (differential A0-A1)
    int16_t shuntRaw = ads.readADC_Differential_0_1();
    float shuntVoltage = ads.computeVolts(shuntRaw);
    float current = shuntVoltage / SHUNT_RESISTANCE;

    // Read supply voltage (single ended A2)
    int16_t voltRaw = ads.readADC_SingleEnded(2);
    float supplyVoltage = ads.computeVolts(voltRaw);

    // Calculate power
    float power = supplyVoltage * current;

    // Approximate gate voltage from DAC value
    float gateVoltage = (dacValue / 4095.0) * VCC;

    // Print readings
    Serial.print(dacValue);
    Serial.print("    | ");
    Serial.print(gateVoltage, 2);
    Serial.print("V   | ");
    Serial.print(current, 4);
    Serial.print("A     | ");
    Serial.print(supplyVoltage, 3);
    Serial.print("V      | ");
    Serial.print(power, 4);
    Serial.println("W");

    // Safety check - cut off if current exceeds limit
    if (current > MAX_CURRENT) {
      Serial.println("!!! OVERCURRENT DETECTED - Shutting down gate !!!");
      dac.setVoltage(0, false);
      while (1); // Halt
    }
  }

  // Ramp back down safely
  dac.setVoltage(0, false);
  Serial.println("-------------------------------------------------------");
  Serial.println("Sweep complete. MOSFET OFF. Restarting in 3 seconds...");
  delay(3000);
}

#include <Wire.h>
#include <Adafruit_MCP4725.h>

Adafruit_MCP4725 dac;

// MCP4725 I2C address is 0x60 by default
#define DAC_ADDRESS 0x60

void setup() {
  Serial.begin(9600);
  Serial.println("Programmable DC Load - Phase 2a: DAC Bring-Up");

  // Initialize MCP4725 DAC
  if (!dac.begin(DAC_ADDRESS)) {
    Serial.println("ERROR: MCP4725 not found! Check wiring.");
    while (1);
  }

  Serial.println("MCP4725 DAC initialized successfully.");
  Serial.println("Starting sweep test...");
  Serial.println("DAC Value | Output Voltage (approx)");
  Serial.println("-----------------------------------------");
}

void loop() {
  // Sweep DAC from 0 to 4095 in steps of 200
  for (int dacValue = 0; dacValue <= 4095; dacValue += 200) {
    dac.setVoltage(dacValue, false);

    // Approximate voltage = (dacValue / 4095) * VCC
    float approxVoltage = (dacValue / 4095.0) * 5.0;

    Serial.print("DAC: ");
    Serial.print(dacValue);
    Serial.print("     | ~");
    Serial.print(approxVoltage, 3);
    Serial.println("V");

    delay(300);
  }

  // Ramp back down to 0 safely
  dac.setVoltage(0, false);
  Serial.println("Sweep complete. Restarting in 2 seconds...");
  Serial.println("-----------------------------------------");
  delay(2000);
}

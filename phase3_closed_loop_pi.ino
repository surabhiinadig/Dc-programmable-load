/*
 * ============================================================
 *   Programmable DC Load & Supply — Phase 3: Closed-Loop PI Current Regulation
 * ============================================================
 *
 *  Goal:
 *    - Implement a PI (Proportional-Integral) software control loop
 *    - Automatically regulate load current to a user-defined setpoint
 *    - Maintain setpoint despite changes in supply voltage or temperature
 *    - Full safety enforcement on every loop cycle
 *    - Report V, I, P, setpoint, and error over Serial in real time
 *
 *  Control Algorithm:
 *    error         = target_current - measured_current
 *    integral     += error * dt
 *    integral      = clamp(integral, -INTEGRAL_LIMIT, +INTEGRAL_LIMIT)  [anti-windup]
 *    output        = (Kp * error) + (Ki * integral)
 *    new_dac_value = old_dac_value + output
 *    new_dac_value = clamp(new_dac_value, DAC_MIN, DAC_MAX)
 *
 *  Hardware Connections:
 *    Same as Phase 2 (no new hardware added in Phase 3)
 *
 *  I2C Addresses:
 *    ADS1115  -> 0x48
 *    MCP4725  -> 0x60
 *
 *  Serial Commands (send via Serial Monitor at 9600 baud):
 *    "S:0.5"   -> Set target current to 0.500 A
 *    "S:1.0"   -> Set target current to 1.000 A
 *    "S:0"     -> Turn off load (target = 0 A)
 *    "STATUS"  -> Print current PI parameters and setpoint
 *
 *  Libraries Required:
 *    - Adafruit ADS1X15
 *    - Adafruit MCP4725
 *    - Wire (built-in)
 *
 * ============================================================
 */

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_MCP4725.h>

// ─── PI Controller Parameters ──────────────────────────────
#define KP                  800.0f
#define KI                  120.0f
#define INTEGRAL_LIMIT      500.0f
#define CONTROL_LOOP_MS     50

// ─── Safety Limits ─────────────────────────────────────────
#define MAX_CURRENT_A       2.0f
#define MAX_POWER_W         10.0f
#define MAX_VOLTAGE_V       12.0f
#define MAX_SETPOINT_A      1.8f

// ─── DAC Operating Range ───────────────────────────────────
#define DAC_MIN_VALUE       0
#define DAC_MAX_VALUE       2600

// ─── Measurement Constants ─────────────────────────────────
#define SHUNT_RESISTANCE    1.0f
#define VOLTAGE_DIVIDER_RATIO 2.0f
#define PRINT_INTERVAL_MS   100

// ─── I2C Addresses ─────────────────────────────────────────
#define ADS_ADDRESS         0x48
#define DAC_ADDRESS         0x60

// ─── Globals ───────────────────────────────────────────────
Adafruit_ADS1115 ads;
Adafruit_MCP4725 dac;

float targetCurrent     = 0.0f;
float integral          = 0.0f;
float dacValue          = 0.0f;

unsigned long lastControlTime = 0;
unsigned long lastPrintTime   = 0;
bool safetyTripped = false;

// ─── Setup ─────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("=== DC Load Project — Phase 3: Closed-Loop PI Regulation ===");

  if (!ads.begin(ADS_ADDRESS)) {
    Serial.println("ERROR: ADS1115 not found! Halting.");
    while (1);
  }
  if (!dac.begin(DAC_ADDRESS)) {
    Serial.println("ERROR: MCP4725 not found! Halting.");
    while (1);
  }

  setDac(0);
  Serial.println("Hardware OK. Load OFF. Waiting for setpoint...");
  Serial.println("Send 'S:<amps>' to set target. Example: S:0.5");
  Serial.println("--------------------------------------------------------------");
  Serial.println("Setpoint(A) | Measured(A) | Error(A) | V(V) | P(W) | Status");
  Serial.println("--------------------------------------------------------------");
}

// ─── Main Loop ─────────────────────────────────────────────
void loop() {
  handleSerialInput();

  unsigned long now = millis();

  if (now - lastControlTime >= CONTROL_LOOP_MS) {
    float dt = (now - lastControlTime) / 1000.0f;
    lastControlTime = now;

    float measuredCurrent = readCurrent();
    float measuredVoltage = readSupplyVoltage();
    float measuredPower   = measuredVoltage * measuredCurrent;

    if (!checkSafety(measuredVoltage, measuredCurrent, measuredPower)) {
      safetyTripped = true;
      return;
    }

    float error = targetCurrent - measuredCurrent;
    integral   += error * dt;
    integral    = constrain(integral, -INTEGRAL_LIMIT, INTEGRAL_LIMIT);

    float output = (KP * error) + (KI * integral);
    dacValue    += output;
    dacValue     = constrain(dacValue, DAC_MIN_VALUE, DAC_MAX_VALUE);

    setDac((uint16_t)dacValue);

    if (now - lastPrintTime >= PRINT_INTERVAL_MS) {
      lastPrintTime = now;
      printStatus(targetCurrent, measuredCurrent, error, measuredVoltage, measuredPower);
    }
  }
}

// ─── Handle Serial Commands ────────────────────────────────
void handleSerialInput() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.startsWith("S:") || input.startsWith("s:")) {
      float requested = input.substring(2).toFloat();
      if (requested < 0.0f) {
        Serial.println("Invalid: Setpoint cannot be negative.");
      } else if (requested > MAX_SETPOINT_A) {
        Serial.print("Invalid: Max setpoint is ");
        Serial.print(MAX_SETPOINT_A, 2);
        Serial.println(" A");
      } else {
        targetCurrent = requested;
        integral = 0.0f;
        safetyTripped = false;
        Serial.print(">> Setpoint updated to: ");
        Serial.print(targetCurrent, 3);
        Serial.println(" A");
      }
    }
    else if (input.equalsIgnoreCase("STATUS")) {
      Serial.println("\n--- Current Parameters ---");
      Serial.print("Setpoint : "); Serial.print(targetCurrent, 3); Serial.println(" A");
      Serial.print("Kp       : "); Serial.println(KP);
      Serial.print("Ki       : "); Serial.println(KI);
      Serial.print("DAC Val  : "); Serial.println((int)dacValue);
      Serial.print("Integral : "); Serial.println(integral, 3);
      Serial.println("--------------------------\n");
    }
    else {
      Serial.println("Unknown command. Use 'S:<amps>' or 'STATUS'.");
    }
  }
}

// ─── Safety Check ──────────────────────────────────────────
bool checkSafety(float v, float i, float p) {
  if (i > MAX_CURRENT_A || v > MAX_VOLTAGE_V || p > MAX_POWER_W) {
    setDac(0);
    targetCurrent = 0.0f;
    integral = 0.0f;
    Serial.println("!!! SAFETY TRIP: Limits exceeded — DAC zeroed. Send 'S:0' to reset. !!!");
    return false;
  }
  return true;
}

// ─── Set DAC Output ────────────────────────────────────────
void setDac(uint16_t value) {
  value = constrain(value, 0, 4095);
  dac.setVoltage(value, false);
}

// ─── Read Supply Voltage ───────────────────────────────────
float readSupplyVoltage() {
  ads.setGain(GAIN_ONE);
  int16_t raw = ads.readADC_SingleEnded(2);
  return ads.computeVolts(raw) * VOLTAGE_DIVIDER_RATIO;
}

// ─── Read Current via Shunt ────────────────────────────────
float readCurrent() {
  ads.setGain(GAIN_SIXTEEN);
  int16_t raw = ads.readADC_Differential_0_1();
  float current = ads.computeVolts(raw) / SHUNT_RESISTANCE;
  return max(0.0f, current);
}

// ─── Print Status to Serial ────────────────────────────────
void printStatus(float sp, float meas, float err, float v, float p) {
  Serial.print(sp, 3);
  Serial.print(" A      | ");
  Serial.print(meas, 4);
  Serial.print(" A    | ");
  Serial.print(err, 4);
  Serial.print(" A | ");
  Serial.print(v, 3);
  Serial.print(" V | ");
  Serial.print(p, 3);
  Serial.print(" W | ");
  Serial.println(safetyTripped ? "TRIP" : "OK");
}

# Programmable DC Electronic Load

A digitally programmable DC electronic load using Arduino, ADS1115 ADC, MCP4725 DAC, LM358 Op-Amp, and IRF3205 MOSFET.

---

## Project Status: 40% Complete

| Phase | Description | Status |
|-------|-------------|--------|
| Phase 1 | Hardware Bring-Up & Sensing | ✅ Complete |
| Phase 2 | MOSFET Control via DAC | ⏳ In Progress |
| Phase 3 | Closed-Loop Current Regulation | 🔲 Pending |

---

## Hardware

| Component | Value / Model | Role |
|-----------|--------------|------|
| Arduino Uno/Nano | — | Microcontroller |
| IRF3205PBF | N-Channel MOSFET | Main switching element |
| ADS1115 | 16-bit ADC | Voltage & current sensing |
| MCP4725 | 12-bit DAC | Gate voltage control |
| LM358P | Op-Amp | Buffers DAC → drives MOSFET gate |
| Shunt Resistor | 1Ω, 3W | Current sensing |
| Gate Resistor | 100Ω | Gate inrush protection |
| Bleed Resistor | 1kΩ (x2) | Prevents floating gate |
| Decoupling Cap | 220nF | Gate oscillation damping |

---

## Pin Connections

### ADS1115 (I2C Address: 0x48)
| ADS1115 Pin | Connects To |
|-------------|-------------|
| VDD | Arduino 5V |
| GND | Arduino GND |
| SCL | Arduino A5 |
| SDA | Arduino A4 |
| A0 | DC Supply + |
| A1 | After shunt resistor |

### MCP4725 DAC (I2C Address: 0x60)
| MCP4725 Pin | Connects To |
|-------------|-------------|
| VDD | Arduino 5V |
| GND | Arduino GND |
| SCL | Arduino A5 (shared) |
| SDA | Arduino A4 (shared) |
| VOUT | LM358 Pin 3 (IN+) |

### LM358 Op-Amp
| LM358 Pin | Connects To |
|-----------|-------------|
| Pin 3 IN+ | MCP4725 VOUT |
| Pin 2 IN– | GND |
| Pin 1 OUT | 100Ω → MOSFET Gate |
| Pin 8 VCC | Arduino 5V |
| Pin 4 GND | Arduino GND |

### IRF3205 MOSFET
| MOSFET Pin | Connects To |
|------------|-------------|
| Gate (G) | LM358 OUT via 100Ω |
| Drain (D) | Shunt Resistor Terminal 2 |
| Source (S) | GND |

---

## Files

```
├── phase1_sensing.ino          # Phase 1: ADC sensing, V/I/P measurement
├── phase2a_dac_bringup.ino     # Phase 2: DAC init and sweep test
├── phase2b_mosfet_gate_control.ino  # Phase 2: Full gate control + safety
└── README.md
```

---

## Libraries Required

Install via Arduino Library Manager:
- `Adafruit ADS1X15`
- `Adafruit MCP4725`

---

## Safety Limits
- Max input voltage: 12V DC
- Max current: 2A (enforced in firmware)
- No mains/AC voltages used
- Overcurrent protection halts system automatically

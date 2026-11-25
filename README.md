# Basic Battery Management System (BMS) – ESP32 + INA226

Monitors a 3.7 V Li-ion cell and estimates its **State of Charge (SoC)** by combining a voltage lookup table (at start-up) with **coulomb counting** (current × time). Output appears on the Serial Monitor.

## Repository structure
```
bms-esp32-ina226/
├── hardware/                  # Real-world version (physical ESP32 + INA226 + battery)
│   └── bms_soc/
│       └── bms_soc.ino
├── wokwi-simulation/          # Simulated version (runs in Wokwi, no hardware needed)
│   ├── sketch.ino             # Same BMS code (BATTERY_MAH = 100 for a quick demo)
│   ├── diagram.json           # Circuit: ESP32, INA226, Li-ion cell, load knob
│   ├── ina226.chip.c / .json  # Simulated INA226 sensor (I2C, address 0x40)
│   ├── li-ion.chip.c / .json  # Simulated 3.7 V Li-ion battery
│   └── libraries.txt          # INA226 library
├── docs/
│   ├── report.tex / report.pdf # Full project report (LaTeX)
│   ├── project-report.md      # Report text in Markdown
│   └── block-diagram.md       # Block diagram, connections, data flow
├── images/                    # Screenshots of circuit and output
├── .gitignore
├── LICENSE
└── README.md
```

📄 Full report: [docs/report.pdf](docs/report.pdf) · 🔌 Wiring: [docs/block-diagram.md](docs/block-diagram.md)

## 1. Hardware version
**Components:** ESP32 dev board, INA226 module (0.1 Ω shunt), 3.7 V Li-ion cell, load (e.g. 10 Ω resistor), jumper wires.

| From | To |
|---|---|
| Battery + | INA226 IN+ |
| INA226 IN- | Load + |
| Load - | Battery - |
| INA226 GND | Battery - and ESP32 GND |
| INA226 VCC | ESP32 3.3V |
| INA226 SDA / SCL | ESP32 GPIO 21 / 22 |

**Run:**
1. Arduino IDE → install ESP32 board package and the **INA226** library by Rob Tillaart.
2. Open `hardware/bms_soc/bms_soc.ino`, set `BATTERY_MAH` to your cell's capacity.
3. Board: **ESP32 Dev Module** → Upload → Serial Monitor at **115200**.
4. Power up with the load disconnected so the starting SoC is read at rest.

## 2. Wokwi simulation
Wokwi has no INA226 or battery part, so both are written as custom chips. The INA226 chip responds over I2C like the real sensor, so the BMS code runs unchanged.

- **Live project:** https://wokwi.com/projects/481598958521700353
- Turn the **load knob** right to discharge, left to charge.
- **Chips Console** shows the simulated battery's actual charge every 5 s – compare it with the ESP32's SoC.

## How SoC is calculated
1. **Start-up:** battery voltage → SoC % from a Li-ion lookup table (linear interpolation).
2. **Every second:** `remaining mAh -= current (mA) × time (h)`
3. `SoC % = remaining mAh / capacity mAh × 100`

| SoC | Status |
|---|---|
| ≤ 20 % | LOW |
| 21–94 % | OK |
| ≥ 95 % | FULL |

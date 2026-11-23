# Block Diagram & Circuit

## Block diagram
```
┌──────────────┐   IN+   ┌──────────┐   IN−   ┌──────────────┐
│ Li-ion 3.7 V │────────►│  INA226  │────────►│ Load         │
│   Battery    │         │  Sensor  │         └──────────────┘
└──────┬───────┘         └────┬─────┘
       │ GND                  │ I2C (SDA = GPIO21, SCL = GPIO22)
       │                      ▼
       │               ┌──────────────┐   USB serial   ┌────────────────┐
       └──────────────►│    ESP32     │───────────────►│ Serial Monitor │
          common GND   └──────────────┘                └────────────────┘
```

## Connections
| From | To | Purpose |
|---|---|---|
| Battery + | INA226 IN+ | Battery voltage into the sensor |
| INA226 IN− | Load + | All current passes through the 0.1 Ω shunt |
| Load − | Battery − | Completes the circuit |
| INA226 SDA | ESP32 GPIO 21 | I2C data |
| INA226 SCL | ESP32 GPIO 22 | I2C clock |
| INA226 VCC | ESP32 3.3 V | Sensor power |
| INA226 GND | Battery − and ESP32 GND | Common ground reference |

## Wokwi simulation wiring
| From | To |
|---|---|
| Li-ion chip BAT | INA226 chip INP (IN+) |
| INA226 chip INN (IN−) | Load knob SIG |
| Load knob SIG | Li-ion chip LOAD |
| INA226 chip SDA / SCL | ESP32 GPIO 21 / 22 |
| All VCC / GND | ESP32 3V3 / GND |

## Data flow
1. Battery supplies current to the load through the INA226.
2. INA226 measures voltage and current, stores them in its registers.
3. ESP32 reads the registers over I2C every second.
4. ESP32 calculates SoC (voltage table at start-up, coulomb counting after).
5. Results are printed on the Serial Monitor.

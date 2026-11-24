# Project Report – Basic Battery Management System (ESP32 + INA226)

**Wokwi project:** https://wokwi.com/projects/481598958521700353

---

## 1. Project Summary

This project is a basic Battery Management System (BMS) that monitors a 3.7 V Li-ion battery and shows how much charge is left in it. Many simple devices guess battery level only from voltage, which is not accurate because voltage drops under load and rises again at rest. This leads to sudden shutdowns, over-discharge and shorter battery life.

Our system uses an ESP32 microcontroller and an INA226 sensor. The INA226 measures the battery voltage and the current flowing in or out through a 0.1 Ω shunt resistor, and sends this data to the ESP32 over I2C. At start-up, the ESP32 estimates the State of Charge (SoC) from the battery voltage using a lookup table. After that, it tracks the charge every second using coulomb counting (current × time), which gives a smooth and accurate reading.

The system displays voltage, current, mode (Charging, Discharging or Idle), SoC percentage, remaining capacity and status (LOW, OK or FULL) on the Serial Monitor. Since the physical sensor was not available, the project was built and tested in the Wokwi simulator using custom chips that act as the INA226 and a Li-ion battery. The same code runs on real hardware with only one setting changed.

The system is low-cost, simple, and can be extended with Wi-Fi monitoring, battery protection and temperature sensing for uses like e-bikes, solar storage and IoT devices.

---

## 2. Novelty and Applications

The main novelty is combining two SoC methods in a simple, low-cost design. Voltage-based estimation is used only once at start-up to find the starting point; after that, coulomb counting tracks the actual charge flowing in and out. Many low-cost battery indicators use only voltage, which jumps up and down with load. Our method gives a smooth and more reliable reading.

The complete system was first built and tested in the Wokwi simulator with a simulated battery and INA226 sensor. The same code can run on real hardware with only the battery capacity changed. This makes the project easy to test, cheap to develop and safe, since no real battery is needed during development.

**Real-time applications:** e-bikes and electric scooters, solar and home backup battery systems, portable medical devices, IoT sensor nodes and wearables, robotics and drones.

Compared with commercial BMS chips, which are costly and complex, our system is simple, cheap and easy to modify. Because the ESP32 has built-in Wi-Fi, it can be extended to send battery data to a phone or cloud dashboard.

---

## 3. Functions, Operating Conditions and Component Selection

**Major functions**
- Measure battery voltage and current using the INA226
- Estimate starting SoC from battery voltage
- Track charge used/added through coulomb counting
- Detect mode: Charging, Discharging or Idle
- Show status: LOW (≤ 20 %), OK, FULL (≥ 95 %)

**Operating conditions and performance**
- Battery: single Li-ion cell, 3.0 V – 4.2 V (3.7 V nominal)
- System voltage: 3.3 V (ESP32 and INA226)
- Current range: up to about 0.8 A with a 0.1 Ω shunt
- Response time: readings and SoC update every 1 second
- Accuracy: INA226 is a 16-bit sensor with 1.25 mV voltage resolution
- Power: INA226 draws well under 1 mA; ESP32 can use sleep modes in future
- Reliability: SoC limited between 0–100 %; sensor connection checked at start-up

**Component selection**
- **ESP32** – low cost, fast, built-in I2C and Wi-Fi/Bluetooth
- **INA226** – accurate voltage and current in one chip over two I2C wires
- **Li-ion 3.7 V cell** – most common rechargeable battery in portable devices
- **Serial Monitor** – simple display, no extra hardware
- **Potentiometer (simulation)** – variable load to test charging and discharging

---

## 4. Hardware and Software Implementation

**Hardware**
- **ESP32** – reads sensor data, calculates SoC and prints results.
- **INA226** – measures battery voltage and current. Current flows through a 0.1 Ω shunt resistor; the sensor measures the voltage drop across it (Ohm's law).
- **3.7 V Li-ion cell** – the battery being monitored (3.0 V empty, 4.2 V full).
- **Load** – uses the battery's power. In simulation, a potentiometer acts as an adjustable load.
- **Serial Monitor** – shows readings every second.

**Software** (Arduino C++, INA226 library by Rob Tillaart)
- **Start-up** – starts I2C, checks the sensor, calibrates it for the shunt resistor.
- **Initial SoC** – reads battery voltage and finds starting SoC from a lookup table using linear interpolation.
- **Coulomb counting** – every second, current × time is subtracted from the remaining charge (mAh).
- **Status logic** – calculates SoC %, detects mode and labels LOW / OK / FULL.
- **Output** – prints voltage, current, mode, SoC and remaining mAh.

---

## 5. Testing

The project was tested in Wokwi. Since Wokwi has no INA226 or battery part, two custom chips were created: a 3.7 V Li-ion cell whose voltage falls as it discharges, and an INA226 sensor that behaves like the real one over I2C.

**Tests performed**
- **Sensor connection** – ESP32 detects the INA226 at start-up.
- **Discharge** – load knob turned right; SoC and voltage go down.
- **Charge** – knob turned left; SoC goes up, mode shows CHARGING.
- **Idle** – no current; SoC stays constant.
- **Accuracy** – Chips Console prints the battery's actual charge every 5 s; the ESP32's SoC stayed closely matched.
- **Manual calculation** – 330 mA for 1 s = 330 × (1/3600) ≈ 0.092 mAh used, matching the change on screen.

**Issues encountered**
- Potentiometers started at zero, giving a wrong starting SoC – fixed by setting default knob positions.
- Knob middle position did not give exactly 0 mA – added a ±20 mA dead zone.
- Voltage table was first written for a 3.3 V LiFePO4 cell – updated for 3.7 V Li-ion.
- Wokwi had no INA226 or battery part – solved by writing custom chips.

---

## 6. Integration

**Battery → INA226 → I2C → ESP32 → Serial Monitor**

1. Battery + connects to INA226 IN+, the load connects to IN−, so all current flows through the shunt.
2. INA226 connects to the ESP32 over I2C (SDA = GPIO 21, SCL = GPIO 22) with 3.3 V power and a common ground.
3. The INA226 library handles I2C communication; the ESP32 writes a calibration value and reads voltage and current registers.
4. The ESP32 uses the voltage table once at start-up and coulomb counting continuously after.
5. Results are sent over USB serial every second.

Each module was tested on its own first, then combined. Because the simulated chips behave like the real INA226 and battery, the same code works in simulation and on real hardware.

---

## 7. Efficiency and Performance

- **Communication** – I2C needs only two wires and a few bytes per reading.
- **Processing** – light calculations finish in microseconds; a 1-second interval suits slow battery changes.
- **Memory** – a few variables and two small fixed arrays; no dynamic memory allocation.
- **Sensor accuracy** – high-precision INA226; shunt voltage added back for true battery voltage; coulomb counting avoids voltage dips under load.
- **Reliability** – SoC limited to 0–100 %; sensor checked before starting.
- **Cost** – only two main components; simulation avoided buying or risking a real battery.

**Future improvements:** ESP32 deep sleep, averaging readings to reduce noise, SoC re-correction at rest, temperature sensor and protection cut-off, Wi-Fi dashboard.

---

## 8. Working Principle and Technical Decisions

**Working principle**

The system works like a fuel meter for a battery. All current going into or out of the battery passes through a small 0.1 Ω shunt resistor inside the INA226. The INA226 measures the tiny voltage drop across it to find the current (I = V / R), and also measures the battery voltage. The ESP32 reads these values over I2C.

The SoC is found in two steps. At start-up, the battery voltage is compared with a Li-ion lookup table to find the starting charge, for example 4.0 V means about 80 %. After that, every second, the charge used or added is calculated as current × time and subtracted from the remaining charge. This is called coulomb counting. The SoC percentage is remaining mAh ÷ capacity mAh × 100.

**Response to different conditions**

When a load is connected, the current is positive, the system shows DISCHARGING, the SoC decreases and the voltage drops slightly. When a charger is connected, the current is negative, the system shows CHARGING and the SoC increases. When nothing is connected, the current is about zero, the system shows IDLE and the SoC stays the same. If the SoC falls to 20 % or below, the status changes to LOW; at 95 % or above it shows FULL. If the INA226 is not connected, the system prints an error and stops safely instead of showing wrong values.

**Technical decisions**

Coulomb counting is used along with the voltage table because battery voltage changes with load, so voltage alone gives jumpy and inaccurate readings. The INA226 was chosen because it measures both voltage and current accurately in one chip using only two I2C wires. The ESP32 was chosen for its low cost, speed, built-in I2C and Wi-Fi for future upgrades. The shunt voltage is added back to the bus voltage to get the true battery voltage. SoC is limited between 0 % and 100 % so small errors can't give impossible values. Testing in Wokwi allowed safe, low-cost development before using a real battery.

---

## 9. Implementation Process

1. **Requirement study** – focused on SoC estimation as the core BMS function.
2. **Component selection** – ESP32 and INA226.
3. **Algorithm design** – voltage table for start-up + coulomb counting.
4. **First simulation** – two potentiometers faked voltage and current to test the logic.
5. **Realistic simulation** – custom Wokwi chips for INA226 and Li-ion battery so the real code runs unchanged.
6. **Testing and fixes** – charging, discharging, idle; fixed knob positions and battery table.

**Key decisions:** same code for simulation and hardware; 100 mAh simulated battery so changes are visible within minutes; Serial Monitor as display.

---

## 10. Results, Conclusions, Limitations and Future Work

**Results**
- Voltage and current read correctly from the INA226 over I2C.
- SoC estimated at start-up and tracked smoothly by coulomb counting.
- Charging, Discharging and Idle modes and LOW / OK / FULL status worked correctly.
- ESP32's SoC closely matched the simulated battery's actual charge.

**Conclusions:** a simple, low-cost BMS can give a reliable battery level using an ESP32 and INA226. Combining voltage estimation with coulomb counting works better than voltage alone. Building in simulation first saved cost and time, and the code is ready for real hardware.

**Limitations**
- Tested only in simulation.
- Coulomb counting can drift slowly over long periods.
- No temperature sensing or protection cut-off.
- Single cell only.
- Serial Monitor needs a computer.

**Future improvements**
- Build and test on real hardware.
- MOSFET cut-off for over-charge / over-discharge protection.
- Temperature sensor.
- SoC re-correction at rest.
- Multi-cell (series) support.
- OLED display or Wi-Fi mobile dashboard.
- Deep sleep for lower power.

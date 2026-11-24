/*
  Basic BMS - State of Charge (SoC) Monitor
  Hardware : ESP32 + INA226 + one 3.7 V Li-ion cell
  Display  : Serial Monitor @ 115200 baud
  Library  : "INA226" by Rob Tillaart

  How SoC is calculated:
   1. At start-up, SoC is estimated from the battery voltage (lookup table).
   2. After that, current is measured every second and added up
      (coulomb counting) to track how much charge is left.

  Same code runs on the real board and in Wokwi - only BATTERY_MAH changes.
*/

#include <Wire.h>
#include <INA226.h>

INA226 ina(0x40);

// ---------------- Settings ----------------
const float BATTERY_MAH = 100.0;    // Wokwi cell = 100 mAh (drains in minutes). Real cell: e.g. 2000
const float SHUNT_OHM   = 0.1;      // shunt resistor on INA226 board ("R100")
const float MAX_AMPS    = 0.8;      // max current for a 0.1 ohm shunt

// Resting voltage -> SoC table for a 3.7 V Li-ion cell
const float volts[] = {3.00, 3.30, 3.60, 3.67, 3.71, 3.75, 3.79, 3.85, 3.92, 4.00, 4.10, 4.20};
const float socs[]  = {   0,    5,   10,   20,   30,   40,   50,   60,   70,   80,   90,  100};
const int   POINTS  = 12;
// ------------------------------------------

float remainingMah;          // charge left in the battery (mAh)
unsigned long lastTime;      // time of last reading (ms)


// Find SoC (%) from voltage using the table above
float socFromVoltage(float v) {
  if (v <= volts[0])          return 0;
  if (v >= volts[POINTS - 1]) return 100;

  for (int i = 1; i < POINTS; i++) {
    if (v < volts[i]) {
      // straight line between the two nearest table points
      return socs[i - 1] + (v - volts[i - 1]) * (socs[i] - socs[i - 1]) / (volts[i] - volts[i - 1]);
    }
  }
  return 100;
}


void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(21, 22);                       // SDA = 21, SCL = 22

  if (!ina.begin()) {
    Serial.println("INA226 not found. Check wiring.");
    while (true) delay(1000);
  }
  ina.setMaxCurrentShunt(MAX_AMPS, SHUNT_OHM, false);

  // Step 1: starting SoC from battery voltage
  float startVoltage = ina.getBusVoltage() + ina.getShuntVoltage();
  float startSoc     = socFromVoltage(startVoltage);
  remainingMah       = BATTERY_MAH * startSoc / 100.0;

  Serial.println("===== Basic BMS - SoC Monitor =====");
  Serial.printf("Start voltage: %.3f V  ->  Start SoC: %.1f %%\n\n", startVoltage, startSoc);

  lastTime = millis();
}


void loop() {
  // Read sensor
  float voltage = ina.getBusVoltage() + ina.getShuntVoltage();   // battery voltage (V)
  float current = ina.getCurrent_mA();                           // + = discharging, - = charging

  // Step 2: coulomb counting
  unsigned long now = millis();
  float hours = (now - lastTime) / 3600000.0;     // time since last reading, in hours
  lastTime = now;

  remainingMah = remainingMah - current * hours;  // mAh used (or added) since last reading
  if (remainingMah < 0)           remainingMah = 0;
  if (remainingMah > BATTERY_MAH) remainingMah = BATTERY_MAH;

  float soc = remainingMah / BATTERY_MAH * 100.0;

  // Charging or discharging?
  const char* mode;
  if (current > 20)       mode = "DISCHARGING";
  else if (current < -20) mode = "CHARGING   ";
  else                    mode = "IDLE       ";

  // Battery status based on SoC
  const char* status;
  if (soc <= 20)      status = "LOW";
  else if (soc >= 95) status = "FULL";
  else                status = "OK";

  // Print to Serial Monitor
  Serial.printf("Voltage: %.3f V | Current: %6.1f mA | %s | SoC: %5.1f %% | Left: %5.1f mAh | %s\n",
                voltage, current, mode, soc, remainingMah, status);

  delay(1000);
}

// Wokwi custom chip: 3.7 V Li-ion cell (simulated battery)
// --------------------------------------------------------
// - BAT  : battery + terminal (its voltage goes to INA226 IN+)
// - LOAD : reads the load knob
//     left = charging (-800 mA) | middle = 0 mA | right = discharging (+800 mA)
// - The cell is drained / charged by that current, so its voltage
//   drops and rises like a real battery.
// - "Chips Console" tab prints the cell's actual charge every 5 s
//   (compare it with the SoC the ESP32 calculates).

#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define R_INTERNAL  0.10f    // internal resistance (ohm) -> voltage sag under load
#define MAX_MA      800.0f   // load knob range
#define STEP_MS     100      // update interval

// Li-ion resting voltage vs charge (%)
static const float OCV_V[]   = {3.00f, 3.30f, 3.60f, 3.67f, 3.71f, 3.75f, 3.79f, 3.85f, 3.92f, 4.00f, 4.10f, 4.20f};
static const float OCV_SOC[] = {0, 5, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
#define OCV_N 12

typedef struct {
  pin_t    bat_pin;
  pin_t    load_pin;
  float    capacity_mah;   // battery size
  float    charge_mah;     // charge inside the battery
  uint32_t ticks;
} chip_state_t;


// Resting voltage for a given charge %
static float voltage_from_soc(float soc) {
  if (soc <= 0)   return OCV_V[0];
  if (soc >= 100) return OCV_V[OCV_N - 1];
  for (int i = 1; i < OCV_N; i++) {
    if (soc < OCV_SOC[i]) {
      float t = (soc - OCV_SOC[i - 1]) / (OCV_SOC[i] - OCV_SOC[i - 1]);
      return OCV_V[i - 1] + t * (OCV_V[i] - OCV_V[i - 1]);
    }
  }
  return OCV_V[OCV_N - 1];
}


// Runs every 100 ms
static void on_timer(void *user_data) {
  chip_state_t *chip = user_data;

  // Load knob -> current
  float knob = pin_adc_read(chip->load_pin) / 3.3f;          // 0.0 ... 1.0
  float ma   = (knob * 2.0f - 1.0f) * MAX_MA;
  if (fabsf(ma) < 20) ma = 0;                                 // dead zone around middle

  // Drain / charge
  chip->charge_mah -= ma * (STEP_MS / 3600000.0f);
  if (chip->charge_mah < 0)                  chip->charge_mah = 0;
  if (chip->charge_mah > chip->capacity_mah) chip->charge_mah = chip->capacity_mah;

  // Terminal voltage = resting voltage - sag from internal resistance
  float soc = chip->charge_mah / chip->capacity_mah * 100.0f;
  float v   = voltage_from_soc(soc) - (ma / 1000.0f) * R_INTERNAL;
  pin_dac_write(chip->bat_pin, v);

  if (++chip->ticks % 50 == 0) {                              // every 5 s
    printf("Battery (actual): %5.1f %% | %.3f V | %6.1f mA\n", soc, v, ma);
  }
}


void chip_init(void) {
  chip_state_t *chip = calloc(1, sizeof(chip_state_t));

  chip->bat_pin      = pin_init("BAT", ANALOG);
  chip->load_pin     = pin_init("LOAD", ANALOG);
  chip->capacity_mah = attr_read_float(attr_init_float("capacityMah", 100.0f));
  float start_soc    = attr_read_float(attr_init_float("startSoc", 80.0f));
  chip->charge_mah   = chip->capacity_mah * start_soc / 100.0f;
  pin_dac_write(chip->bat_pin, voltage_from_soc(start_soc));

  const timer_config_t timer_config = { .callback = on_timer, .user_data = chip };
  timer_t timer = timer_init(&timer_config);
  timer_start(timer, STEP_MS * 1000, true);

  printf("Li-ion cell: %.0f mAh, starting at %.0f %% (%.3f V)\n",
         chip->capacity_mah, start_soc, voltage_from_soc(start_soc));
}

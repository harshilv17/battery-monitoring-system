// Wokwi custom chip: INA226 current / voltage sensor
// --------------------------------------------------
// - Talks to the ESP32 over I2C (address 0x40) exactly like a real INA226,
//   so the normal INA226 Arduino library works unchanged.
// - INP (= IN+) : connect to battery +   -> measures battery voltage
// - INN (= IN-) : connect to the load    -> load knob sets the current
//     left = charging (-800 mA) | middle = 0 mA | right = discharging (+800 mA)

#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// INA226 registers
#define REG_SHUNT        0x01
#define REG_BUS          0x02
#define REG_POWER        0x03
#define REG_CURRENT      0x04
#define REG_CALIBRATION  0x05

#define SHUNT_OHM  0.1f      // same shunt as the real module
#define MAX_MA     800.0f    // load knob range

typedef struct {
  pin_t    inp_pin;         // IN+ (battery)
  pin_t    inn_pin;         // IN- (load)
  uint16_t regs[8];         // INA226 registers 0x00..0x07
  uint8_t  reg_ptr;         // register selected by the ESP32
  uint8_t  write_count;
  uint8_t  write_msb;
  uint8_t  read_count;
  uint16_t read_value;
} chip_state_t;


// Register values the ESP32 reads (same maths as the real INA226)
static uint16_t read_register(chip_state_t *chip, uint8_t reg) {
  float battery_v = pin_adc_read(chip->inp_pin);              // voltage on IN+

  float knob = pin_adc_read(chip->inn_pin) / 3.3f;            // load setting 0..1
  float ma   = (knob * 2.0f - 1.0f) * MAX_MA;
  if (fabsf(ma) < 20) ma = 0;

  float shunt_v = (ma / 1000.0f) * SHUNT_OHM;                 // drop across shunt
  float bus_v   = battery_v - shunt_v;                        // measured after the shunt

  int16_t  shunt_reg   = (int16_t)lroundf(shunt_v / 2.5e-6f);     // 2.5 uV per bit
  uint16_t bus_reg     = (uint16_t)lroundf(bus_v / 1.25e-3f);     // 1.25 mV per bit
  int16_t  current_reg = (int16_t)((int32_t)shunt_reg * chip->regs[REG_CALIBRATION] / 2048);

  switch (reg) {
    case REG_SHUNT:   return (uint16_t)shunt_reg;
    case REG_BUS:     return bus_reg;
    case REG_CURRENT: return (uint16_t)current_reg;
    case REG_POWER:   return (uint16_t)((uint32_t)abs(current_reg) * bus_reg / 20000);
    case 0xFE:        return 0x5449;   // manufacturer ID "TI"
    case 0xFF:        return 0x2260;   // die ID
    default:          return (reg < 8) ? chip->regs[reg] : 0;
  }
}


// ---------------- I2C ----------------
static bool on_i2c_connect(void *user_data, uint32_t address, bool read) {
  chip_state_t *chip = user_data;
  chip->write_count = 0;
  chip->read_count  = 0;
  return true;                                   // ACK
}

static bool on_i2c_write(void *user_data, uint8_t data) {
  chip_state_t *chip = user_data;
  if (chip->write_count == 0) {
    chip->reg_ptr = data;                        // 1st byte = register number
  } else if (chip->write_count == 1) {
    chip->write_msb = data;                      // 2nd byte = high byte
  } else if (chip->write_count == 2 && chip->reg_ptr < 8) {
    chip->regs[chip->reg_ptr] = (chip->write_msb << 8) | data;   // 3rd byte = low byte
  }
  chip->write_count++;
  return true;
}

static uint8_t on_i2c_read(void *user_data) {
  chip_state_t *chip = user_data;
  if (chip->read_count == 0) chip->read_value = read_register(chip, chip->reg_ptr);
  uint8_t data = (chip->read_count % 2 == 0) ? (chip->read_value >> 8) : (chip->read_value & 0xFF);
  chip->read_count++;
  return data;                                   // high byte first, then low byte
}

static void on_i2c_disconnect(void *user_data) {}


// ---------------- Start-up ----------------
void chip_init(void) {
  chip_state_t *chip = calloc(1, sizeof(chip_state_t));
  chip->inp_pin = pin_init("INP", ANALOG);
  chip->inn_pin = pin_init("INN", ANALOG);
  chip->regs[0] = 0x4127;                        // INA226 default config

  const i2c_config_t i2c_config = {
    .user_data  = chip,
    .address    = 0x40,
    .scl        = pin_init("SCL", INPUT_PULLUP),
    .sda        = pin_init("SDA", INPUT_PULLUP),
    .connect    = on_i2c_connect,
    .read       = on_i2c_read,
    .write      = on_i2c_write,
    .disconnect = on_i2c_disconnect,
  };
  i2c_init(&i2c_config);
}

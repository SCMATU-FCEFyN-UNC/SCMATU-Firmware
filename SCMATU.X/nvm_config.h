#ifndef NVM_CONFIG_H
#define NVM_CONFIG_H

#include "mcc_generated_files/system/system.h"
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

// Base address for your configuration data is EEPROM_START_ADDRESS
// The size is EEPROM_SIZE          (256U)
#define EEPROM_CONFIG_MAGIC                     0xA5                        // Password for writing

// EEPROM addresses for holding registers
#define EEPROM_MAGIC_ADDR                       (EEPROM_START_ADDRESS + 0)  // Check whether the memory has already been written or not
#define EEPROM_ADDR_SLAVE_ADDR                  (EEPROM_START_ADDRESS + 1)  // Holding Register 0
#define EEPROM_BAUDRATE_ADDR                    (EEPROM_START_ADDRESS + 3)  // Holding Register 1
#define SENSOR_TYPE_ADDR                        (EEPROM_START_ADDRESS + 5)  // Input Register 0
#define SERIAL_NUMBER_ADDR                      (EEPROM_START_ADDRESS + 7)  // Input Register 1

#define EEPROM_FREQ_HI_ADDR                     (EEPROM_START_ADDRESS + 9)  // Holding Register 2
#define EEPROM_FREQ_LO_ADDR                     (EEPROM_START_ADDRESS + 11) // Holding Register 3
#define EEPROM_VOLT_LVL_ADDR                    (EEPROM_START_ADDRESS + 13) // Holding Register 4

#define EEPROM_ON_TIME_MS_ADDR                  (EEPROM_START_ADDRESS + 15) // Holding Register 5
#define EEPROM_OFF_TIME_MS_ADDR                 (EEPROM_START_ADDRESS + 17) // Holding Register 5

#define EEPROM_FREQ_MODE_ADDR                   (EEPROM_START_ADDRESS + 19) // Holding Register 6

#define EEPROM_SAMPLES_AMOUNT_ADDR              (EEPROM_START_ADDRESS + 21) // Holding Register 7
#define EEPROM_FREQ_STEP_ADDR                   (EEPROM_START_ADDRESS + 23) // Holding Register 9
#define EEPROM_FREQ_RANGE_START_HI_ADDR         (EEPROM_START_ADDRESS + 25) // Holding Register 10 - freq_range_start_hi
#define EEPROM_FREQ_RANGE_START_LO_ADDR         (EEPROM_START_ADDRESS + 27) // Holding Register 11 - freq_range_start_lo
#define EEPROM_FREQ_RANGE_END_HI_ADDR           (EEPROM_START_ADDRESS + 29) // Holding Register 12 - freq_range_end_hi
#define EEPROM_FREQ_RANGE_END_LO_ADDR           (EEPROM_START_ADDRESS + 31) // Holding Register 13 - freq_range_end_lo

#define EEPROM_VOLT_AD_GAIN_ADDR                (EEPROM_START_ADDRESS + 33) // Holding Register 14
#define EEPROM_CURR_AD_GAIN_ADDR                (EEPROM_START_ADDRESS + 35) // Holding Register 15
#define EEPROM_SHUNT_RES_ADDR                   (EEPROM_START_ADDRESS + 37) // Holding Register 16
#define EEPROM_ADC_SAMPLES_ADDR                 (EEPROM_START_ADDRESS + 39) // Holding Register 17
#define EEPROM_MAX_PHASE_CURR_ADDR              (EEPROM_START_ADDRESS + 41) // Holding Register 18
#define EEPROM_AUTO_FREQ_SWEEP_WIDTH_ADDR       (EEPROM_START_ADDRESS + 43) // Holding Register 19
#define EEPROM_CLOSED_LOOP_CONTROL_ENABLE_ADDR  (EEPROM_START_ADDRESS + 45) // Holding Register 20
#define EEPROM_CLOSED_LOOP_CONTROL_PERIOD_ADDR  (EEPROM_START_ADDRESS + 47) // Holding Register 21

#define EEPROM_RES_FREQ_HI_ADDR                 (EEPROM_START_ADDRESS + 49) // Input Register 10
#define EEPROM_RES_FREQ_LO_ADDR                 (EEPROM_START_ADDRESS + 51) // Input Register 11

#define EEPROM_BEST_PHASE_FREQ_HI_ADDR          (EEPROM_START_ADDRESS + 53) // Input Register 14
#define EEPROM_BEST_PHASE_FREQ_LO_ADDR          (EEPROM_START_ADDRESS + 55) // Input Register 15

#define EEPROM_BEST_CURR_FREQ_HI_ADDR           (EEPROM_START_ADDRESS + 57) // Input Register 18
#define EEPROM_BEST_CURR_FREQ_LO_ADDR           (EEPROM_START_ADDRESS + 59) // Input Register 19

#define EEPROM_SERIAL_NUMBER_IN_ADDR            (EEPROM_START_ADDRESS + 61) // Holding Register 19
#define EEPROM_SN_PASSWORD_ADDR                 (EEPROM_START_ADDRESS + 63) // Holding Register 20
#define EEPROM_SN_WRITE_STATUS_ADDR             (EEPROM_START_ADDRESS + 61) // Holding Register 21

// Function prototype
uint16_t EEPROM_ReadWord(uint16_t address);
bool EEPROM_WriteWord(uint16_t address, uint16_t value);

#endif

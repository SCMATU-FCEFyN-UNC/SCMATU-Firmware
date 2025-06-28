#ifndef NVM_CONFIG_H
#define NVM_CONFIG_H

#include "mcc_generated_files/system/system.h"
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

// Base address for your configuration data is EEPROM_START_ADDRESS
// The size is EEPROM_SIZE          (256U)
#define EEPROM_CONFIG_MAGIC         0xA5

// EEPROM addresses for holding registers
#define EEPROM_MAGIC_ADDR           (EEPROM_START_ADDRESS + 0)      // Check whether the memory has already been written or not
#define EEPROM_ADDR_SLAVE_ADDR      (EEPROM_START_ADDRESS + 1)      // Holding Register 0
#define EEPROM_BAUDRATE_ADDR        (EEPROM_START_ADDRESS + 3)      // Holding Register 1
#define SENSOR_TYPE_ADDR            (EEPROM_START_ADDRESS + 5)      // Input Register 0
#define SERIAL_NUMBER_ADDR          (EEPROM_START_ADDRESS + 7)      // Input Register 1

// Function prototype
uint16_t EEPROM_ReadWord(uint16_t address);
bool EEPROM_WriteWord(uint16_t address, uint16_t value);

#endif

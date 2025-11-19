#ifndef SN_HANDLER_H
#define SN_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

// Use constants defined in modbus_imp.h:
#include "modbus_imp.h"

// API
void sn_write_handler(uint16_t *status_reg);
void sn_submit_password(uint16_t password, uint16_t *status_reg);
bool sn_attempt_write(uint16_t serial_in, mod_bus_registers* modbus_data);
bool sn_is_write_enabled(void);

#endif
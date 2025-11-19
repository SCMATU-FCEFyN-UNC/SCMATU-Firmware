#ifndef CLOSED_LOOP_CONTROL_H
#define CLOSED_LOOP_CONTROL_H

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "mcc_generated_files/system/system.h"
#include "modbus_imp.h"

void disable_closed_loop_timer();
void enable_closed_loop_timer();

#endif
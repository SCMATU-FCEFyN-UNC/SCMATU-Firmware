#ifndef RESONANCE_SWEEP_H
#define RESONANCE_SWEEP_H

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "modbus_imp.h"
#include "AD9833.h"

typedef struct {
    uint32_t freq;
    int16_t phase_ns;
    uint16_t current_adc;
} sweep_result_t;

typedef enum {
    SWEEP_IDLE,
    SWEEP_SET_FREQ,
    SWEEP_TRIGGER_PHASE_MEASUREMENT,
    SWEEP_WAIT_PHASE,
    SWEEP_MEASURE_CURRENT,
    SWEEP_NEXT_FREQ,
    SWEEP_DONE
} sweep_state_t;

void resonance_state_machine(mod_bus_registers* modbus_data);
void trigger_phase_measurement(bool is_internal, mod_bus_registers* modbus_data);
void handle_measurement_completion(mod_bus_registers* modbus_data);

//extern bool resonance_auto_detection_running;

#endif

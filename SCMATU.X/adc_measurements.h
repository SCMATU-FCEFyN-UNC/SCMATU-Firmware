#ifndef ADC_MEASUREMENTS_H
#define ADC_MEASUREMENTS_H

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "mcc_generated_files/system/system.h"
#include "modbus_imp.h"
#include "nanomodbus.h"

typedef enum
{
    ADC_MEAS_OK = 0,
    ADC_MEAS_INVALID = 1
} adc_status_t;

// ADC channels for peak detectors
//extern adc_channel_t VRLCr_PEAK; // Voltage
//extern adc_channel_t Vr_PEAK;    // Current

void adc_measurement_handler(mod_bus_registers *modbus_data);
uint16_t get_ADC_measurement(adc_channel_t channel);
uint16_t get_ADC_average(adc_channel_t channel, uint8_t samples);

#endif
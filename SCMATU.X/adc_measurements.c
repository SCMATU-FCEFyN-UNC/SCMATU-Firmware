#include "adc_measurements.h"

// --- Define ADC peak channels ---
static adc_channel_t VRLCr_PEAK = ADC_CHANNEL_ANC5; // Voltage
static adc_channel_t Vr_PEAK = ADC_CHANNEL_ANC4;    // Current

// --- ADC utility functions ---
uint16_t get_ADC_measurement(adc_channel_t channel)
{
    adc_result_t ADC_result = 0;

    ADC_ChannelSelect(channel);
    ADC_ConversionStart();

    while (!ADC_IsConversionDone())
        ;

    ADC_result = ADC_ConversionResultGet();

    return (uint16_t)ADC_result;
}

uint16_t get_ADC_average(adc_channel_t channel, uint8_t samples)
{
    uint32_t sum = 0;
    if (samples == 0)
        return 0;

    for (uint8_t i = 0; i < samples; i++)
    {
        sum += get_ADC_measurement(channel);
        __delay_us(100); // small delay between samples
    }

    return (uint16_t)(sum / samples);
}

// --- High-level measurement handler ---
void adc_measurement_handler(mod_bus_registers *modbus_data)
{
    // Mark as in progress
    modbus_data->server_input_register.curr_adc_measurement_ready = 0;

    // Determine sample count (bounded)
    uint8_t samples = modbus_data->server_holding_register.adc_samples_amount;
    if (samples == 0 || samples > MAX_ADC_SAMPLES)
        samples = 1;

    // Perform averaged measurements
    uint16_t ADC_peak_voltage = get_ADC_average(VRLCr_PEAK, samples);
    uint16_t ADC_peak_current = get_ADC_average(Vr_PEAK, samples);

    // Store results in Modbus input registers
    modbus_data->server_input_register.ADC_peak_voltage = ADC_peak_voltage;
    modbus_data->server_input_register.ADC_peak_current = ADC_peak_current;

    // Mark measurement ready
    modbus_data->server_input_register.curr_adc_measurement_ready = 1;
}
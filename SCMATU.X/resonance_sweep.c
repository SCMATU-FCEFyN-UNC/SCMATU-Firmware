#include "mcc_generated_files/system/system.h"
#include "resonance_sweep.h"
#include "nanomodbus.h"

extern uint8_t sample_index;
extern bool sampling_active;
extern bool measurement_ready;
extern uint32_t desired_frequency;
extern bool resonance_auto_detection_running;

// ---------------- Local sweep variables ----------------
static sweep_state_t sweep_state = SWEEP_IDLE;

static uint32_t sweep_start = 60000;
static uint32_t sweep_end = 70000;
static uint32_t sweep_step = 100;
static uint32_t sweep_freq = 0;

static uint16_t max_current_adc = 0;
static int16_t last_phase_ns = 0;
static int16_t abs_phase = 0;
static int16_t abs_best = 0;

static bool measurement_initiated = false;
static bool internal_request = false;

static sweep_result_t best_phase = {0, 32767, 0};
static sweep_result_t best_current = {0, 0, 0};
static sweep_result_t best_combined = {0, 32767, 0};

// ---------------- Phase measurement trigger ----------------
void trigger_phase_measurement(bool is_internal, mod_bus_registers* modbus_data)
{
    internal_request = is_internal;
    measurement_initiated = true;
    modbus_data->server_input_register.phase_ready = 0;
    modbus_data->server_input_register.internal_measurement_ready = 0;
    sample_index = 0;
    sampling_active = true;
    measurement_ready = false;

    PIR6bits.CCP1IF = 0;
    PIR6bits.CCP2IF = 0;
    CCP1CONbits.EN = 1;
    PIE6bits.CCP1IE = 1;
    PIE6bits.CCP2IE = 0;
}

// ---------------- Phase measurement completion handler ----------------
void handle_measurement_completion(mod_bus_registers* modbus_data)
{
    if (modbus_data->server_input_register.phase_ready == 1 && measurement_initiated)
    {
        last_phase_ns = modbus_data->server_input_register.phase_difference;

        if (internal_request)
            modbus_data->server_input_register.internal_measurement_ready = 1;

        measurement_initiated = false;
        internal_request = false;
    }
}

// ---------------- Resonance Sweep State Machine ----------------
void resonance_state_machine(mod_bus_registers* modbus_data)
{
    switch (sweep_state)
    {
    case SWEEP_IDLE:
        if (resonance_auto_detection_running)
        {
            // desired_frequency = (((uint32_t)modbus_data.server_holding_register.frequency_hi << 16) | modbus_data.server_holding_register.frequency_lo);
            sweep_start = (uint32_t)(((uint32_t)modbus_data->server_holding_register.freq_range_start_hi << 16) | modbus_data->server_holding_register.freq_range_start_lo);
            sweep_end = (uint32_t)(((uint32_t)modbus_data->server_holding_register.freq_range_end_hi << 16) | modbus_data->server_holding_register.freq_range_end_lo);
            sweep_step = (uint32_t)modbus_data->server_holding_register.freq_step;

            sweep_freq = sweep_start;

            // Reset best values
            best_phase.freq = 0;
            best_phase.phase_ns = 32767; // initialize with large phase (worst)
            best_phase.current_adc = 0;

            best_current.freq = 0;
            best_current.phase_ns = 0;
            best_current.current_adc = 0;

            max_current_adc = 0;
            best_combined.phase_ns = 32767;

            sweep_state = SWEEP_SET_FREQ;
        }
        break;

    case SWEEP_SET_FREQ:
        // Apply new frequency to AD9833
        desired_frequency = sweep_freq;
        AD9833SetFrequency(AD9833_REG_FREQ0, sweep_freq);
        __delay_ms(5); // let analog chain settle

        // Start with phase measurement first
        sweep_state = SWEEP_TRIGGER_PHASE_MEASUREMENT;
        break;

    case SWEEP_TRIGGER_PHASE_MEASUREMENT:
        // Trigger a new phase measurement (internal)
        if (!measurement_initiated)
        {
            trigger_phase_measurement(true, modbus_data);
            sweep_state = SWEEP_WAIT_PHASE;
        }
        break;

    case SWEEP_WAIT_PHASE:
        // Wait until the internal measurement is ready
        if (modbus_data->server_input_register.internal_measurement_ready)
        {
            // Phase value is now in modbus_data.server_input_register.phase_difference
            last_phase_ns = modbus_data->server_input_register.phase_difference;

            //* For Debugginh, 50kHz -> 0ns phase, 49900Hz -> 20ns phase, 49800Hz -> 40ns phase
            /*if(sweep_freq == 49800){modbus_data->server_input_register.test_1 = (uint16_t)last_phase_ns;}
            if(sweep_freq == 49900){modbus_data->server_input_register.test_2 = (uint16_t)last_phase_ns;}
            if(sweep_freq == 50000){modbus_data->server_input_register.test_3 = (uint16_t)last_phase_ns;}*/
            
            // Now trigger ADC current measurement
            modbus_data->server_input_register.curr_adc_measurement_ready = 0;
            nmbs_bitfield_write(modbus_data->server_coils.coils, 2, 1); // coil 2 ? measure power (current only)
            sweep_state = SWEEP_MEASURE_CURRENT;
        }
        break;

    case SWEEP_MEASURE_CURRENT:
        // Wait for ADC average measurement to complete
        if (modbus_data->server_input_register.curr_adc_measurement_ready == 1)
        {
            uint16_t current_adc = modbus_data->server_input_register.ADC_peak_current;

            // --- Evaluate best phase (closest to 0) ---
            abs_phase = (last_phase_ns >= 0) ? last_phase_ns : -last_phase_ns;
            abs_best = (best_phase.phase_ns >= 0) ? best_phase.phase_ns : -best_phase.phase_ns;

            if (abs_phase < abs_best)
            {
                best_phase.freq = sweep_freq;
                best_phase.phase_ns = last_phase_ns;
                best_phase.current_adc = current_adc;
            }

            // --- Evaluate best current (highest) ---
            if (current_adc > best_current.current_adc)
            {
                best_current.freq = sweep_freq;
                best_current.phase_ns = last_phase_ns;
                best_current.current_adc = current_adc;
            }

            // ? --- Evaluate overall "resonance" frequency ---
            // Priority 1: higher current
            // Priority 2: lower absolute phase (if current equal)
            if ((current_adc > max_current_adc) ||
                ((current_adc == max_current_adc) && (abs_phase < abs_best)))
            {
                max_current_adc = current_adc;
                best_combined.freq = sweep_freq;
                best_combined.phase_ns = last_phase_ns;
                best_combined.current_adc = current_adc;
            }

            sweep_state = SWEEP_NEXT_FREQ;
        }
        break;

    case SWEEP_NEXT_FREQ:
        sweep_freq += sweep_step;

        if (sweep_freq > sweep_end)
        {
            sweep_state = SWEEP_DONE;
        }
        else
        {
            sweep_state = SWEEP_SET_FREQ;
        }
        break;

    case SWEEP_DONE:
        // ---------------- Write "best phase" results ----------------
        modbus_data->server_input_register.best_freq_phase_hi = (uint16_t)(best_phase.freq >> 16);
        modbus_data->server_input_register.best_freq_phase_lo = (uint16_t)(best_phase.freq & 0xFFFF);
        modbus_data->server_input_register.best_freq_phase_phase = (uint16_t)best_phase.phase_ns;
        modbus_data->server_input_register.best_freq_phase_curr = (uint16_t)best_phase.current_adc;

        // ---------------- Write "best current" results ----------------
        modbus_data->server_input_register.best_freq_curr_hi = (uint16_t)(best_current.freq >> 16);
        modbus_data->server_input_register.best_freq_curr_lo = (uint16_t)(best_current.freq & 0xFFFF);
        modbus_data->server_input_register.best_freq_curr_phase = (uint16_t)best_current.phase_ns;
        modbus_data->server_input_register.best_freq_curr_curr = (uint16_t)best_current.current_adc;

        // ? ---------------- Write "overall resonance" results ----------------
        modbus_data->server_input_register.res_freq_hi = (uint16_t)(best_combined.freq >> 16);
        modbus_data->server_input_register.res_freq_lo = (uint16_t)(best_combined.freq & 0xFFFF);
        modbus_data->server_input_register.res_freq_phase = (uint16_t)best_combined.phase_ns;
        modbus_data->server_input_register.res_freq_curr = (uint16_t)best_combined.current_adc;

        // Indicate sweep completed
        modbus_data->server_input_register.res_freq_status = 1;
        resonance_auto_detection_running = false;
        AD9833SetFrequency(AD9833_REG_FREQ0, best_combined.freq);
        sweep_state = SWEEP_IDLE;
        break;
    }
}

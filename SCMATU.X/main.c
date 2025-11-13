#include "mcc_generated_files/system/system.h"
#include <xc.h>
#include <stdio.h>  // Include the standard I/O library
#include <string.h> // Include the string library
#include <stdbool.h> // So the bool variable type can be used
#include "AD9833.h" // Library to control AD9833 signal generator
#include "nanomodbus.h" // Library to control AD9833 signal generator
#include "modbus_imp.h" // Library to control AD9833 signal generator
#include "nvm_config.h" // Library to control AD9833 signal generator
#include "robust_measurement.h" // Robust measurement library


// Actuator Control Variables
uint32_t desired_frequency = 140000;
uint32_t resonance_frequency = 131000;

// Modbus Variables
mod_bus_registers modbus_data;      // Coils, Holding Registers, Input Registers
holding_register prev_holding_regs; // Store the Holdding Registers  previous values upon modbus commands
nmbs_t nmbs;                        // Main Server Structure
nmbs_platform_conf platform_conf;   // Platform Specific Config
nmbs_callbacks callbacks;           // Structure containing callback functions to be executed upon Modbus commands

// CPP Variables
uint16_t ccp1_value = 0;
uint16_t ccp2_value = 0;
uint16_t phase_ticks;
bool measurement_ready = false;

// Interrupt Service Routines for phase measuring
void CCP1_Interrupt_Handler(uint16_t value);
void CCP2_Interrupt_Handler(uint16_t value);

// Robust_measurement values
uint16_t raw_phase_samples[MAX_SAMPLES];
uint16_t temp_samples[MAX_SAMPLES];
uint16_t median;
uint8_t requested_samples = 0;
uint8_t sample_index = 0;
bool sampling_active = false;

// Robust Measurement Functions
void get_phase_samples();

// ADC (Peak-Detectors - Voltage and Current Measurements) Variables & Functions
uint16_t ADC_peak_voltage;
uint16_t ADC_peak_current;
adc_channel_t VRLCr_PEAK = ADC_CHANNEL_ANC5;
adc_channel_t Vr_PEAK = ADC_CHANNEL_ANC4; 

// ADC Functions
uint16_t get_ADC_measurement(adc_channel_t channel);
uint16_t get_ADC_average(adc_channel_t channel, uint8_t samples);

bool performing_internal_measurement = false;
bool internal_measurement_ready = false;

// ---------------- Resonance Sweep State Machine ----------------


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
    SWEEP_EVALUATE_PHASE,
    SWEEP_MEASURE_CURRENT,
    SWEEP_NEXT_FREQ,
    SWEEP_DONE
} sweep_state_t;

static sweep_state_t sweep_state = SWEEP_IDLE;

uint32_t sweep_start = 60000;
uint32_t sweep_end = 70000;
uint32_t sweep_step = 100;
uint32_t sweep_freq = 0;

sweep_result_t best_phase = {0, 32767, 0};
sweep_result_t best_current = {0, 0, 0};
sweep_result_t best_combined = {0, 32767, 0};

uint16_t max_current_adc = 0;
int16_t last_phase_ns = 0;

int16_t abs_phase = 0;
int16_t abs_best = 0;

bool resonance_auto_detection_running = false;
// ? unified measurement state flags
bool measurement_initiated = false;
bool internal_request = false;

void resonance_state_machine();

void trigger_phase_measurement(bool is_internal);
void handle_measurement_completion(void);

int main(void)
{
    SYSTEM_Initialize();

    // If using interrupts in PIC18 High/Low Priority Mode you need to enable the Global High and Low Interrupts 
    // If using interrupts in PIC Mid-Range Compatibility Mode you need to enable the Global and Peripheral Interrupts 
    // Use the following macros to: 

    // Enable the Global Interrupts 
    INTERRUPT_GlobalInterruptEnable(); 

    // Disable the Global Interrupts 
    //INTERRUPT_GlobalInterruptDisable(); 

    // Enable the Peripheral Interrupts 
    INTERRUPT_PeripheralInterruptEnable(); 

    // Disable the Peripheral Interrupts 
    //INTERRUPT_PeripheralInterruptDisable(); 
    
    // ------------------------------- CCP Initialization -------------------------------
    // Initially, CCP1 and CCP2 are disabled (both peripheral and interrupt)
    PIE6bits.CCP1IE = 0;
    PIE6bits.CCP2IE = 0;
    PIR6bits.CCP1IF = 0;
    PIR6bits.CCP2IF = 0;
    CCP1CONbits.EN = 0; 
    CCP2CONbits.EN = 0; //Disables the CCP2 module (puts the peripheral in reset
    TEST_SetLow();
    // Link each ISR to custom handlers
    CCP1_SetCallBack(&CCP1_Interrupt_Handler);
    CCP2_SetCallBack(&CCP2_Interrupt_Handler);
    // ------------------------------- /CCP Initialization ------------------------------
    
    __delay_ms(50); // This delay is required for modbus to work
    
    // --------------------------------------------- Modbus Initialization ------------------------------------------------------
    // Load default values into each modbus register
    default_values_register(&modbus_data);
    // Load default values into each modbus holding register (shadow copy)
    set_holding_regs_to_default(&prev_holding_regs);
    
    // Link Callback functions for modbus commands
    callbacks.read_holding_registers    = handler_read_holding_registers;
    callbacks.read_input_registers      = handler_read_input_registers; 
    callbacks.write_single_coil         = handle_write_single_coil;
    callbacks.write_single_register     = handle_write_single_register;
    
    // Platform config for Modbus RTU
    platform_conf.transport = NMBS_TRANSPORT_RTU;  // Modbus on RTU protocol
    platform_conf.read = read_serial;              // Link our modbus_imp read_serial function to the read from UART operation
    platform_conf.write = write_serial;            // Link our modbus_imp write_serial function to the write to UART operation
    platform_conf.arg = &(modbus_data);            // Pass out modbus registers instance as a parameter
    
    // Create Modbus Server Instance. Initialize nanomodbus stack as an RTU slave with the defined address in modbus_Data
    nmbs_error err = nmbs_server_create(
        &nmbs,                                              // Main Server Structure
        modbus_data.server_holding_register.addr_slave,     // Slave Address
        &platform_conf,                                     // Platform specific config
        &callbacks                                          // Callback functions to be executed upon modbus commands
    );
    
    // Check for critical errors
    if (err != NMBS_ERROR_NONE) 
    {
        // check_error_modbus(err)  // Handle error
        //while(1){}                  // Halt if unable to create modbus server 
    } 
    // --------------------------------------------- /Modbus Initialization -----------------------------------------------------
    
    // ----------------------------- AD9833 Initialization ------------------------------
    desired_frequency = (((uint32_t)modbus_data.server_holding_register.frequency_hi << 16) | modbus_data.server_holding_register.frequency_lo);
    AD9833Reset();
    AD9833SetRegisterValue(AD9833_OUT_SINUS);
    AD9833SetFrequency(AD9833_REG_FREQ0, desired_frequency);
    AD9833SetRegisterValue(AD9833_REG_CMD); // Clears RESET, enabling output
    // ----------------------------- /AD9833 Initialization -----------------------------
    
    ADC_Enable();
    
    modbus_data.server_input_register.internal_measurement_ready = 0;
    modbus_data.server_input_register.best_freq_curr_hi = 0;
    modbus_data.server_input_register.best_freq_curr_lo = 0;
    modbus_data.server_input_register.best_freq_phase_hi = 0;
    modbus_data.server_input_register.best_freq_phase_lo = 0;
    
    modbus_data.server_input_register.best_freq_phase_phase = 0;
    modbus_data.server_input_register.best_freq_curr_phase = 0;
    
    while(1)
    {
        err = nmbs_server_poll(&nmbs);
        if (err != NMBS_ERROR_NONE)  // Basic runtime error handling
        {
            // check_error_modbus(err)  // Handle error
        }
        else
        {
            // Handle changes in Holding Registers
            holding_register_change_handler(&modbus_data, &prev_holding_regs, &nmbs); 
            // Handle changes in coil registers
            if(nmbs_bitfield_read(modbus_data.server_coils.coils, 0))
            {
                //* 0            | Enable/Disable transducer
            }
            if (nmbs_bitfield_read(modbus_data.server_coils.coils, 1) || 
                nmbs_bitfield_read(modbus_data.server_coils.coils, 2)) //* 1 | Measure All (Phase and Power) | Measure Power
            {
                // ? Indicate measurement is in progress
                modbus_data.server_input_register.curr_adc_measurement_ready = 0;

                // Get number of samples (limited by MAX_ADC_SAMPLES)
                uint8_t samples = modbus_data.server_holding_register.adc_samples_amount;
                if (samples == 0 || samples > MAX_ADC_SAMPLES)
                    samples = 1; // fallback to single measurement

                // Perform averaged measurement
                ADC_peak_voltage = get_ADC_average(VRLCr_PEAK, samples);
                ADC_peak_current = get_ADC_average(Vr_PEAK, samples);

                // Store averaged results
                modbus_data.server_input_register.ADC_peak_voltage = ADC_peak_voltage;
                modbus_data.server_input_register.ADC_peak_current = ADC_peak_current;

                // ? Indicate measurement is complete
                modbus_data.server_input_register.curr_adc_measurement_ready = 1;

                // Reset coils so it can be triggered again later
                nmbs_bitfield_write(modbus_data.server_coils.coils, 1, 0);
                nmbs_bitfield_write(modbus_data.server_coils.coils, 2, 0);
            }
            if ((nmbs_bitfield_read(modbus_data.server_coils.coils, 3)) || 
                (nmbs_bitfield_read(modbus_data.server_coils.coils, 6)))
            {
                bool internal = nmbs_bitfield_read(modbus_data.server_coils.coils, 6);
                trigger_phase_measurement(internal);
                nmbs_bitfield_write(modbus_data.server_coils.coils, 3, 0);
                nmbs_bitfield_write(modbus_data.server_coils.coils, 6, 0);
            }
            if(nmbs_bitfield_read(modbus_data.server_coils.coils, 4)) // Apply changes in frequency
            {
                nmbs_bitfield_write(modbus_data.server_coils.coils, 1, 0);
                nmbs_bitfield_write(modbus_data.server_coils.coils, 4, 0);
                desired_frequency = (((uint32_t)modbus_data.server_holding_register.frequency_hi << 16) | modbus_data.server_holding_register.frequency_lo);
                AD9833SetFrequency(AD9833_REG_FREQ0, desired_frequency);
            }
            if(nmbs_bitfield_read(modbus_data.server_coils.coils, 5))
            {
                if(!resonance_auto_detection_running)
                {
                    modbus_data.server_input_register.res_freq_status = 3;
                    resonance_auto_detection_running = true; 
                }
                nmbs_bitfield_write(modbus_data.server_coils.coils, 1, 0);
                nmbs_bitfield_write(modbus_data.server_coils.coils, 5, 0);
            }
        } 
        
        get_phase_samples();
        handle_measurement_completion();
        resonance_state_machine();
    }    
}

void CCP1_Interrupt_Handler(uint16_t value)
{
    // TEST_SetHigh(); // This was used to visualized the measured period between CCP1_Interrupt and CCP2_interrupt in the oscilloscope.
    ccp1_value = value;
    CCP1CONbits.EN = 0;     // Disable CCP1 peripheral so that no more CCP1 interrupts are queued
    CCP2CONbits.EN = 1;     // Enable CCP2 peripheral
    PIR6bits.CCP2IF = 0;    // Clear any pending CCP2 interrupts before enabling so that the ISR is not immediately triggered due to previous interrupts
    PIE6bits.CCP2IE = 1;    // Enable CCP2 Interrupt
    PIE6bits.CCP1IE = 0;    // Disable CCP1 Interrupt
}

void CCP2_Interrupt_Handler(uint16_t value) {
    // TEST_SetLow(); // This was used to visualized the measured period between CCP1_Interrupt and CCP2_interrupt in the oscilloscope.
    ccp2_value = value;
    phase_ticks = ccp2_value - ccp1_value;
    CCP2CONbits.EN = 0;     // Disable CCP2 peripheral
    PIE6bits.CCP2IE = 0;    // Disable CCP2 Interrupt
    PIR6bits.CCP1IF = 0;    // Clear CCP1 Interrupt Flag (prevents immediate execution of CCP1 ISR due to previous interrupts)
    PIR6bits.CCP2IF = 0; 
    // These two following lines need to be uncommented in order to visualize measured period via TEST pin.
    // CCP1CONbits.EN = 1;  // Re-enable CCP1 peripheral for continuous phase measuring
    // PIE6bits.CCP1IE = 1; // Re-enable CCP1 Interrupt for continuous phase measuring. 
    measurement_ready = true;   // Once the measurement is ready, the values can be stored into modbus registers.
}

uint16_t get_ADC_measurement(adc_channel_t channel)
{
    adc_result_t ADC_result = 0;
    
    // Select the ADC channel
    ADC_ChannelSelect(channel);

    // Start the conversion
    ADC_ConversionStart();

    // Wait for the conversion to complete
    while (!ADC_IsConversionDone());

    // Get the conversion result
    ADC_result = ADC_ConversionResultGet();
    
    return (uint16_t)ADC_result;
}

uint16_t get_ADC_average(adc_channel_t channel, uint8_t samples)
{
    uint32_t sum = 0;

    for (uint8_t i = 0; i < samples; i++)
    {
        sum += get_ADC_measurement(channel);
        __delay_us(100); // small delay between samples to reduce correlation noise
    }

    return (uint16_t)(sum / samples);
}

void get_phase_samples()
{
    // --- 2. Handle when a measurement just finished (set by CCP2 ISR) ---
    requested_samples = modbus_data.server_holding_register.samples_amount;
        
    if (sampling_active && measurement_ready)
    {
        measurement_ready = false;

        // Store the measured value
        raw_phase_samples[sample_index++] = phase_ticks;

        // Reset intermediate variables
        phase_ticks = 0;
        ccp1_value = 0;
        ccp2_value = 0;

        if (sample_index < requested_samples)
        {
            // Start the next measurement
            PIR6bits.CCP1IF = 0;
            PIR6bits.CCP2IF = 0;
            CCP1CONbits.EN = 1;
            PIE6bits.CCP1IE = 1;
            PIE6bits.CCP2IE = 0;
        }
        else
        {
            sampling_active = false;

            // --- Compute median & robust average ---
            uint16_t median = calculate_median(raw_phase_samples, temp_samples, requested_samples);
            const uint16_t tolerance_ticks = 2;   // ?200 ns at 125 ns/tick
            uint16_t robust_avg_ticks = robust_average(raw_phase_samples, median,
                                                       requested_samples, tolerance_ticks);

            // --- Convert to nanoseconds ---
            float phase_ns = calculate_phase_ns(robust_avg_ticks,
                                                (float)desired_frequency,
                                                (float)TICKS_NS);

            // --- Store results ---
            modbus_data.server_input_register.phase_difference = (int16_t)phase_ns;
            modbus_data.server_input_register.phase_ready = 1;   // ready for host
        }
    }
}

// ? unified trigger for both internal/external
void trigger_phase_measurement(bool is_internal)
{
    internal_request = is_internal;
    measurement_initiated = true;
    modbus_data.server_input_register.phase_ready = 0;
    modbus_data.server_input_register.internal_measurement_ready = 0;
    sample_index = 0;
    sampling_active = true;
    measurement_ready = false;

    PIR6bits.CCP1IF = 0;
    PIR6bits.CCP2IF = 0;
    CCP1CONbits.EN = 1;
    PIE6bits.CCP1IE = 1;
    PIE6bits.CCP2IE = 0;
}

// ? unified completion handler
void handle_measurement_completion(void)
{
    if (modbus_data.server_input_register.phase_ready == 1 && measurement_initiated)
    {
        last_phase_ns = modbus_data.server_input_register.phase_difference;

        if (internal_request)
        {
            modbus_data.server_input_register.internal_measurement_ready = 1;
        }

        measurement_initiated = false;
        internal_request = false;
    }
}

void resonance_state_machine()
{
    switch (sweep_state)
    {
        case SWEEP_IDLE:
            if (resonance_auto_detection_running)
            {
                // desired_frequency = (((uint32_t)modbus_data.server_holding_register.frequency_hi << 16) | modbus_data.server_holding_register.frequency_lo);
                sweep_start = (uint32_t)(((uint32_t)modbus_data.server_holding_register.freq_range_start_hi << 16) | modbus_data.server_holding_register.freq_range_start_lo);
                sweep_end   = (uint32_t)(((uint32_t)modbus_data.server_holding_register.freq_range_end_hi   << 16) | modbus_data.server_holding_register.freq_range_end_lo);
                sweep_step = (uint32_t)modbus_data.server_holding_register.freq_step;
                
                sweep_freq = sweep_start;

                // Reset best values
                best_phase.freq = 0;
                best_phase.phase_ns = 32767;  // initialize with large phase (worst)
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
            __delay_ms(20); // let analog chain settle

            // Start with phase measurement first
            sweep_state = SWEEP_TRIGGER_PHASE_MEASUREMENT;
            break;

        case SWEEP_TRIGGER_PHASE_MEASUREMENT:
            // Trigger a new phase measurement (internal)
            if (!measurement_initiated)
            {
                trigger_phase_measurement(true);
                sweep_state = SWEEP_WAIT_PHASE;
            }
            break;

        case SWEEP_WAIT_PHASE:
            // Wait until the internal measurement is ready
            if (modbus_data.server_input_register.internal_measurement_ready)
            {
                // Phase value is now in modbus_data.server_input_register.phase_difference
                last_phase_ns = modbus_data.server_input_register.phase_difference;

                // Now trigger ADC current measurement
                modbus_data.server_input_register.curr_adc_measurement_ready = 0;
                nmbs_bitfield_write(modbus_data.server_coils.coils, 2, 1);  // coil 2 ? measure power (current only)
                sweep_state = SWEEP_MEASURE_CURRENT;
            }
            break;

        case SWEEP_MEASURE_CURRENT:
            // Wait for ADC average measurement to complete
            if (modbus_data.server_input_register.curr_adc_measurement_ready == 1)
            {
                uint16_t current_adc = modbus_data.server_input_register.ADC_peak_current;

                // --- Evaluate best phase (closest to 0) ---
                abs_phase = (last_phase_ns >= 0) ? last_phase_ns : -last_phase_ns;
                abs_best  = (best_phase.phase_ns >= 0) ? best_phase.phase_ns : -best_phase.phase_ns;

                if (abs_phase < abs_best)
                {
                    best_phase.freq        = sweep_freq;
                    best_phase.phase_ns    = last_phase_ns;
                    best_phase.current_adc = current_adc;
                }

                // --- Evaluate best current (highest) ---
                if (current_adc > best_current.current_adc)
                {
                    best_current.freq        = sweep_freq;
                    best_current.phase_ns    = last_phase_ns;
                    best_current.current_adc = current_adc;
                }

                // ? --- Evaluate overall "resonance" frequency ---
                // Priority 1: higher current
                // Priority 2: lower absolute phase (if current equal)
                if ((current_adc > max_current_adc) ||
                    ((current_adc == max_current_adc) && (abs_phase < abs_best)))
                {
                    max_current_adc = current_adc;
                    best_combined.freq        = sweep_freq;
                    best_combined.phase_ns    = last_phase_ns;
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
            modbus_data.server_input_register.best_freq_phase_hi   = (uint16_t)(best_phase.freq >> 16);
            modbus_data.server_input_register.best_freq_phase_lo   = (uint16_t)(best_phase.freq & 0xFFFF);
            modbus_data.server_input_register.best_freq_phase_phase = (uint16_t)best_phase.phase_ns;
            modbus_data.server_input_register.best_freq_phase_curr  = (uint16_t)best_phase.current_adc;

            // ---------------- Write "best current" results ----------------
            modbus_data.server_input_register.best_freq_curr_hi   = (uint16_t)(best_current.freq >> 16);
            modbus_data.server_input_register.best_freq_curr_lo   = (uint16_t)(best_current.freq & 0xFFFF);
            modbus_data.server_input_register.best_freq_curr_phase = (uint16_t)best_current.phase_ns;
            modbus_data.server_input_register.best_freq_curr_curr  = (uint16_t)best_current.current_adc;

            // ? ---------------- Write "overall resonance" results ----------------
            modbus_data.server_input_register.res_freq_hi   = (uint16_t)(best_combined.freq >> 16);
            modbus_data.server_input_register.res_freq_lo   = (uint16_t)(best_combined.freq & 0xFFFF);
            modbus_data.server_input_register.res_freq_phase = (uint16_t)best_combined.phase_ns;
            modbus_data.server_input_register.res_freq_curr  = (uint16_t)best_combined.current_adc;

            // Indicate sweep completed
            modbus_data.server_input_register.res_freq_status = 1;  
            resonance_auto_detection_running = false;
            AD9833SetFrequency(AD9833_REG_FREQ0, best_combined.freq);
            sweep_state = SWEEP_IDLE;
            break;
    }
}
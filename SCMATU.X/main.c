#include "mcc_generated_files/system/system.h"
#include <xc.h>
#include <stdio.h>  // Include the standard I/O library
#include <string.h> // Include the string library
#include <stdbool.h> // So the bool variable type can be used
#include "AD9833.h" // Library to control AD9833 signal generator
#include "nanomodbus.h" // Library to control AD9833 signal generator
#include "modbus_imp.h" // Library to control AD9833 signal generator
#include "nvm_config.h" // Library to control AD9833 signal generator

// Actuator Control Variables
uint32_t desired_frequency = 140000;
uint32_t resonance_frequency = 131000;

// Modbus Variables
mod_bus_registers modbus_data;      // Coils, Holding Registers, Input Registers
holding_register prev_holding_regs; // Store the Holdding Registers´ previous values upon modbus commands
nmbs_t nmbs;                        // Main Server Structure
nmbs_platform_conf platform_conf;   // Platform Specific Config
nmbs_callbacks callbacks;           // Structure containing callback functions to be executed upon Modbus commands

// CPP Variables
uint16_t ccp1_value = 0;
uint16_t ccp2_value = 0;
uint16_t phase_ticks;
bool measurement_ready = false;

// Interrupt Service Routines
void CCP1_Interrupt_Handler(uint16_t value);
void CCP2_Interrupt_Handler(uint16_t value);

// ADC (Peak-Detectors - Voltage and Current Measurements) Variables & Functions
uint16_t ADC_peak_voltage;
uint16_t ADC_peak_current;
adc_channel_t VRLCr_PEAK = ADC_CHANNEL_ANC5;
adc_channel_t Vr_PEAK = ADC_CHANNEL_ANC4; 

uint16_t get_ADC_measurement(adc_channel_t channel);

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
 
    // Load default frequency into the correct modbus registers so it can be read externally.
    //modbus_data.server_holding_register.frequency_hi = (desired_frequency >> 16) & 0xFFFF;
    //modbus_data.server_holding_register.frequency_lo = desired_frequency & 0xFFFF;
    
    // ----------------------------- AD9833 Initialization ------------------------------
    desired_frequency = (((uint32_t)modbus_data.server_holding_register.frequency_hi << 16) | modbus_data.server_holding_register.frequency_lo);
    AD9833Reset();
    AD9833SetRegisterValue(AD9833_OUT_SINUS);
    AD9833SetFrequency(AD9833_REG_FREQ0, desired_frequency);
    AD9833SetRegisterValue(AD9833_REG_CMD); // Clears RESET, enabling output
    // ----------------------------- /AD9833 Initialization -----------------------------
    
    ADC_Enable();
    
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
            if(nmbs_bitfield_read(modbus_data.server_coils.coils, 1) || nmbs_bitfield_read(modbus_data.server_coils.coils, 2)) //* 1  | Measure All (Phase and Power) | Measure Power
            {
                // Perform measurement
                ADC_peak_voltage = get_ADC_measurement(VRLCr_PEAK);
                ADC_peak_current = get_ADC_measurement(Vr_PEAK);
                // Store measurement
                modbus_data.server_input_register.ADC_peak_voltage = ADC_peak_voltage;
                modbus_data.server_input_register.ADC_peak_current = ADC_peak_current;
                nmbs_bitfield_write(modbus_data.server_coils.coils, 1, 0);
                nmbs_bitfield_write(modbus_data.server_coils.coils, 2, 0);
            }
            if (nmbs_bitfield_read(modbus_data.server_coils.coils, 1) ||
                nmbs_bitfield_read(modbus_data.server_coils.coils, 3))
            {            
                // clear CCP flags
                PIR6bits.CCP1IF = 0;
                PIR6bits.CCP2IF = 0;
                
                // enable CCP1 in order to start measurement
                CCP1CONbits.EN = 1 ;  
                PIE6bits.CCP1IE = 1;
                PIE6bits.CCP2IE = 0;
                // clear coil bit so it can be triggered again later
                nmbs_bitfield_write(modbus_data.server_coils.coils, 1, 0);
                nmbs_bitfield_write(modbus_data.server_coils.coils, 3, 0);
            }
            if(nmbs_bitfield_read(modbus_data.server_coils.coils, 4)) // Apply changes in frequency
            {
                nmbs_bitfield_write(modbus_data.server_coils.coils, 1, 0);
                nmbs_bitfield_write(modbus_data.server_coils.coils, 4, 0);
                desired_frequency = (((uint32_t)modbus_data.server_holding_register.frequency_hi << 16) | modbus_data.server_holding_register.frequency_lo);
                AD9833SetFrequency(AD9833_REG_FREQ0, desired_frequency);
            }
            if(nmbs_bitfield_read(modbus_data.server_coils.coils, 5)) // Auto-determine resonance frequency (dummy)
            {
                resonance_frequency = 131000;
                
                modbus_data.server_input_register.res_freq_hi = (uint16_t)(resonance_frequency >> 16);
                modbus_data.server_input_register.res_freq_lo = (uint16_t)(resonance_frequency & 0xFFFF);
                modbus_data.server_input_register.res_freq_status = 1;
                nmbs_bitfield_write(modbus_data.server_coils.coils, 1, 0);
                nmbs_bitfield_write(modbus_data.server_coils.coils, 5, 0);
            }
        } 
        
        if(measurement_ready)
        {
            measurement_ready = false;                                          // Clear these flag to avoid re-execution
            modbus_data.server_input_register.phase_difference = phase_ticks;   // Store result into corresponding modbus register
            // Reset values before next measurement
            phase_ticks = 0;
            ccp1_value = 0;
            ccp2_value = 0;
        }
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
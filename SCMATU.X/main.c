#include "mcc_generated_files/system/system.h"
#include <xc.h>
#include <stdio.h>  // Include the standard I/O library
#include <string.h> // Include the string library
#include <stdbool.h> // So the bool variable type can be used
#include "AD9833.h" // Library to control AD9833 signal generator
#include "nanomodbus.h" // Library to control AD9833 signal generator
#include "modbus_imp.h" // Library to control AD9833 signal generator
#include "nvm_config.h" // Library to control AD9833 signal generator

#define PHASE_SAMPLES 15 // amount of samples to be taken to obtain the average  

// Actuator Control Variables
uint32_t desiredFrequency = 150;

// Modbus Variables
mod_bus_registers modbus_data;      // Coils, Holding Registers, Input Registers
holding_register prev_holding_regs; // Store the Holdding Registers´ previous values upon modbus commands
nmbs_t nmbs;                        // Main Server Structure
nmbs_platform_conf platform_conf;   // Platform Specific Config
nmbs_callbacks callbacks;           // Structure containing callback functions to be executed upon Modbus commands

// CCP variables
uint16_t CCP1_Captured_Values[PHASE_SAMPLES];
uint16_t CCP2_Captured_Values[PHASE_SAMPLES];
uint16_t CCP1_Difference = 0;
bool iCCP1 = false;
bool print_enable = false;

// Calculate phase 
uint16_t phase_measurements[PHASE_SAMPLES];
uint32_t  phase_measurement_accumulator = 0;
uint16_t phase_average = 0;
uint8_t measurements_iterator = 0;
uint8_t measuring_phase = 0; // currently measuring phase, some samples still need to be taken
uint8_t keep_measuring = 0;
bool phase_measurement_enable = false;
bool measurement_ready = false;
bool is_phase_ready = false;

uint16_t ccp1 = 0;;
uint16_t ccp2 = 0;
uint16_t diff;

uint8_t tmr0_count = 0;

// Interrupt Service Routines
void TMR0_Interrupt_Handler();
void CCP1_Interrupt_Handler(uint16_t value);
void CCP2_Interrupt_Handler(uint16_t value);

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
    
    PIE6bits.CCP1IE = 0;    // Initially disable CCP1 interrupt
    PIE6bits.CCP2IE = 0;    // Initially disable CCP2 interrupt
    TEST_SetLow();          // This pin will help us visualize the time between CCP1 and CCP2 interrupts that we are to measure
    
    // Link each ISR to custom handlers
    TMR0_PeriodMatchCallbackRegister(&TMR0_Interrupt_Handler);
    CCP1_SetCallBack(&CCP1_Interrupt_Handler);
    CCP2_SetCallBack(&CCP2_Interrupt_Handler);
    
    // AD9833 Variables 
    desiredFrequency = 140000;
    
    AD9833Reset();
    AD9833SetRegisterValue(AD9833_OUT_SINUS);
    AD9833SetFrequency(AD9833_REG_FREQ0, desiredFrequency);
    AD9833SetRegisterValue(AD9833_REG_CMD); // Clears RESET, enabling output
    
    __delay_ms(50);
    //PIE6bits.CCP1IE = 1; // Enable the CCP1 interrupt
    
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
                modbus_data.server_input_register.power_output = 50;            // Replace 50 with the actual calculated power output using two ADC channels
            }
            if(nmbs_bitfield_read(modbus_data.server_coils.coils, 1) || nmbs_bitfield_read(modbus_data.server_coils.coils, 3)) //* 1  | Measure All (Phase and Power) | Measure Phase
            {
                phase_measurement_enable = true;
                nmbs_bitfield_write(modbus_data.server_coils.coils, 3, 0);  // Clear after triggering
                // phase = meassure_phase();                                    // Replace with proper variable/function to handle phase measurement
                //modbus_data.server_input_register.phase_difference = phase;   // Replace with proper variable to handle phase measurement
                /*if(phase == 0)
                {
                    modbus_data.server_input_register.resonance_status = true;
                    current_frecuency = ((uint32_t)modbus_data.server_holding_register.frequency_hi << 16) | modbus_data.server_holding_register.frequency_lo;
                    modbus_data.server_input_register.resonance_freq = current_frecuency;
                }*/
            }
            if(nmbs_bitfield_read(modbus_data.server_coils.coils, 4)) //* 4  | Start frequency sweep - Auto-determine resonance frecuency
            {
                // resonance_freq = auto_detect_res_freq();                     // Replace with proper function to autp-detect resonance frequency
                // modbus_data.server_input_register.resonance_freq_hi = (resonance_freq >> 16) & 0xFFFF;
                // modbus_data.server_input_register.resonance_freq_lo = resonance_freq & 0xFFFF;
                // modbus_data.server_input_register.resonance_status = true;
            }
        } 
    
        if(phase_measurement_enable) // If the coil was set then triger phase measurement once
        {
            phase_measurement_enable = false;
            diff = 0;
            ccp1 = 0;
            ccp2 = 0;
            memset(CCP1_Captured_Values, 0, sizeof(CCP1_Captured_Values));
            memset(CCP2_Captured_Values, 0, sizeof(CCP2_Captured_Values));
            measurements_iterator = 0;
            measuring_phase = 1;
            keep_measuring = 1;
            tmr0_count = 0;
            iCCP1 = false;
            PIE6bits.CCP1IE = 1; // Enable the CCP1 interrupt, this will start phase measurement
        }
        if(measuring_phase == 1)
        {
           if(measurement_ready)
            {
                ccp1 = CCP1_Captured_Values[measurements_iterator];
                ccp2 = CCP2_Captured_Values[measurements_iterator];
                if (ccp2 >= ccp1) 
                {
                    diff = ccp2 - ccp1;
                    measurements_iterator++;
                    phase_measurement_accumulator += diff;
                }
                else
                {
                    // Overflow occurred: wraparound
                    diff = (0xFFFF - ccp1) + ccp2 + 1;

                    // Optional safety check: reject if too big to be real
                    if (diff < (0xFFFF / 2))  // or another threshold like 5000
                    {
                        phase_measurement_accumulator += diff;
                        measurements_iterator++;
                    }
                    // else: discard this measurement silently
                }
                measurement_ready = false;
            }
            if(measurements_iterator < PHASE_SAMPLES)
            {
                CCP1_Captured_Values[measurements_iterator] = 0;
                CCP2_Captured_Values[measurements_iterator] = 0;
                keep_measuring = 1;
                iCCP1 = false;
                PIE6bits.CCP1IE = 1; // Re-trigger phase measurement until 10 correct measurements have been taken
            }
           else
            {
                measuring_phase = 0; // Do not trigger any more measurements until the coil is set again
                keep_measuring = 0;
                
                phase_average = phase_measurement_accumulator / PHASE_SAMPLES;
                phase_measurement_accumulator = 0;
                is_phase_ready = true;
                measurements_iterator = 0;
            }   
        }
        if(is_phase_ready)
        {
            modbus_data.server_input_register.phase_difference = phase_average;
            memset(CCP1_Captured_Values, 0, sizeof(CCP1_Captured_Values));
            memset(CCP2_Captured_Values, 0, sizeof(CCP2_Captured_Values));
            phase_measurement_accumulator = 0;
            measurements_iterator = 0;
            is_phase_ready = false;
        }
       
    }    
}

void CCP1_Interrupt_Handler(uint16_t value) 
{
    if(iCCP1 == false)
    {
        TEST_SetHigh();
        CCP1_Captured_Values[measurements_iterator] = value;
    }
    iCCP1 = !iCCP1;
    PIE6bits.CCP1IE = 0;
    PIE6bits.CCP2IE = 1;  
 }

void CCP2_Interrupt_Handler(uint16_t value) {
    if(iCCP1 == true)
    {
        TEST_SetLow();
        CCP2_Captured_Values[measurements_iterator] = value;
    } 
    PIE6bits.CCP2IE = 0;
    PIE6bits.CCP1IE = keep_measuring;  // Both interrupts are disabled as one measurement is taken at the time
 }

void TMR0_Interrupt_Handler()
{
    if(keep_measuring == 1)
    {
        tmr0_count++;
        if(tmr0_count == 3)
        {
            keep_measuring = 0;
            measurement_ready = true;
            tmr0_count = 0;   
        }
    }   
}
#include "mcc_generated_files/system/system.h"
#include <xc.h>
#include <stdio.h>  // Include the standard I/O library
#include <string.h> // Include the string library
#include "AD9833.h" // Library to control AD9833 signal generator
#include "nanomodbus.h" // Library to control AD9833 signal generator
#include "modbus_imp.h" // Library to control AD9833 signal generator

// Actuator Control Variables
uint32_t desiredFrequency = 150;

// Modbus Variables
mod_bus_registers modbus_data;      // Coils, Holding Registers, Input Registers
holding_register prev_holding_regs; // Store the Holdding Registers´ previous values upon modbus commands
nmbs_t nmbs;                        // Main Server Structure
nmbs_platform_conf platform_conf;   // Platform Specific Config
nmbs_callbacks callbacks;           // Structure containing callback functions to be executed upon Modbus commands

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
    
    // AD9833 Variables 
    desiredFrequency = 140000;
    
    AD9833Reset();
    AD9833SetRegisterValue(AD9833_OUT_SINUS);
    AD9833SetFrequency(AD9833_REG_FREQ0, desiredFrequency);
    AD9833SetRegisterValue(AD9833_REG_CMD); // Clears RESET, enabling output
    
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
            holding_register_change_handler(&modbus_data, &prev_holding_regs); // &nmbs
            // Handle changes in coil registers
            if(nmbs_bitfield_read(modbus_data.server_coils.coils, 0))
            {
                //* 0            | Enable/Disable transducer
            }
            if(nmbs_bitfield_read(modbus_data.server_coils.coils, 1) || nmbs_bitfield_read(modbus_data.server_coils.coils, 2)) //* 1  | Measure All (Phase and Power) | Measure Power
            {
                modbus_data.server_input_register.power_output = 50;            // Repplace 50 with the actual calculated power output using two ADC channels
            }
            if(nmbs_bitfield_read(modbus_data.server_coils.coils, 1) || nmbs_bitfield_read(modbus_data.server_coils.coils, 3)) //* 1  | Measure All (Phase and Power) | Measure Phase
            {
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
    }    
}

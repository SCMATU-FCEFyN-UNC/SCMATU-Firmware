
#ifndef MODBUS_IMP_H
#define	MODBUS_IMP_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include "nanomodbus.h"  

/*
 * Modbus Addressing Overview
 *
 * Function Codes and Address Ranges:
 * ----------------------------------
 * Coils (0x01, 0x05):              Addresses 00001?09999
 *   - Read/Write single-bit values (booleans)
 *
 * Discrete Inputs (status) (0x02): Addresses 10001?19999
 *   - Read-only single-bit inputs
 *
 * Input Registers (0x04):          Addresses 30001?39999
 *   - Read-only 16-bit data
 *
 * Holding Registers (0x03, 0x06): Addresses 40001?49999
 *   - Read/Write 16-bit data
 *
 * Note:
 * These address ranges are logical/semantic representations.
 * In actual Modbus requests, addresses are zero-based offsets.
 * For example, register 40001 corresponds to address 0 in a Modbus frame.
 */
    
// ------------------- Modbus Limits -------------------
#define COILS_ADDR_MAX          4
#define REGS_INPUT_ADDR_MAX     10
#define REGS_HOLDING_ADDR_MAX   8
#define MAX_SLAVE_VALUE         255
#define MIN_SLAVE_VALUE         1

// ------------------------------------------- Coils -------------------------------------------
 /* Coil Address | Function   (used to order history/max/min/mean voltage/current from panel/battery/consumption)
 * -------------|---------------------------------------------------------
 * 0            | Enable/Disable transducer
 * 1            | Measure All (Phase and Power)
 * 2            | Measure Power
 * 3            | Measure Phase
 * 4            | Start frequency sweep - Auto-determine resonance frecuency
 * 5            | Save current settings
 * 6            | Reserved
 */
typedef struct  // A single nmbs_bitfield variable can keep 2000 coils
{
    nmbs_bitfield coils;
}coils;
// ---------------------------------------------------------------------------------------------

// ------------------------------------- Holding Registers -------------------------------------
// Default Values
#define RTU_SERVER_ADDRESS_DEFAULT  20      // Our RTU address (Slave number 20) - Slaves can be 0 to 255
#define RTU_BAUDRATE_DEFAULT        9600
#define DEFAULT_FRECUENCY           140000
#define DEFAULT_FRECUENCY_HIGH      2
#define DEFAULT_FRECUENCY_LOW       8928
#define DEFAULT_VOLTAGE_LEVEL       100
#define DEFAULT_ON_TIME_MS          500
#define DEFAULT_OFF_TIME_MS         500

// EEPROM Configuration Flags (determine whether to use default (RAM) values or previously saved (EEPROM) ones.)    
#define IS_IN_MEMORY_VALUE          0x00    // Use defaults (RAM)
#define IS_IN_MEMORY_EPP_ADDR       0x7001  // EEPROM address for the flag  
// In memory addresses for the content that goes into holding registers
#define SLAVE_EPP_ADDR              0x7002
#define BAUDRATE_EPP_ADDR           0x7004



typedef struct
{
    uint16_t addr_slave;            // 40000 - Holding Register 0 - Slave Num
    uint16_t baudrate;              // 40001 - Holding Register 1 - COM Baudrate (9600 default))
    
    uint16_t frequency_hi;          // 40002 - Holding Register 2 - High word (upper 16 bits) of Current/Desired Frequency (0.1 kHz jumps) 
    uint16_t frequency_lo;          // 40003 - Holding Register 3 - Low word (lower 16 bits) of current/desired frecunecy
    uint16_t voltage_level;         // 40004 - Holding Register 4 - voltage level (%)
    uint16_t on_time_ms;            // 40005 - Holding Register 5 - on_time_ms
    uint16_t off_time_ms;           // 40006 - Holding Register 6 - off_time_ms
}holding_register;

// ---------------------------------------------------------------------------------------------

// -------------------------------------- Input Registers --------------------------------------
/* Input registers contain:
 * The sensor type (In this case defined as code 100 -> Energy Board)
 * The sensor´s/board´s serial number (in this case 1, later to be changed for a defined convention) */

#define SENSOR_TYPE_ADDR        0
#define SERIAL_NUMBER_ADDR      1       // Input Register 0 

// Default Values for Input Registers
#define RTU_SERIAL_NUMBER_DEFAULT   1
#define RTU_SENSOR_TYPE_DEFAULT     700

typedef struct
{
    uint16_t sensor_type;               // 30000 - Input Register 0 - Code for phase/resonance/power sensor
    uint16_t serial_number;             // 30001 - Input Register 1 - Sensor´s serial number
       
    uint16_t power_output;              // 30003 - Input Register 2 - Measured Output Power [W])
    int16_t  phase_difference;          // 30004 - Input Register 3 - Phase between V and I
    uint16_t voltage_rms;               // 30005 - Input Register 4 - RMS Voltage
    uint16_t current_rms;               // 30006 - Input Register 5 - RMS Current
    uint16_t resonance_freq_hi;         // 30007 - Input Register 6 - Obtained Resonance Frecuency (high part, big endian)
    uint16_t resonance_freq_lo;         // 30008 - Input Register 7 - Obtained Resonance Frecuency (low part, big endian))
    uint16_t resonance_status;          // 30009 - Input Register 8 - (1 = resonance, 0 = no)
    uint16_t system_status;             // 30010 - Input Register 9 - System Status
    uint16_t last_error;                // 30011 - Input Register 10 - Last Error
}input_register;
// ---------------------------------------------------------------------------------------------

// With all of this data the modbus registers struct is built
typedef struct
{
    coils server_coils;
    input_register server_input_register;
    holding_register server_holding_register;
}mod_bus_registers;  // This is what will be created and later accessed when using the read/write coil/holding_registers/input_registers handlers

// ----------------------------------- Function prototypes --------------------------------------- 

// Initializes Modbus holding register structures with default values.
void set_holding_regs_to_default(holding_register* regs);

// Initializes Modbus register structures with default values.
void default_values_register(mod_bus_registers* registers);

void holding_register_change_handler(mod_bus_registers* registers,holding_register* prev_holding_regs); // nmbs_t* nmbs 

// Handles and processes Modbus error codes (currently commented out).
void check_error_modbus(nmbs_error err); 


int32_t read_serial(uint8_t* buf, uint16_t count, int32_t byte_timeout_ms, void* arg);  // Reads 'count' bytes from the serial port into 'buf' with a timeout.

int32_t write_serial(const uint8_t* buf, uint16_t count, int32_t byte_timeout_ms, void* arg); // Writes 'count' bytes from 'buf' to the serial port with a timeout.

nmbs_error handle_read_coils(uint16_t address, uint16_t quantity, nmbs_bitfield coils_out, uint8_t unit_id, void *arg); // Handles Modbus "Read Coils" requests by copying coil states to output buffer.

nmbs_error handle_write_single_coil(uint16_t address, bool coils, uint8_t unit_id, void *arg); // Handles Modbus "Write Single Coil" requests by updating a single coil state.

nmbs_error handler_read_input_registers(uint16_t address, uint16_t quantity, uint16_t* registers_out, uint8_t unit_id, void *arg); // Handles Modbus "Read Input Registers" requests by copying input register values to output buffer.

nmbs_error handler_read_holding_registers(uint16_t address, uint16_t quantity, uint16_t* registers_out, uint8_t unit_id, void *arg); // Handles Modbus "Read Holding Registers" requests by copying holding register values to output buffer.

nmbs_error handle_write_single_register(uint16_t address, const uint16_t* registers, uint8_t unit_id, void *arg); // Handles Modbus "Write Single Register" requests by updating a single holding register value.

void check_error_modbus(nmbs_error err); // Handles and processes Modbus error codes (currently commented out).

// ---------------------------------------------------------------------------------------------

#ifdef	__cplusplus
}
#endif

#endif	/* MODBUS_IMP_H */

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
#define COILS_ADDR_MAX          7
#define REGS_INPUT_ADDR_MAX     26
#define REGS_HOLDING_ADDR_MAX   25
#define MAX_SLAVE_VALUE         255
#define MIN_SLAVE_VALUE         1

// ------------------------------------------- Coils -------------------------------------------
 /* Coil Address | Function   (used to order history/max/min/mean voltage/current from panel/battery/consumption)
 * -------------|---------------------------------------------------------
 * 0            | Enable/Disable transducer
 * 1            | Measure All (Phase and Power)
 * 2            | Measure Power
 * 3            | Measure Phase
 * 4            | Update output frequency
 * 5            | Auto-determine resonance frequency
 * 6            | Internal Phase Measurement
 */
typedef struct  // A single nmbs_bitfield variable can keep 2000 coils
{
    nmbs_bitfield coils;
}coils;
// ---------------------------------------------------------------------------------------------

// ------------------------------------- Holding Registers -------------------------------------
// Default Values
#define RTU_SERVER_ADDRESS_DEFAULT          20      // Our RTU address (Slave number 20) - Slaves can be 0 to 255
#define RTU_BAUDRATE_DEFAULT                9600

#define DEFAULT_FRECUENCY                   60000
#define DEFAULT_FRECUENCY_HIGH              0
#define DEFAULT_FRECUENCY_LOW               60000
    
#define DEFAULT_VOLTAGE_LEVEL               100

#define DEFAULT_ON_TIME_MS                  500
#define DEFAULT_OFF_TIME_MS                 500

#define DEFAULT_FREQ_MODE                   0   

#define DEFAULT_SAMPLES_AMOUNT              20
#define DEFAULT_FREQ_STEP                   100
#define DEFAULT_FREQ_START_HI               0
#define DEFAULT_FREQ_START_LO               48000 
#define DEFAULT_FREQ_END_HI                 0 
#define DEFAULT_FREQ_END_LO                 51000  

#define DEFAULT_BEST_FREQ_PHASE             32767
#define DEFAULT_BEST_FREQ_CURR              0

#define DEFAULT_VOLT_AD_GAIN                4188    // Vout = Vin*0.4188 
#define DEFAULT_CURR_AD_GAIN                39493   // Vout = Vin*39.493
#define DEFAULT_SHUNT_RES                   1010    // I = (((ADC_Value / 4095) * Vref) / (current_adecuator_gain/1000)) / (shunt_res/100)
#define MAX_ADC_SAMPLES                     10      // Each ADC measurement is the average of max 10 ADC samples

#define DEFAULT_MAX_DISTANCE_HZ             20000
#define DEFAULT_AUTO_FREQ_SWEEP_WIDTH       1000
#define MAX_AUTO_FREQ_SWEEP_WIDTH           10000
#define DEFAULT_CLOSED_LOOP_CONTROL_ENABLE  0
#define DEFAULT_CLOSED_LOOP_CONTROL_PERIOD  10     // Default 10 minutes
#define MIN_CLOSED_LOOP_CONTROL_PERIOD      3     // Min 3 minutes
#define MAX_CLOSED_LOOP_CONTROL_PERIOD      3600    // Max 60 minutes

// Holding registers for serial number write operations     
#define SN_PASSWORD_CORRECT             8336    // This value must be written into holding register 19 in order to enable a serial number write.
#define SN_WRITE_TIMEOUT                15      // Duration (in seconds) for which the serial number write operation remains enabled after correct password entry
                                            // Keep in mind, this will control a counter inside the TMR0 interrupt so if the interrupt period changes, so will this duration
#define SNW_STATUS_IDLE                 0 
#define SNW_STATUS_SUCCESS              1 
#define SNW_STATUS_WRONG_PASS           2 
#define SNW_STATUS_NOT_AUTHORIZED       3 
#define SNW_STATUS_NOT_AVAILABLE        4 

#define MAX_ADC_SAMPLES                 10

#define MAX_ALLOWED_DISTANCE_HZ 60000
// -----------------------------------------------------------------------------------------------------------------------

typedef struct
{
    uint16_t addr_slave;                    // 40000 - Holding Register 0 - Slave Num
    uint16_t baudrate;                      // 40001 - Holding Register 1 - COM Baudrate (9600 default))
    
    uint16_t frequency_hi;                  // 40002 - Holding Register 2 - High word (upper 16 bits) of Current/Desired Frequency (0.1 kHz jumps) 
    uint16_t frequency_lo;                  // 40003 - Holding Register 3 - Low word (lower 16 bits) of current/desired frecunecy
    uint16_t voltage_level;                 // 40004 - Holding Register 4 - voltage level (%)
    uint16_t on_time_ms;                    // 40005 - Holding Register 5 - on_time_ms
    uint16_t off_time_ms;                   // 40006 - Holding Register 6 - off_time_ms
    
    uint16_t freq_mode;                     // 40007 - Holding Register 7 - Set frecuency mode: 0- Set by user | 1- Lock to resonance frequency (closed loop control)
    
    uint16_t samples_amount;                // 40008 - Holding Register 8 - Amount of samples taken per phase measurement
    uint16_t freq_step;                     // 40009 - Holding Register 9 - Frequency step for resonance frequency auto-detection (example 0.1 kHz jumps) [Hz/100]
    uint16_t freq_range_start_hi;           // 40010 - Holding Register 10 - Start of the frequency range to be tested when auto-detecting resonance frequency (hi)
    uint16_t freq_range_start_lo;           // 40011 - Holding Register 11 - Start of the frequency range to be tested when auto-detecting resonance frequency (lo)
    uint16_t freq_range_end_hi;             // 40012 - Holding Register 12 - End of the frequency range to be tested when auto-detecting resonance frequency (hi)
    uint16_t freq_range_end_lo;             // 40013 - Holding Register 13 - End of the frequency range to be tested when auto-detecting resonance frequency (lo)
    
    uint16_t voltage_adecuator_gain;        // 40014 - Holding Register 14 - Calibration: RLCr Voltage = ((ADC_Value / 4095) * Vref) / (voltage_adecuator_gain/10000))
    uint16_t current_adecuator_gain;        // 40015 - Holding Register 15 - Calibration: r Voltage = ((ADC_Value / 4095) * Vref) / (current_adecuator_gain/1000))
    uint16_t shunt_res;                     // 40016 - Holding Register 16 - Shunt resistor for current determination (I = V / R) [Ohm*100]
    uint16_t adc_samples_amount;            // 40017 - Holding Register 17 - Ammount of samples to be averaged for ADC mreasurements.
    
    uint16_t phase_curr_max_distance;       // 40018 - Holding Register 18 - Max disntance (Hz) between Best Current Freq and Best Phase Freq (avois anti-resonance)
    uint16_t auto_freq_sweep_width;         // 40019 - Holding Register 19 - After resonance has been obtained, the control-freq-sweep will be performed with res_freq +/- auto_freq_sweep_width
    uint16_t closed_loop_control_enable;    // 40020 - Holding Register 20 - Enables/Disables closed loop control
    uint16_t closed_loop_control_period;    // 40021 - Holding Register 21 - Period for closed loop control resonance frequeency sweeps [seconds]
    
    uint16_t serial_number_in;              // 40022 - Holding Register 22 - Use this register to write the serial number (first input the password)
    uint16_t sn_password;                   // 40023 - Holding Register 23 - Writing the correct value into this register enables 1 serial number write for 15 seconds
    uint16_t sn_write_status;               // 40024 - Holding Register 24 - Status for the last serial number write attempt 
                                            // sn_write_status can be 0 - Idle / Not triggered | 1 - Write success | 2 - Incorrect password | 3- Write not authorized | 4- Not Available
}holding_register;

// ---------------------------------------------------------------------------------------------

// -------------------------------------- Input Registers --------------------------------------
/* Input registers contain:
 * The sensor type (In this case defined as code 100 -> Energy Board)
 * The sensor?s/board?s serial number (in this case 1, later to be changed for a defined convention) */

// Default Values for Input Registers
#define RTU_SERIAL_NUMBER_DEFAULT   1
#define RTU_SENSOR_TYPE_DEFAULT     700

typedef struct
{
    uint16_t sensor_type;                   // 30000 - Input Register 0 - Code for phase/resonance/power sensor
    uint16_t serial_number;                 // 30001 - Input Register 1 - Sensor?s serial number
       
    uint16_t phase_difference;              // 30002 - Input Register 2 - Measured Phase between V and I in tmr1 ticks
    uint16_t phase_ready;                   // 30003 - Input Register 3 - 0 - Phase measurement is still in progress | 1 - phase measurement ready
    uint16_t ADC_peak_voltage;              // 30004 - Input Register 4 - Peak Voltage in ADC steps
    uint16_t ADC_peak_current;              // 30005 - Input Register 5 - Peak Current in ADC steps
    uint16_t volt_adc_measurement_ready;    // 30006 - Input Register 6 - Voltage AVG ADC measurement ready for reading mobdus register flag
    uint16_t curr_adc_measurement_ready;    // 30007 - Input Register 7 - Current AVG ADC measurement ready for reading mobdus register flag
    
    uint16_t internal_measurement_ready;    // 30008 - Input Register 8 - Internal phase measurement ready flag
    uint16_t res_freq_status;               // 30009 - Input Register 9 - Resonance Frecunecy status: 0 - Not obtained | 1 Obtained | 2 Failed to obtain | 3 Measurement in progress
    
    uint16_t res_freq_hi;                   // 30010 - Input Register 10 - High word (upper 16 bits) of obtained resonance frequency 
    uint16_t res_freq_lo;                   // 30011 - Input Register 11 - Low word (lower 16 bits) of obtained resonance frecunecy
    uint16_t res_freq_phase;                // 30012 - Input Register 12 - Measured phase for the resonance frequency
    uint16_t res_freq_curr;                 // 30013 - Input Register 13 - Measured current for the resonance frequency
    
    uint16_t best_freq_phase_hi;            // 30014 - Input Register 14 - High word (upper 16 bits) of obtained min-phase frequency 
    uint16_t best_freq_phase_lo;            // 30015 - Input Register 15 - Low word (lower 16 bits) of obtained min-phase frecunecy
    uint16_t best_freq_phase_phase;         // 30016 - Input Register 16 - Measured phase for the lowest phase frequency
    uint16_t best_freq_phase_curr;          // 30017 - Input Register 17 - Measured current for the lowest phase frequency
    
    uint16_t best_freq_curr_hi;             // 30018 - Input Register 18 - High word (upper 16 bits) of obtained max-current frequency 
    uint16_t best_freq_curr_lo;             // 30019 - Input Register 19 - Low word (lower 16 bits) of obtained max-current frecunecy 
    uint16_t best_freq_curr_phase;          // 30020 - Input Register 20 - Measured phase for the highest current frequency
    uint16_t best_freq_curr_curr;           // 30021 - Input Register 21 - Measured current for the highest current frequency
   
    uint16_t test_1;                        // 30022 - Input Register 22 - Debugging Register 1
    uint16_t test_2;                        // 30023 - Input Register 22 - Debugging Register 2
    uint16_t test_3;                        // 30024 - Input Register 22 - Debugging Register 3
            
    uint16_t system_status;                 // 30025 - Input Register 13 - System Status
    uint16_t last_error;                    // 30026 - Input Register 14 - Last Error
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

// Handles and updates the registers and the NVM when holding registers are written
void holding_register_change_handler(mod_bus_registers* registers,holding_register* prev_holding_regs, nmbs_t* nmbs); // nmbs_t* nmbs 

// Writes a single 16 bit value into the NVM without deleting the rest of the row
void single_16_bit_nvm_write(uint16_t value);

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
/*
 * File:   modbus_imp.c
 * Author: asus
 *
 * Created on 24 de agosto de 2023, 12:00
 */

/*
   This example application sets up an RTU server and handles modbus requests

   This server supports the following function codes:
   FC 01 (0x01) Read Coils
   FC 03 (0x03) Read Holding Registers
   FC 15 (0x0F) Write Multiple Coils
   FC 16 (0x10) Write Multiple registers
*/

#include "mcc_generated_files/system/system.h"
#include <xc.h>
#include "modbus_imp.h"
#include "nvm_config.h"
#include "eusart1_utils.h"
#include "AD9833.h"
#include "closed_loop_control.h"
#include "sn_handler.h"

int32_t read_serial(uint8_t *buf, uint16_t count, int32_t byte_timeout_ms, void *arg)
{
    int32_t charCount = 0;
    uint32_t timeout = 50000; // Adjust as needed

    while (charCount < count)
    {
        uint32_t t = 0;
        while (!EUSART1_IsRxReady())
        {
            if (t++ > timeout)
                return charCount > 0 ? charCount : NMBS_ERROR_TIMEOUT;
        }
        buf[charCount++] = EUSART1_Read();
    }

    return charCount;
}

int32_t write_serial(const uint8_t *buf, uint16_t count, int32_t byte_timeout_ms, void *arg)
{
    // RW_SetHigh();  // Enable TX  // Use for 485 driver
    int32_t number_of_byte_send = 0;

    while (number_of_byte_send < count)
    {
        if (EUSART1_IsTxReady())
        {
            EUSART1_Write(*buf);
            while (!EUSART1_IsTxDone())
                ;
            buf++;
            number_of_byte_send++;
        }
    }
    while (!EUSART1_IsTxDone())
        ;
    // RW_SetLow(); // Disable TX  // Use for 485 driver

    return number_of_byte_send;
}

nmbs_error handle_read_coils(uint16_t address, uint16_t quantity, nmbs_bitfield coils_out, uint8_t unit_id, void *arg)
{
    if (address + quantity > COILS_ADDR_MAX)
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;

    nmbs_bitfield *server_coils = &((mod_bus_registers *)arg)->server_coils.coils;
    // Read our coils values into coils_out
    for (int i = 0; i < quantity; i++)
    {
        bool value = nmbs_bitfield_read(*server_coils, address + i);
        nmbs_bitfield_write(coils_out, i, value);
    }

    return NMBS_ERROR_NONE;
}

nmbs_error handle_write_single_coil(uint16_t address, bool coils, uint8_t unit_id, void *arg)
{
    if (address > COILS_ADDR_MAX)
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;

    // Write coils values to our server_coils
    nmbs_bitfield *server_coils = &((mod_bus_registers *)arg)->server_coils.coils;
    nmbs_bitfield_write(*server_coils, address, coils);

    return NMBS_ERROR_NONE;
}

nmbs_error handler_read_input_registers(uint16_t address, uint16_t quantity, uint16_t *registers_out, uint8_t unit_id, void *arg)
{
    if (address + quantity > REGS_INPUT_ADDR_MAX)
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;

    uint16_t *server_registers = (uint16_t *)(&(((mod_bus_registers *)arg)->server_input_register));
    for (int i = 0; i < quantity; i++)
        registers_out[i] = server_registers[address + i];

    return NMBS_ERROR_NONE;
}

nmbs_error handler_read_holding_registers(uint16_t address, uint16_t quantity, uint16_t *registers_out, uint8_t unit_id, void *arg)
{
    if (address + quantity > REGS_HOLDING_ADDR_MAX)
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;

    // Read our registers values into registers_out
    uint16_t *server_registers = (uint16_t *)(&(((mod_bus_registers *)arg)->server_holding_register));
    for (int i = 0; i < quantity; i++)
        registers_out[i] = server_registers[address + i];

    return NMBS_ERROR_NONE;
}

nmbs_error handle_write_single_register(uint16_t address, const uint16_t *registers, uint8_t unit_id, void *arg)
{
    if (address > REGS_HOLDING_ADDR_MAX)
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;

    // Write registers values to our server_registers
    uint16_t *server_registers = (uint16_t *)(&(((mod_bus_registers *)arg)->server_holding_register));

    server_registers[address] = registers;

    return NMBS_ERROR_NONE;
}

void set_holding_regs_to_default(holding_register* regs)
{
    regs->addr_slave                    = RTU_SERVER_ADDRESS_DEFAULT;
    regs->baudrate                      = RTU_BAUDRATE_DEFAULT;

    regs->frequency_hi                  = DEFAULT_FRECUENCY_HIGH;
    regs->frequency_lo                  = DEFAULT_FRECUENCY_LOW;

    regs->voltage_level                 = DEFAULT_VOLTAGE_LEVEL;
    regs->on_time_ms                    = DEFAULT_ON_TIME_MS;
    regs->off_time_ms                   = DEFAULT_OFF_TIME_MS;
    
    regs->samples_amount                = DEFAULT_SAMPLES_AMOUNT;
    regs->freq_step                     = DEFAULT_FREQ_STEP;
    
    regs->freq_range_start_hi           = DEFAULT_FREQ_START_HI;
    regs->freq_range_start_lo           = DEFAULT_FREQ_START_LO;
    regs->freq_range_end_hi             = DEFAULT_FREQ_END_HI;
    regs->freq_range_end_lo             = DEFAULT_FREQ_END_LO;
    
    regs->voltage_adecuator_gain        = DEFAULT_VOLT_AD_GAIN;
    regs->current_adecuator_gain        = DEFAULT_CURR_AD_GAIN;
    regs->shunt_res                     = DEFAULT_SHUNT_RES;
    regs->adc_samples_amount            = MAX_ADC_SAMPLES;
    regs->phase_curr_max_distance       = DEFAULT_MAX_DISTANCE_HZ;
    regs->auto_freq_sweep_width         = DEFAULT_AUTO_FREQ_SWEEP_WIDTH;
    regs->closed_loop_control_enable    = DEFAULT_CLOSED_LOOP_CONTROL_ENABLE;
    regs->closed_loop_control_period    = DEFAULT_CLOSED_LOOP_CONTROL_PERIOD;
    
    regs->external_res_freq_hi          = 0;
    regs->external_res_freq_lo          = 0;
    
    regs->serial_number_in              = 0;
    regs->sn_password                   = 0;
    regs->sn_write_status               = SNW_STATUS_IDLE;
}

void default_values_register(mod_bus_registers* registers)
{
    // Clear all registers
    m_memset(&(registers->server_coils),            0, sizeof(registers->server_coils));
    m_memset(&(registers->server_input_register),   0, sizeof(registers->server_input_register));
    m_memset(&(registers->server_holding_register), 0, sizeof(registers->server_holding_register));
    
    // Initialize holding registers with defaults
    set_holding_regs_to_default(&registers->server_holding_register);
        
    // Initialize input registers with defaults
    registers->server_input_register.sensor_type                = RTU_SENSOR_TYPE_DEFAULT;
    registers->server_input_register.serial_number              = RTU_SERIAL_NUMBER_DEFAULT;
    registers->server_input_register.phase_difference           = 0;
    registers->server_input_register.phase_ready                = 0; 
    registers->server_input_register.ADC_peak_voltage           = 0;
    registers->server_input_register.ADC_peak_current           = 0;
    registers->server_input_register.volt_adc_measurement_ready = 0;
    registers->server_input_register.curr_adc_measurement_ready = 0;
    
    registers->server_input_register.internal_measurement_ready = 0;
    registers->server_input_register.res_freq_status            = 0;
    
    registers->server_input_register.res_freq_hi                = DEFAULT_FRECUENCY_HIGH;
    registers->server_input_register.res_freq_lo                = DEFAULT_FRECUENCY_LOW;
    registers->server_input_register.res_freq_phase             = DEFAULT_BEST_FREQ_PHASE;
    registers->server_input_register.res_freq_curr              = DEFAULT_BEST_FREQ_CURR;
    
    registers->server_input_register.best_freq_phase_hi         = DEFAULT_FRECUENCY_HIGH;
    registers->server_input_register.best_freq_phase_lo         = DEFAULT_FRECUENCY_LOW;
    registers->server_input_register.best_freq_phase_phase      = DEFAULT_BEST_FREQ_PHASE;
    registers->server_input_register.best_freq_phase_curr       = DEFAULT_BEST_FREQ_CURR;
            
    registers->server_input_register.best_freq_curr_hi          = DEFAULT_FRECUENCY_HIGH;
    registers->server_input_register.best_freq_curr_lo          = DEFAULT_FRECUENCY_LOW;
    registers->server_input_register.best_freq_curr_phase       = DEFAULT_BEST_FREQ_PHASE;
    registers->server_input_register.best_freq_curr_curr        = DEFAULT_BEST_FREQ_CURR;
    
    registers->server_input_register.test_1                     = 0;
    registers->server_input_register.test_2                     = 0;
    registers->server_input_register.test_3                     = 0;
    registers->server_input_register.system_status              = 0;
    registers->server_input_register.last_error                 = 0;
    
    // Slave Number and Baudrate could have been stored in the Non Volatile Memory
    // The first time the NVM is written we write NVM_CONFIG_MAGIC in the first address to indicate that the NVM contains usable data.
    while (NVM_IsBusy());   // Wait until the NVM is ready before reading
    if(EEPROM_Read(EEPROM_MAGIC_ADDR) != EEPROM_CONFIG_MAGIC)
    {
        registers->server_holding_register.addr_slave                    = RTU_SERVER_ADDRESS_DEFAULT;
        registers->server_holding_register.baudrate                      = RTU_BAUDRATE_DEFAULT;
        registers->server_holding_register.frequency_hi                  = DEFAULT_FRECUENCY_HIGH;
        registers->server_holding_register.frequency_lo                  = DEFAULT_FRECUENCY_LOW;

        registers->server_holding_register.voltage_level                 = DEFAULT_VOLTAGE_LEVEL;
        registers->server_holding_register.on_time_ms                    = DEFAULT_ON_TIME_MS;
        registers->server_holding_register.off_time_ms                   = DEFAULT_OFF_TIME_MS;

        registers->server_holding_register.samples_amount                = DEFAULT_SAMPLES_AMOUNT;
        registers->server_holding_register.freq_step                     = DEFAULT_FREQ_STEP;
        
        registers->server_holding_register.freq_range_start_hi           = DEFAULT_FREQ_START_HI;
        registers->server_holding_register.freq_range_start_lo           = DEFAULT_FREQ_START_LO;
        registers->server_holding_register.freq_range_end_hi             = DEFAULT_FREQ_END_HI;
        registers->server_holding_register.freq_range_end_lo             = DEFAULT_FREQ_END_LO;

        registers->server_holding_register.voltage_adecuator_gain        = DEFAULT_VOLT_AD_GAIN;
        registers->server_holding_register.current_adecuator_gain        = DEFAULT_CURR_AD_GAIN;
        registers->server_holding_register.shunt_res                     = DEFAULT_SHUNT_RES;
        registers->server_holding_register.adc_samples_amount            = MAX_ADC_SAMPLES;
        registers->server_holding_register.phase_curr_max_distance       = DEFAULT_MAX_DISTANCE_HZ;
        registers->server_holding_register.auto_freq_sweep_width         = DEFAULT_AUTO_FREQ_SWEEP_WIDTH;
        registers->server_holding_register.closed_loop_control_enable    = DEFAULT_CLOSED_LOOP_CONTROL_ENABLE;
        registers->server_holding_register.closed_loop_control_period    = DEFAULT_CLOSED_LOOP_CONTROL_PERIOD;

        registers->server_holding_register.serial_number_in              = 0;
        registers->server_holding_register.sn_password                   = 0;
        registers->server_holding_register.sn_write_status               = SNW_STATUS_IDLE;
        
        registers->server_input_register.sensor_type        = RTU_SENSOR_TYPE_DEFAULT;
        registers->server_input_register.serial_number      = RTU_SERIAL_NUMBER_DEFAULT;
        
        // Then load them into the NVM
        NVM_UnlockKeySet(UNLOCK_KEY); 
        while (NVM_IsBusy()); 
        EEPROM_Write(EEPROM_MAGIC_ADDR, EEPROM_CONFIG_MAGIC);
        EEPROM_WriteWord(EEPROM_ADDR_SLAVE_ADDR, RTU_SERVER_ADDRESS_DEFAULT);
        EEPROM_WriteWord(EEPROM_BAUDRATE_ADDR, RTU_BAUDRATE_DEFAULT);
        EEPROM_WriteWord(SENSOR_TYPE_ADDR, RTU_SENSOR_TYPE_DEFAULT);
        EEPROM_WriteWord(SERIAL_NUMBER_ADDR, RTU_SERIAL_NUMBER_DEFAULT);
        
        EEPROM_WriteWord(EEPROM_FREQ_HI_ADDR, DEFAULT_FRECUENCY_HIGH);
        EEPROM_WriteWord(EEPROM_FREQ_LO_ADDR, DEFAULT_FRECUENCY_LOW);
        
        EEPROM_WriteWord(EEPROM_VOLT_LVL_ADDR, DEFAULT_VOLTAGE_LEVEL);
        
        EEPROM_WriteWord(EEPROM_ON_TIME_MS_ADDR, DEFAULT_ON_TIME_MS);
        EEPROM_WriteWord(EEPROM_OFF_TIME_MS_ADDR, DEFAULT_OFF_TIME_MS);
        
        EEPROM_WriteWord(EEPROM_SAMPLES_AMOUNT_ADDR, DEFAULT_SAMPLES_AMOUNT);
        EEPROM_WriteWord(EEPROM_FREQ_STEP_ADDR, DEFAULT_FREQ_STEP);
        
        // Frequency range registers
        EEPROM_WriteWord(EEPROM_FREQ_RANGE_START_HI_ADDR, DEFAULT_FREQ_START_HI);
        EEPROM_WriteWord(EEPROM_FREQ_RANGE_START_LO_ADDR, DEFAULT_FREQ_START_LO);
        EEPROM_WriteWord(EEPROM_FREQ_RANGE_END_HI_ADDR, DEFAULT_FREQ_END_HI);
        EEPROM_WriteWord(EEPROM_FREQ_RANGE_END_LO_ADDR, DEFAULT_FREQ_END_LO);
        
        EEPROM_WriteWord(EEPROM_VOLT_AD_GAIN_ADDR, DEFAULT_VOLT_AD_GAIN);
        EEPROM_WriteWord(EEPROM_CURR_AD_GAIN_ADDR, DEFAULT_CURR_AD_GAIN);
        EEPROM_WriteWord(EEPROM_SHUNT_RES_ADDR, DEFAULT_SHUNT_RES);
        EEPROM_WriteWord(EEPROM_ADC_SAMPLES_ADDR, MAX_ADC_SAMPLES);
        
        EEPROM_WriteWord(EEPROM_MAX_PHASE_CURR_ADDR, DEFAULT_MAX_DISTANCE_HZ);
        EEPROM_WriteWord(EEPROM_AUTO_FREQ_SWEEP_WIDTH_ADDR, DEFAULT_AUTO_FREQ_SWEEP_WIDTH);
        EEPROM_WriteWord(EEPROM_CLOSED_LOOP_CONTROL_ENABLE_ADDR, DEFAULT_CLOSED_LOOP_CONTROL_ENABLE);
        EEPROM_WriteWord(EEPROM_CLOSED_LOOP_CONTROL_PERIOD_ADDR, DEFAULT_CLOSED_LOOP_CONTROL_PERIOD);
        
        // Serial number write registers
        EEPROM_WriteWord(EEPROM_SERIAL_NUMBER_IN_ADDR, 0);
        EEPROM_WriteWord(EEPROM_SN_PASSWORD_ADDR, 0);
        EEPROM_WriteWord(EEPROM_SN_WRITE_STATUS_ADDR, SNW_STATUS_IDLE);
        
        // Resonance frequency results
        EEPROM_WriteWord(EEPROM_RES_FREQ_HI_ADDR, DEFAULT_FRECUENCY_HIGH);
        EEPROM_WriteWord(EEPROM_RES_FREQ_LO_ADDR, DEFAULT_FRECUENCY_LOW);
        EEPROM_WriteWord(EEPROM_BEST_PHASE_FREQ_HI_ADDR, DEFAULT_FRECUENCY_HIGH);
        EEPROM_WriteWord(EEPROM_BEST_PHASE_FREQ_LO_ADDR, DEFAULT_FRECUENCY_LOW);
        EEPROM_WriteWord(EEPROM_BEST_CURR_FREQ_HI_ADDR, DEFAULT_FRECUENCY_HIGH);
        EEPROM_WriteWord(EEPROM_BEST_CURR_FREQ_LO_ADDR, DEFAULT_FRECUENCY_LOW);
    }
    else
    {
        // Load values from EEPROM NVM
        registers->server_holding_register.addr_slave             = EEPROM_ReadWord(EEPROM_ADDR_SLAVE_ADDR);
        registers->server_holding_register.baudrate               = EEPROM_ReadWord(EEPROM_BAUDRATE_ADDR);
        
        EUSART1_SetBaudRate(registers->server_holding_register.baudrate);
        
        registers->server_input_register.sensor_type                    = EEPROM_ReadWord(SENSOR_TYPE_ADDR);
        registers->server_input_register.serial_number                  = EEPROM_ReadWord(SERIAL_NUMBER_ADDR);
        
        registers->server_holding_register.frequency_hi                 = EEPROM_ReadWord(EEPROM_FREQ_HI_ADDR);
        registers->server_holding_register.frequency_lo                 = EEPROM_ReadWord(EEPROM_FREQ_LO_ADDR);
        registers->server_holding_register.voltage_level                = EEPROM_ReadWord(EEPROM_VOLT_LVL_ADDR);
        
        registers->server_holding_register.on_time_ms                   = EEPROM_ReadWord(EEPROM_ON_TIME_MS_ADDR);
        registers->server_holding_register.off_time_ms                  = EEPROM_ReadWord(EEPROM_OFF_TIME_MS_ADDR);
        
        registers->server_holding_register.samples_amount               = EEPROM_ReadWord(EEPROM_SAMPLES_AMOUNT_ADDR);
        registers->server_holding_register.freq_step                    = EEPROM_ReadWord(EEPROM_FREQ_STEP_ADDR);
        
        // Load frequency range registers
        registers->server_holding_register.freq_range_start_hi          = EEPROM_ReadWord(EEPROM_FREQ_RANGE_START_HI_ADDR);
        registers->server_holding_register.freq_range_start_lo          = EEPROM_ReadWord(EEPROM_FREQ_RANGE_START_LO_ADDR);
        registers->server_holding_register.freq_range_end_hi            = EEPROM_ReadWord(EEPROM_FREQ_RANGE_END_HI_ADDR);
        registers->server_holding_register.freq_range_end_lo            = EEPROM_ReadWord(EEPROM_FREQ_RANGE_END_LO_ADDR);
        
        registers->server_holding_register.voltage_adecuator_gain       = EEPROM_ReadWord(EEPROM_VOLT_AD_GAIN_ADDR);
        registers->server_holding_register.current_adecuator_gain       = EEPROM_ReadWord(EEPROM_CURR_AD_GAIN_ADDR);
        registers->server_holding_register.shunt_res                    = EEPROM_ReadWord(EEPROM_SHUNT_RES_ADDR);
        registers->server_holding_register.adc_samples_amount           = EEPROM_ReadWord(EEPROM_ADC_SAMPLES_ADDR);
        registers->server_holding_register.phase_curr_max_distance      = EEPROM_ReadWord(EEPROM_MAX_PHASE_CURR_ADDR);
        registers->server_holding_register.auto_freq_sweep_width        = EEPROM_ReadWord(EEPROM_AUTO_FREQ_SWEEP_WIDTH_ADDR);
        
        // Load serial number write registers
        registers->server_holding_register.serial_number_in             = EEPROM_ReadWord(EEPROM_SERIAL_NUMBER_IN_ADDR);
        registers->server_holding_register.sn_password                  = EEPROM_ReadWord(EEPROM_SN_PASSWORD_ADDR);
        registers->server_holding_register.sn_write_status              = EEPROM_ReadWord(EEPROM_SN_WRITE_STATUS_ADDR);
        
        // Load resonance frequency results
        registers->server_input_register.res_freq_hi                    = EEPROM_ReadWord(EEPROM_RES_FREQ_HI_ADDR);
        registers->server_input_register.res_freq_lo                    = EEPROM_ReadWord(EEPROM_RES_FREQ_LO_ADDR);
        registers->server_input_register.best_freq_phase_hi             = EEPROM_ReadWord(EEPROM_BEST_PHASE_FREQ_HI_ADDR);
        registers->server_input_register.best_freq_phase_lo             = EEPROM_ReadWord(EEPROM_BEST_PHASE_FREQ_LO_ADDR);
        registers->server_input_register.best_freq_curr_hi              = EEPROM_ReadWord(EEPROM_BEST_CURR_FREQ_HI_ADDR);
        registers->server_input_register.best_freq_curr_lo              = EEPROM_ReadWord(EEPROM_BEST_CURR_FREQ_LO_ADDR);
    }
}

void holding_register_change_handler(mod_bus_registers* modbus_data, holding_register* prev_holding_regs, nmbs_t* nmbs)
{    
    // Check for Slave Num (RTU Address) change
    if(modbus_data->server_holding_register.addr_slave != prev_holding_regs->addr_slave)
    {
        prev_holding_regs->addr_slave = modbus_data->server_holding_register.addr_slave;
        EEPROM_WriteWord(EEPROM_ADDR_SLAVE_ADDR, modbus_data->server_holding_register.addr_slave);
        nmbs->address_rtu = (uint8_t)modbus_data->server_holding_register.addr_slave;
    }
    
    // Check for baudrate changes 
    if(modbus_data->server_holding_register.baudrate != prev_holding_regs->baudrate)
    {
        if(EUSART1_SetBaudRate(modbus_data->server_holding_register.baudrate))
        {
            prev_holding_regs->baudrate = modbus_data->server_holding_register.baudrate;
            EEPROM_WriteWord(EEPROM_BAUDRATE_ADDR, modbus_data->server_holding_register.baudrate);
        }
        else
        {
            modbus_data->server_holding_register.baudrate = prev_holding_regs->baudrate;
        }
    }
    
    // Check for changes in frequency registers
    if(modbus_data->server_holding_register.frequency_hi != prev_holding_regs->frequency_hi)
    {
        prev_holding_regs->frequency_hi = modbus_data->server_holding_register.frequency_hi;
        EEPROM_WriteWord(EEPROM_FREQ_HI_ADDR, modbus_data->server_holding_register.frequency_hi);
    }
    if(modbus_data->server_holding_register.frequency_lo != prev_holding_regs->frequency_lo)
    {
        prev_holding_regs->frequency_lo = modbus_data->server_holding_register.frequency_lo;
        EEPROM_WriteWord(EEPROM_FREQ_LO_ADDR, modbus_data->server_holding_register.frequency_lo);
    }
    
    // Check for changes in voltage level
    if(modbus_data->server_holding_register.voltage_level != prev_holding_regs->voltage_level)
    {
        prev_holding_regs->voltage_level = modbus_data->server_holding_register.voltage_level;
        EEPROM_WriteWord(EEPROM_VOLT_LVL_ADDR, modbus_data->server_holding_register.voltage_level);
    }
    
    // Check for changes in ON time
    if(modbus_data->server_holding_register.on_time_ms != prev_holding_regs->on_time_ms)
    {
        prev_holding_regs->on_time_ms = modbus_data->server_holding_register.on_time_ms;
        EEPROM_WriteWord(EEPROM_ON_TIME_MS_ADDR, modbus_data->server_holding_register.on_time_ms);
    }
    
    // Check for changes in OFF time
    if(modbus_data->server_holding_register.off_time_ms != prev_holding_regs->off_time_ms)
    {
        prev_holding_regs->off_time_ms = modbus_data->server_holding_register.off_time_ms;
        EEPROM_WriteWord(EEPROM_OFF_TIME_MS_ADDR, modbus_data->server_holding_register.off_time_ms);
    }
    
    // Check for changes in Samples amount
    if(modbus_data->server_holding_register.samples_amount != prev_holding_regs->samples_amount)
    {
        prev_holding_regs->samples_amount = modbus_data->server_holding_register.samples_amount;
        EEPROM_WriteWord(EEPROM_SAMPLES_AMOUNT_ADDR, modbus_data->server_holding_register.samples_amount);
    }
    
    // Check for changes in frequency step
    if(modbus_data->server_holding_register.freq_step != prev_holding_regs->freq_step)
    {
        prev_holding_regs->freq_step = modbus_data->server_holding_register.freq_step;
        EEPROM_WriteWord(EEPROM_FREQ_STEP_ADDR, modbus_data->server_holding_register.freq_step);
    }
    
    // Check for changes in frequency range registers (auto-resonance detection frequency range)
    if(modbus_data->server_holding_register.freq_range_start_hi != prev_holding_regs->freq_range_start_hi)
    {
        prev_holding_regs->freq_range_start_hi = modbus_data->server_holding_register.freq_range_start_hi;
        EEPROM_WriteWord(EEPROM_FREQ_RANGE_START_HI_ADDR, modbus_data->server_holding_register.freq_range_start_hi);
    }
    if(modbus_data->server_holding_register.freq_range_start_lo != prev_holding_regs->freq_range_start_lo)
    {
        prev_holding_regs->freq_range_start_lo = modbus_data->server_holding_register.freq_range_start_lo;
        EEPROM_WriteWord(EEPROM_FREQ_RANGE_START_LO_ADDR, modbus_data->server_holding_register.freq_range_start_lo);
    }
    if(modbus_data->server_holding_register.freq_range_end_hi != prev_holding_regs->freq_range_end_hi)
    {
        prev_holding_regs->freq_range_end_hi = modbus_data->server_holding_register.freq_range_end_hi;
        EEPROM_WriteWord(EEPROM_FREQ_RANGE_END_HI_ADDR, modbus_data->server_holding_register.freq_range_end_hi);
    }
    if(modbus_data->server_holding_register.freq_range_end_lo != prev_holding_regs->freq_range_end_lo)
    {
        prev_holding_regs->freq_range_end_lo = modbus_data->server_holding_register.freq_range_end_lo;
        EEPROM_WriteWord(EEPROM_FREQ_RANGE_END_LO_ADDR, modbus_data->server_holding_register.freq_range_end_lo);
    }
    
    // Check for changes in voltage_adecuator_gain
    if(modbus_data->server_holding_register.voltage_adecuator_gain != prev_holding_regs->voltage_adecuator_gain)
    {
        prev_holding_regs->voltage_adecuator_gain = modbus_data->server_holding_register.voltage_adecuator_gain;
        EEPROM_WriteWord(EEPROM_VOLT_AD_GAIN_ADDR, modbus_data->server_holding_register.voltage_adecuator_gain);
    }
    
    // Check for changes in current_adecuator_gain
    if(modbus_data->server_holding_register.current_adecuator_gain != prev_holding_regs->current_adecuator_gain)
    {
        prev_holding_regs->current_adecuator_gain = modbus_data->server_holding_register.current_adecuator_gain;
        EEPROM_WriteWord(EEPROM_CURR_AD_GAIN_ADDR, modbus_data->server_holding_register.current_adecuator_gain);
    }
    
    // Check for changes in shunt resistor value
    if(modbus_data->server_holding_register.shunt_res != prev_holding_regs->shunt_res)
    {
        prev_holding_regs->shunt_res = modbus_data->server_holding_register.shunt_res;
        EEPROM_WriteWord(EEPROM_SHUNT_RES_ADDR, modbus_data->server_holding_register.shunt_res);
    }
    
    // Check for changes in ADC samples amount
    if(modbus_data->server_holding_register.adc_samples_amount != prev_holding_regs->adc_samples_amount)
    {
        if(modbus_data->server_holding_register.adc_samples_amount > MAX_ADC_SAMPLES) {modbus_data->server_holding_register.adc_samples_amount = MAX_ADC_SAMPLES;}
        prev_holding_regs->adc_samples_amount = modbus_data->server_holding_register.adc_samples_amount;
        EEPROM_WriteWord(EEPROM_ADC_SAMPLES_ADDR, modbus_data->server_holding_register.adc_samples_amount);
    }

    // Check for changes in MAX best_phase_freq - best_curr_freq distance
    if(modbus_data->server_holding_register.phase_curr_max_distance != prev_holding_regs->phase_curr_max_distance)
    {
        if (modbus_data->server_holding_register.phase_curr_max_distance > MAX_ALLOWED_DISTANCE_HZ || 
        modbus_data->server_holding_register.phase_curr_max_distance <= 0) {
        modbus_data->server_holding_register.phase_curr_max_distance = MAX_ALLOWED_DISTANCE_HZ;}
        
        prev_holding_regs->phase_curr_max_distance = modbus_data->server_holding_register.phase_curr_max_distance;
        EEPROM_WriteWord(EEPROM_MAX_PHASE_CURR_ADDR, modbus_data->server_holding_register.phase_curr_max_distance);
    }
    
    // Check for changes in frequency hi and lo limits (width) for closed loop control frequency sweeps
    if(modbus_data->server_holding_register.auto_freq_sweep_width != prev_holding_regs->auto_freq_sweep_width)
    {
        if (modbus_data->server_holding_register.auto_freq_sweep_width > MAX_AUTO_FREQ_SWEEP_WIDTH || 
        modbus_data->server_holding_register.auto_freq_sweep_width <= 0) {
        modbus_data->server_holding_register.auto_freq_sweep_width = MAX_AUTO_FREQ_SWEEP_WIDTH;}
        
        prev_holding_regs->auto_freq_sweep_width = modbus_data->server_holding_register.auto_freq_sweep_width;
        EEPROM_WriteWord(EEPROM_AUTO_FREQ_SWEEP_WIDTH_ADDR, modbus_data->server_holding_register.auto_freq_sweep_width);
    }
    
    extern bool closed_loop_enabled;
    extern bool closed_loop_timer_enabled;
    
    // Check for changes in closed_loop_control_enable value
    if(modbus_data->server_holding_register.closed_loop_control_enable != prev_holding_regs->closed_loop_control_enable)
    {
        prev_holding_regs->closed_loop_control_enable = modbus_data->server_holding_register.closed_loop_control_enable;
        EEPROM_WriteWord(EEPROM_CLOSED_LOOP_CONTROL_ENABLE_ADDR, modbus_data->server_holding_register.closed_loop_control_enable);
        closed_loop_enabled = (modbus_data->server_holding_register.closed_loop_control_enable == 1);
        if (closed_loop_enabled != closed_loop_timer_enabled)  // Enable timer if it should be on, disable it if it should be off
        {
            closed_loop_enabled ? enable_closed_loop_timer() : disable_closed_loop_timer();
        }    
    }
    
    extern uint16_t closed_loop_period_seconds;
    
    // Check for changes in frequency hi and lo limits (width) for closed loop control frequency sweeps
    if(modbus_data->server_holding_register.closed_loop_control_period != prev_holding_regs->closed_loop_control_period)
    {
        if(closed_loop_enabled) // Closed loop must be disabled for the timer to be changed
        {
            modbus_data->server_holding_register.closed_loop_control_period = prev_holding_regs->closed_loop_control_period;
            modbus_data->server_input_register.system_status = 20; // Error when attempting to write holding register 21
        }
        else
        {
            // Check limits
            if (modbus_data->server_holding_register.closed_loop_control_period > MAX_CLOSED_LOOP_CONTROL_PERIOD || 
                modbus_data->server_holding_register.closed_loop_control_period <= MIN_CLOSED_LOOP_CONTROL_PERIOD) {
                modbus_data->server_holding_register.closed_loop_control_period = DEFAULT_CLOSED_LOOP_CONTROL_PERIOD;}
            // Store into previous state
            prev_holding_regs->closed_loop_control_period = modbus_data->server_holding_register.closed_loop_control_period;
            // Update global variable
            closed_loop_period_seconds = modbus_data->server_holding_register.closed_loop_control_period;
            // Update NVM
            EEPROM_WriteWord(EEPROM_CLOSED_LOOP_CONTROL_PERIOD_ADDR, modbus_data->server_holding_register.closed_loop_control_period); 
        }
    }
    
    // Check for changes in external_res_freq_hi
    if(modbus_data->server_holding_register.external_res_freq_hi != prev_holding_regs->external_res_freq_hi)
    {
        prev_holding_regs->external_res_freq_hi = modbus_data->server_holding_register.external_res_freq_hi;
    }
    
    // Check for changes in external_res_freq_lo
    if(modbus_data->server_holding_register.external_res_freq_lo != prev_holding_regs->external_res_freq_lo)
    {
        prev_holding_regs->external_res_freq_lo = modbus_data->server_holding_register.external_res_freq_lo;
    }
    
    // ------------------------ SN logic ------------------------
    
    // 1. Handle password submission
    if (modbus_data->server_holding_register.sn_password != prev_holding_regs->sn_password)
    {
        sn_submit_password(
            modbus_data->server_holding_register.sn_password,
            &modbus_data->server_holding_register.sn_write_status
        );

        modbus_data->server_holding_register.sn_password = 0;
        prev_holding_regs->sn_password = 0;
    }
    
    if (modbus_data->server_holding_register.serial_number_in != prev_holding_regs->serial_number_in)
    {
        bool ok = sn_attempt_write(modbus_data->server_holding_register.serial_number_in, &modbus_data->server_holding_register.sn_write_status);

        if (ok)
        {
            // If allowed, update input register
            modbus_data->server_input_register.serial_number = modbus_data->server_holding_register.serial_number_in;
            modbus_data->server_holding_register.sn_write_status = SNW_STATUS_SUCCESS;
            
            // Return timer and closed_loop_enable to their original state
            closed_loop_enabled = (modbus_data->server_holding_register.closed_loop_control_enable == 1);
            if (closed_loop_enabled != closed_loop_timer_enabled)  // Enable timer if it should be on, disable it if it should be off
            {
                closed_loop_enabled ? enable_closed_loop_timer() : disable_closed_loop_timer();
            }  
        }
        modbus_data->server_holding_register.serial_number_in = 0;
        prev_holding_regs->serial_number_in = 0;
    }
    // ------------------------ SN logic ------------------------
}

void single_16_bit_nvm_write(uint16_t value)
{
    /*flash_address_t base = NVM_CONFIG_BASE_ADDR;
    flash_data_t flash_row[PROGMEM_PAGE_SIZE] = {0};  // Ensure size matches page size (e.g., 32)

    // Step 1: Read current flash content into flash_row
    for (uint8_t i = 0; i < PROGMEM_PAGE_SIZE; i++)
    {
        flash_row[i] = FLASH_Read(base + i);
    }

    // Step 2: Modify only the changed value
    flash_row[NVM_ADDR_SLAVE_OFFSET]*/
}

void check_error_modbus(nmbs_error err)
{
    //    switch(err)
    //    {
    //        MBS_ERROR_INVALID_UNIT_ID = -7,  /**< Received invalid unit ID in response from server */
    //        NMBS_ERROR_INVALID_TCP_MBAP = -6, /**< Received invalid TCP MBAP */
    //        NMBS_ERROR_CRC = -5,              /**< Received invalid CRC */
    //        NMBS_ERROR_TRANSPORT = -4,        /**< Transport error */
    //        NMBS_ERROR_TIMEOUT = -3,          /**< Read/write timeout occurred */
    //        NMBS_ERROR_INVALID_RESPONSE = -2, /**< Received invalid response from server */
    //        NMBS_ERROR_INVALID_ARGUMENT = -1, /**< Invalid argument provided */
    //        NMBS_ERROR_NONE = 0,              /**< No error */
    //
    //        // Modbus exceptions
    //        NMBS_EXCEPTION_ILLEGAL_FUNCTION = 1,      /**< Modbus exception 1 */
    //        NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS = 2,  /**< Modbus exception 2 */
    //        NMBS_EXCEPTION_ILLEGAL_DATA_VALUE = 3,    /**< Modbus exception 3 */
    //        NMBS_EXCEPTION_SERVER_DEVICE_FAILURE = 4, /**< Modbus exception 4 */
    //        case NMBS_ERROR_CRC:
    //            //sendPacket((char*)&sensor.sensor_info, sizeof(sensor_info_t));
    //            sendSensorInfo(&sensor.sensor_info);
    //        case CMD_SENSE:
    //            sensor_data = sensor.sense(&sensor.sensor_info.ADCChannel);
    //            break;
    //        case CMD_GET_DATA:
    //            sendSensorData(sensor_data);
    //            break;
    //        default:
    //            //sendPacket("INVALIDO", 8);
    //            break;
    //       }
}

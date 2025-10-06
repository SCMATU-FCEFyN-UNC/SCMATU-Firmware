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

int32_t read_serial(uint8_t* buf, uint16_t count, int32_t byte_timeout_ms, void* arg) 
{    
    int32_t charCount = 0;
    uint32_t timeout = 50000; // Adjust as needed

    while (charCount < count) {
        uint32_t t = 0;
        while (!EUSART1_IsRxReady()) {
            if (t++ > timeout) return charCount > 0 ? charCount : NMBS_ERROR_TIMEOUT;
        }
        buf[charCount++] = EUSART1_Read();
    }

    return charCount;
}

int32_t write_serial(const uint8_t* buf, uint16_t count, int32_t byte_timeout_ms, void* arg)
{ 
    // RW_SetHigh();  // Enable TX  // Use for 485 driver
    int32_t number_of_byte_send = 0;

    while(number_of_byte_send<count)
    {
        if(EUSART1_IsTxReady())
        {
            EUSART1_Write(*buf);
            while(!EUSART1_IsTxDone());
            buf++;
            number_of_byte_send++;
        }
    }
    while(!EUSART1_IsTxDone());
    // RW_SetLow(); // Disable TX  // Use for 485 driver
    
    return number_of_byte_send;
}


nmbs_error handle_read_coils(uint16_t address, uint16_t quantity, nmbs_bitfield coils_out, uint8_t unit_id, void *arg) {
  if (address + quantity > COILS_ADDR_MAX)
    return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;

  nmbs_bitfield *server_coils = &((mod_bus_registers*) arg)->server_coils.coils;
  // Read our coils values into coils_out
  for (int i = 0; i < quantity; i++) {
    bool value = nmbs_bitfield_read(*server_coils, address + i);
    nmbs_bitfield_write(coils_out, i, value);
  }

  return NMBS_ERROR_NONE;
}


nmbs_error handle_write_single_coil(uint16_t address, bool coils, uint8_t unit_id, void *arg) {
  if (address > COILS_ADDR_MAX)
    return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;

  // Write coils values to our server_coils
  nmbs_bitfield *server_coils = &((mod_bus_registers*) arg)->server_coils.coils;
  nmbs_bitfield_write(*server_coils, address, coils);
  
  return NMBS_ERROR_NONE;
}

nmbs_error handler_read_input_registers(uint16_t address, uint16_t quantity, uint16_t* registers_out, uint8_t unit_id, void *arg) {
    if (address + quantity > REGS_INPUT_ADDR_MAX)
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;

    uint16_t* server_registers = (uint16_t *)(&(((mod_bus_registers*) arg)->server_input_register));
    for (int i = 0; i < quantity; i++)
        registers_out[i] = server_registers[address + i];

    return NMBS_ERROR_NONE;
}

nmbs_error handler_read_holding_registers(uint16_t address, uint16_t quantity, uint16_t* registers_out, uint8_t unit_id, void *arg) {
  if (address + quantity > REGS_HOLDING_ADDR_MAX)
    return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;

  // Read our registers values into registers_out
  uint16_t* server_registers = (uint16_t *)(&(((mod_bus_registers*) arg)->server_holding_register));
  for (int i = 0; i < quantity; i++)
    registers_out[i] = server_registers[address + i];

  return NMBS_ERROR_NONE;
}


nmbs_error handle_write_single_register(uint16_t address, const uint16_t* registers, uint8_t unit_id, void *arg) {
  if (address > REGS_HOLDING_ADDR_MAX)
    return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;

  // Write registers values to our server_registers
  uint16_t* server_registers = (uint16_t *)(&(((mod_bus_registers*) arg)->server_holding_register));

  server_registers[address] = registers;

  return NMBS_ERROR_NONE;
}

void set_holding_regs_to_default(holding_register* regs)
{
    regs->addr_slave                = RTU_SERVER_ADDRESS_DEFAULT;
    regs->baudrate                  = RTU_BAUDRATE_DEFAULT;

    regs->frequency_hi              = DEFAULT_FRECUENCY_HIGH;
    regs->frequency_lo              = DEFAULT_FRECUENCY_LOW;

    regs->voltage_level             = DEFAULT_VOLTAGE_LEVEL;
    regs->on_time_ms                = DEFAULT_ON_TIME_MS;
    regs->off_time_ms               = DEFAULT_OFF_TIME_MS;
    
    regs->freq_mode                 = DEFAULT_FREQ_MODE;
    
    regs->samples_amount            = DEFAULT_SAMPLES_AMOUNT;
    regs->freq_step                 = DEFAULT_FREQ_STEP;
    
    regs->voltage_adecuator_gain    = DEFAULT_VOLT_AD_GAIN;
    regs->current_adecuator_gain    = DEFAULT_CURR_AD_GAIN;
    regs->shunt_res                 = DEFAULT_SHUNT_RES;
    
    regs->serial_number_in          = 0;
    regs->sn_password               = 0;
    regs->sn_write_status           = SNW_STATUS_IDLE;
}

void default_values_register(mod_bus_registers* registers)
{
    m_memset(&(registers->server_coils),            0 ,sizeof(registers->server_coils));
    m_memset(&(registers->server_input_register),   0 ,sizeof(registers->server_input_register));
    m_memset(&(registers->server_holding_register), 0 ,sizeof(registers->server_holding_register));
        
    registers->server_holding_register.frequency_hi             = DEFAULT_FRECUENCY_HIGH;
    registers->server_holding_register.frequency_lo             = DEFAULT_FRECUENCY_LOW;
    
    registers->server_holding_register.voltage_level            = DEFAULT_VOLTAGE_LEVEL;
    registers->server_holding_register.on_time_ms               = DEFAULT_ON_TIME_MS;
    registers->server_holding_register.off_time_ms              = DEFAULT_OFF_TIME_MS;
    
    registers->server_holding_register.freq_mode                = DEFAULT_FREQ_MODE;
    
    registers->server_holding_register.samples_amount           = DEFAULT_SAMPLES_AMOUNT;
    registers->server_holding_register.freq_step                = DEFAULT_FREQ_STEP;
    
    registers->server_holding_register.voltage_adecuator_gain   = DEFAULT_VOLT_AD_GAIN;
    registers->server_holding_register.current_adecuator_gain   = DEFAULT_CURR_AD_GAIN;
    registers->server_holding_register.shunt_res                = DEFAULT_SHUNT_RES;
    
    registers->server_holding_register.serial_number_in             = 0;
    registers->server_holding_register.sn_password                  = 0;
    registers->server_holding_register.sn_write_status              = SNW_STATUS_IDLE;
    
    registers->server_input_register.serial_number              = RTU_SERIAL_NUMBER_DEFAULT;
    registers->server_input_register.sensor_type                = RTU_SENSOR_TYPE_DEFAULT;
    registers->server_input_register.phase_difference           = 0;
    registers->server_input_register.ADC_peak_voltage           = 0;
    registers->server_input_register.ADC_peak_current           = 0;
    registers->server_input_register.system_status              = 0;
    registers->server_input_register.last_error                 = 0;
    
    // Slave Number and Baudrate could have been stored in the Nov Volatile Memory
    // The first time the NVM is written we write NVM_CONFIG_MAGIC in the first address to indicate that the NVM contains usable data.
    while (NVM_IsBusy());   // Wait until the NVM is ready before reding
    if(EEPROM_Read(EEPROM_MAGIC_ADDR) != EEPROM_CONFIG_MAGIC)
    {
        registers->server_holding_register.addr_slave       = RTU_SERVER_ADDRESS_DEFAULT;
        registers->server_holding_register.baudrate         = RTU_BAUDRATE_DEFAULT;
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
        
        EEPROM_WriteWord(EEPROM_FREQ_MODE_ADDR, DEFAULT_FREQ_MODE);
        
        EEPROM_WriteWord(EEPROM_SAMPLES_AMOUNT_ADDR, DEFAULT_SAMPLES_AMOUNT);
        EEPROM_WriteWord(EEPROM_FREQ_STEP_ADDR, DEFAULT_FREQ_STEP);
        
        EEPROM_WriteWord(EEPROM_VOLT_AD_GAIN_ADDR, DEFAULT_FREQ_STEP);
        EEPROM_WriteWord(EEPROM_CURR_AD_GAIN_ADDR, DEFAULT_FREQ_STEP);
        EEPROM_WriteWord(EEPROM_SHUNT_RES_ADDR, DEFAULT_FREQ_STEP);
        
        EEPROM_WriteWord(EEPROM_RES_FREQ_HI_ADDR, DEFAULT_FRECUENCY_HIGH);
        EEPROM_WriteWord(EEPROM_RES_FREQ_LO_ADDR, DEFAULT_FRECUENCY_LOW);    
    }
    else
    {
        // Load values from EEPROM NVM
        registers->server_holding_register.addr_slave                   = EEPROM_ReadWord(EEPROM_ADDR_SLAVE_ADDR);
        registers->server_holding_register.baudrate                     = EEPROM_ReadWord(EEPROM_BAUDRATE_ADDR);
        
        EUSART1_SetBaudRate(registers->server_holding_register.baudrate);
        
        registers->server_input_register.sensor_type                    = EEPROM_ReadWord(SENSOR_TYPE_ADDR);
        registers->server_input_register.serial_number                  = EEPROM_ReadWord(SERIAL_NUMBER_ADDR);
        registers->server_input_register.sensor_type                    = 999;
        
        registers->server_holding_register.frequency_hi                 = EEPROM_ReadWord(EEPROM_FREQ_HI_ADDR);
        registers->server_holding_register.frequency_lo                 = EEPROM_ReadWord(EEPROM_FREQ_LO_ADDR);
        registers->server_holding_register.voltage_level                = EEPROM_ReadWord(EEPROM_VOLT_LVL_ADDR);
        
        registers->server_holding_register.on_time_ms                   = EEPROM_ReadWord(EEPROM_ON_TIME_MS_ADDR);
        registers->server_holding_register.off_time_ms                  = EEPROM_ReadWord(EEPROM_OFF_TIME_MS_ADDR);
        
        registers->server_holding_register.freq_mode                    = EEPROM_ReadWord(EEPROM_FREQ_MODE_ADDR);
        
        registers->server_holding_register.samples_amount               = EEPROM_ReadWord(EEPROM_SAMPLES_AMOUNT_ADDR);
        registers->server_holding_register.freq_step                    = EEPROM_ReadWord(EEPROM_FREQ_STEP_ADDR);
        
        registers->server_holding_register.voltage_adecuator_gain       = EEPROM_ReadWord(EEPROM_VOLT_AD_GAIN_ADDR);
        registers->server_holding_register.current_adecuator_gain       = EEPROM_ReadWord(EEPROM_CURR_AD_GAIN_ADDR);
        registers->server_holding_register.shunt_res                    = EEPROM_ReadWord(EEPROM_SHUNT_RES_ADDR);
        
        registers->server_input_register.res_freq_hi                    = EEPROM_ReadWord(EEPROM_RES_FREQ_HI_ADDR);
        registers->server_input_register.res_freq_lo                    = EEPROM_ReadWord(EEPROM_RES_FREQ_LO_ADDR);  
    }
}

void holding_register_change_handler(mod_bus_registers* modbus_data,holding_register* prev_holding_regs, nmbs_t* nmbs) // nmbs_t* nmbs 
{    
    // Check for Salve Num (RTU Address) change
    if(modbus_data->server_holding_register.addr_slave != prev_holding_regs->addr_slave)
    {
        prev_holding_regs->addr_slave = modbus_data->server_holding_register.addr_slave;                // Update previous holding register value
        EEPROM_WriteWord(EEPROM_ADDR_SLAVE_ADDR, modbus_data->server_holding_register.addr_slave);      // Update non volatile memory  
        nmbs->address_rtu = (uint8_t)modbus_data->server_holding_register.addr_slave;                   // Update modbus server slave address
    }
    
    // Check for baudrate changes 
    if(modbus_data->server_holding_register.baudrate != prev_holding_regs->baudrate)
    {
        if(EUSART1_SetBaudRate(modbus_data->server_holding_register.baudrate))              // Set UART Baudrate to the new value. This returns true on success
        {
            prev_holding_regs->baudrate = modbus_data->server_holding_register.baudrate;    // Update shadow copy with the new value
            EEPROM_WriteWord(EEPROM_BAUDRATE_ADDR, modbus_data->server_holding_register.baudrate);
        }
        else
        {
            modbus_data->server_holding_register.baudrate = prev_holding_regs->baudrate; // Baudrate invalid, revert to previous
        }
    }
    
    // Check for changes in fecuency registers
    if(modbus_data->server_holding_register.frequency_hi != prev_holding_regs->frequency_hi)
    {
        prev_holding_regs->frequency_hi = modbus_data->server_holding_register.frequency_hi;

        //modbus_data->server_input_register.res_freq_hi = new_frecuency_hi;
        EEPROM_WriteWord(EEPROM_FREQ_HI_ADDR, modbus_data->server_holding_register.frequency_hi); 
    }
    if(modbus_data->server_holding_register.frequency_lo != prev_holding_regs->frequency_lo)
    {
        prev_holding_regs->frequency_lo = modbus_data->server_holding_register.frequency_lo;
        //modbus_data->server_input_register.res_freq_lo = new_frecuency_lo;
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
    
    // Check for changes in Freq Mode
    if(modbus_data->server_holding_register.freq_mode != prev_holding_regs->freq_mode)
    {
        prev_holding_regs->freq_mode = modbus_data->server_holding_register.freq_mode;
        EEPROM_WriteWord(EEPROM_FREQ_MODE_ADDR, DEFAULT_FREQ_MODE);
    }
    
    // Check for changes in Samples ammount
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


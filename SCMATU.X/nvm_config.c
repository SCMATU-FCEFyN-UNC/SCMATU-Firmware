#include "nvm_config.h"
#include "mcc_generated_files/system/system.h"
#include <xc.h>

uint16_t EEPROM_ReadWord(uint16_t address)
{
    uint16_t value = 0;
    
    while (NVM_IsBusy());  // Wait for the memory to be ready
    value = EEPROM_Read(address);
    if(NVM_StatusGet() == NVM_ERROR)
    {
        return 4444;       // Unable to write memory, return -1
    }
    else
    {
        while (NVM_IsBusy());  // Wait for the memory to be ready
        value |= ((uint16_t)EEPROM_Read(address + 1)) << 8;
        if(NVM_StatusGet() == NVM_OK)
        {
            return value;
        }
        else
        {
            return 4444;
        }
    }
}

bool EEPROM_WriteWord(uint16_t address, uint16_t value)
{    
    NVM_UnlockKeySet(UNLOCK_KEY);
    while (NVM_IsBusy()); 
    EEPROM_Write(address, (uint8_t)(value & 0xFF));
    if(NVM_StatusGet() == NVM_ERROR)
    {
        return false;       // Unable to write memory, return false
    }
    else
    {
        NVM_UnlockKeySet(UNLOCK_KEY); 
        while (NVM_IsBusy());  // Wait for the memory to be ready
        EEPROM_Write(address + 1, (uint8_t)((value >> 8) & 0xFF));
        if(NVM_StatusGet() == NVM_OK)
        {
            return true;    // Write word to EEPROM completed, return
        }
        else
        {
           return false;       // Unable to write memory, return false 
        }
    }   
}

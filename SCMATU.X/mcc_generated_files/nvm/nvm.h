/**
 * 
 * @file nvm.h
 *
 * @defgroup nvm_driver  Non-Volatile Memory
 *
 * @brief This file contains API prototypes and other data types for the Non-Volatile Memory (NVM) driver.
 *
 * @version NVM Driver Version 4.0.1
 */

/*
© [2025] Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip 
    software and any derivatives exclusively with Microchip products. 
    You are responsible for complying with 3rd party license terms  
    applicable to your use of 3rd party software (including open source  
    software) that may accompany Microchip software. SOFTWARE IS ?AS IS.? 
    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS 
    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,  
    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY 
    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF 
    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE 
    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S 
    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT 
    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR 
    THIS SOFTWARE.
*/

#ifndef NVM_H
#define NVM_H

#include <xc.h>
#include <stdbool.h>

/**
 * @ingroup nvm_driver
 * @def EEPROM_START_ADDRESS
 * Contains the starting address of EEPROM.
 */
#define EEPROM_START_ADDRESS (0xF000U)

/**
 * @ingroup nvm_driver
 * @def EEPROM_SIZE
 * Contains the size of EEPROM in bytes.
 */
#ifndef EEPROM_SIZE
#define EEPROM_SIZE          (256U)
#endif

/**
 * @ingroup nvm_driver
 * @def UNLOCK_KEY
 * Contains the unlock key required for the NVM operations.
 */
#define UNLOCK_KEY           (0xAA55U)

/**
 * @ingroup nvm_driver
 * @brief Data type for the EEPROM data.
 */
typedef uint8_t eeprom_data_t;
/**
 * @ingroup nvm_driver
 * @brief Data type for the EEPROM address.
 */
typedef uint16_t eeprom_address_t;

/**
 * @ingroup nvm_driver
 * @enum nvm_status_t
 * @brief Contains the return codes for the NVM driver APIs.
 */
typedef enum
{
    NVM_OK, /**<0 - The NVM operation succeeded.*/
    NVM_ERROR /**<1 - The NVM operation failed.*/
} nvm_status_t;

/**
 * @ingroup nvm_driver
 * @brief Initializes the NVM driver. 
 * @param None.
 * @return None.
 */
void NVM_Initialize(void);

/**
 * @ingroup nvm_driver
 * @brief Checks if the NVM is busy with the read/write/erase commands.
 * @param None.
 * @retval True - The NVM operation is being performed.
 * @retval False - The NVM operation is not being performed.
 */
bool NVM_IsBusy(void);

/**
 * @ingroup nvm_driver
 * @brief Returns the status of the last NVM operation.
 * @param None.
 * @retval NVM_OK - The NVM operation succeeded.
 * @retval NVM_ERROR - The NVM operation failed.
 */
nvm_status_t NVM_StatusGet(void);

/**
 * @ingroup nvm_driver
 * @brief Clears the NVM error status.
 * @param None.
 * @return None.
 */
void NVM_StatusClear(void);

/**
 * @ingroup nvm_driver
 * @brief Sets the unlock keys required to perform any erase or write operations on Flash and EEPROM.
 * @param [in] unlockKey - Unlock key value as specified in the device data sheet. 
 * @return None.
 */
void NVM_UnlockKeySet(uint16_t unlockKey);

/**
 * @ingroup nvm_driver
 * @brief Clears the previously configured unlock keys.
 * @param None. 
 * @return None.
 */
void NVM_UnlockKeyClear(void);

/**
 * @ingroup nvm_driver
 * @brief Reads one byte from the given EEPROM address.
 * @param [in] address - Address of the EEPROM location to be read.
 * @return Byte read from the given EEPROM address.
 */
eeprom_data_t EEPROM_Read(eeprom_address_t address);

/**
 * @ingroup nvm_driver
 * @brief Writes one byte to the given EEPROM address. 
 *      The NVM busy status must be checked using the @ref NVM_IsBusy() API to know if the write operation is completed.
 *      Use the @ref NVM_StatusGet() API to see the result of the write operation.
 * @pre Set the unlock key using the @ref NVM_UnlockKeySet() API, if the key has been cleared before. 
 *       AoU: **Address Qualifiers** must be configured to **Require** under *Project Properties>XC8 Compiler>Optimizations*.
 * @param [in] address - Address of the EEPROM location to be written.
 * @param [in] data - Byte to be written to the given EEPROM location.
 * @return None.
 */
void EEPROM_Write(eeprom_address_t address, eeprom_data_t data);

#endif /* NVM_H */

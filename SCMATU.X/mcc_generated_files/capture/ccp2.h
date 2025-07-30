/**
 * CCP2 Generated Driver API Header File.
 * 
 * @file ccp2.h
 * 
 * @defgroup capture2 CAPTURE2
 * 
 * @brief This file contains the API prototypes and other data types for the CCP2 module.
 *
 * @version CCP2 Driver Version 2.0.2
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

#ifndef CCP2_H
#define CCP2_H

 /**
   Section: Included Files
 */

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>


/** 
   Section: Data Type Definition
*/

/**
 * @ingroup capture2
 * @union CCPR2_PERIOD_REG_T
 * @brief Custom data type to hold the low byte, high byte, and 16-bit values of the period register.
 */
typedef union CCPR2Reg_tag
{
   struct
   {
      uint8_t ccpr2l; /**< CCPR2L low byte.*/
      uint8_t ccpr2h; /**< CCPR2H high byte.*/
   };
   struct
   {
      uint16_t ccpr2_16Bit; /**< CCPR2 16-bit.*/
   };
} CCPR2_PERIOD_REG_T ;

/**
  Section: CCP2 Capture Module APIs
*/

/**
 * @ingroup capture2
 * @brief Initializes the CCP2 module. This is called only once before calling other CCP2 APIs.
 * @param None.
 * @return None.
 */
void CCP2_Initialize(void);

/**
 * @ingroup capture2
 * @brief Implements the Interrupt Service Routine (ISR) for the capture interrupt.
 * @param None.
 * @return None.
 */
void CCP2_CaptureISR(void);

/**
 * @ingroup capture2
 * @brief Assigns a callback function that will be called from the Capture ISR when a capture interrupt event occurs.
 * @param (*customCallBack)(uint16_t) - Function pointer to the new callback.
 * @return None.
 */
void CCP2_SetCallBack(void (*customCallBack)(uint16_t));

#endif // CCP2_H
/**
 End of File
*/
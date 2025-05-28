/**
 * CCP2 Generated Driver File.
 * 
 * @file ccp2.c
 * 
 * @ingroup capture2
 * 
 * @brief This file contains the API implementation for the CCP2 driver.
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

 /**
   Section: Included Files
 */

#include <xc.h>
#include "../ccp2.h"

static void (*CCP2_CallBack)(uint16_t);

/**
  Section: Capture Module APIs
*/

/**
 * @ingroup capture2
 * @brief Default callback function for the capture interrupt events.
 * @param capturedValue - 16-bit captured value.
 * @return None.
 */
static void CCP2_DefaultCallBack(uint16_t capturedValue) {
    // Add your code here
}

void CCP2_Initialize(void)
{
    // Set the CCP2 to the options selected in the User Interface

    // CCPM 4th rising edge; EN enabled; FMT right_aligned; 
    CCP2CON = 0x86;

    // CTS CCP2 pin; 
    CCP2CAP = 0x0;

    // CCPRH 0; 
    CCPR2H = 0x0;

    // CCPRL 0; 
    CCPR2L = 0x0;

    // Set the default call back function for CCP2
    CCP2_SetCallBack(CCP2_DefaultCallBack);

    // Selecting Timer 1
    CCPTMRS0bits.C2TSEL = 0x1;

    // Clear the CCP2 interrupt flag    
    PIR6bits.CCP2IF = 0;    

    // Enable the CCP2 interrupt
    PIE6bits.CCP2IE = 1;
}

void CCP2_CaptureISR(void)
{
    CCPR2_PERIOD_REG_T module;

    // Clear the CCP2 interrupt flag
    PIR6bits.CCP2IF = 0;
    
    // Copy captured value.
    module.ccpr2l = CCPR2L;
    module.ccpr2h = CCPR2H;
    
    // Return 16-bit captured value
    CCP2_CallBack(module.ccpr2_16Bit);
}

void CCP2_SetCallBack(void (*customCallBack)(uint16_t)){
    CCP2_CallBack = customCallBack;
}
/**
 End of File
*/

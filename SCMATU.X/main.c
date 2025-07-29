#include "mcc_generated_files/system/system.h"
#include <xc.h>
#include <stdio.h>  // Include the standard I/O library
#include <string.h> // Include the string library
#include "AD9833.h" // Library to control AD9833 signal generator
#include <stdbool.h> // So the bool variable type can be used

#define BUFFER_SIZE 64  // Adjust as needed for your application

// UART varaibles
char receiveBuffer[BUFFER_SIZE]; // Buffer to hold the received string
uint8_t bufferIndex = 0;         // Index to track the current position in the buffer
uint8_t dataToSend = 0, receivedData = 0, receivedData2;
int rxError = 0, rxReady = 0, rxIterator = 0;
char buffer[32]; // This buffer stores the formated message to be sent via UART

// UART Functions
void EUSART1_SendString(const char *str);

// AD9833 variables
uint32_t desiredFrequency = 150;

// CCP variables
//bool CCP1_Print = false;
uint16_t CCP1_Captured_Values[2];
uint16_t CCP1_Difference = 0;
bool iCCP1 = false;
bool print_enable = false;
/*
bool CCP2_Print = false;
uint16_t CCP2_Captured_Values[2];
uint16_t CCP2_Difference = 0;
bool iCCP2 = 0;*/

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
    
    CCP1_SetCallBack(&CCP1_Interrupt_Handler);
    CCP2_SetCallBack(&CCP2_Interrupt_Handler);

    EUSART1_SendString("SCMATU Hello, World!\r\n");
    
    // AD9833 Variables 
    desiredFrequency = 30000;
    
    AD9833Reset();
    AD9833SetRegisterValue(AD9833_OUT_SINUS);
    AD9833SetFrequency(AD9833_REG_FREQ0, desiredFrequency);
    AD9833SetRegisterValue(AD9833_REG_CMD); // Clears RESET, enabling output
    
    TMR0_PeriodMatchCallbackRegister(&TMR0_Interrupt_Handler);
    
    __delay_ms(50);
    PIE6bits.CCP1IE = 1; // Enable the CCP1 interrupt
    
    while(1)
    {
    }    
}

// ---------------------- UART Functions ----------------------
void EUSART1_SendString(const char *str) {
    while(*str != '\0') {          // Loop until the end of the string
        while(!EUSART1_IsTxReady()); // Wait until the transmitter is ready
        EUSART1_Write(*str);       // Send the character
        str++;                     // Move to the next character
    }
}

void CCP1_Interrupt_Handler(uint16_t value) 
{
    if(iCCP1 == false)
    {
        TEST_SetHigh();
        CCP1_Captured_Values[0] = value;
    }
    iCCP1 = !iCCP1;
    PIE6bits.CCP1IE = 0;
    PIE6bits.CCP2IE = 1;  
 }

void CCP2_Interrupt_Handler(uint16_t value) {
    if(iCCP1 == true)
    {
        TEST_SetLow();
        CCP1_Captured_Values[1] = value;
        CCP1_Difference = CCP1_Captured_Values[1] - CCP1_Captured_Values[0];
        print_enable = true;
    } 
    PIE6bits.CCP2IE = 0;
    PIE6bits.CCP1IE = 1;  
 }

void TMR0_Interrupt_Handler()
{
    if(CCP1_Difference > 0 && print_enable) 
    {
        sprintf(buffer, "CCP First Capture: %u\r\n", CCP1_Captured_Values[0]);
        EUSART1_SendString(buffer);
        sprintf(buffer, "CCP Second Capture: %u\r\n", CCP1_Captured_Values[1]);
        EUSART1_SendString(buffer);
        sprintf(buffer, "CCP Difference: %u\r\n", CCP1_Difference);
        EUSART1_SendString(buffer);
        print_enable = false;
    }
}


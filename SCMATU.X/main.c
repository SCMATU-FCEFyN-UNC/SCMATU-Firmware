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
void UART_Receive();

uint32_t desiredFrequency = 150;

// CCP ISR
void MyCaptureHandler(uint16_t value);
bool ccp1_print = false;
bool CCP_ENABLE = false;
uint16_t firstCapture = 0, secondCapture = 0, CCP_Difference = 0;
uint16_t capturedValues[2];
uint8_t iCCP = 0;

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
    
    CCP1_SetCallBack(&MyCaptureHandler);
    
    EUSART1_SendString("SCMATU Hello, World!\r\n");
    
    // AD9833 Variables 
    desiredFrequency = 140000;
    
    AD9833Reset();
    AD9833SetRegisterValue(AD9833_OUT_SINUS);
    AD9833SetFrequency(AD9833_REG_FREQ0, desiredFrequency);
    AD9833SetRegisterValue(AD9833_REG_CMD); // Clears RESET, enabling output
    
    // Disable the CCP1 interrupt
    PIE6bits.CCP1IE = 0;
    

    while(1)
    {
        UART_Receive();
        if (ccp1_print && CCP_Difference > 0) {
            sprintf(buffer, "CCP First Capture: %u\r\n", capturedValues[0]);
            EUSART1_SendString(buffer);
            sprintf(buffer, "CCP Second Capture: %u\r\n", capturedValues[1]);
            EUSART1_SendString(buffer);
            sprintf(buffer, "CCP Difference: %u\r\n", CCP_Difference);
            EUSART1_SendString(buffer);
            ccp1_print = false;
        }
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

void UART_Receive() {
    
    if (EUSART1_IsRxReady())
    {
        if(rxIterator < BUFFER_SIZE - 1)
        {
            receiveBuffer[rxIterator] = EUSART1_Read();
            if(receiveBuffer[rxIterator] == '\n'){
                receiveBuffer[rxIterator] = '\0';  // Null-terminate the string  (remove endline)
                if (strcmp(receiveBuffer, "CCP_Enable") == 0) //if (strncmp(receiveBuffer, "CCP_Enable", 10) == 0)
                {
                    sprintf(buffer, "CCP enabled\r\n"); // This is not happening
                    EUSART1_SendString(buffer);
                    iCCP = 0;
                    capturedValues[0] = 0;
                    capturedValues[1] = 0; 
                    CCP_Difference = 0;
                    PIE6bits.CCP1IE = 1;  // Enable CCP interrupt
                    CCP_ENABLE = true;  
                }
                else 
                {
                    sprintf(buffer, "Recibido: %s\r\n",receiveBuffer);
                    EUSART1_SendString(buffer);
                    desiredFrequency = (uint32_t)atoi(receiveBuffer);
                    AD9833SetFrequency(AD9833_REG_FREQ0, desiredFrequency);
                    sprintf(buffer, "Frec set to: %ld\r\n",desiredFrequency);
                    EUSART1_SendString(buffer);
                }
                rxIterator = 0;
                memset(receiveBuffer, 0, BUFFER_SIZE); // Clear the buffer
            }
            else {rxIterator++;}
        }
        else 
        {
           sprintf(buffer, "\r\nExceeded Buffer Size\r\n");
           EUSART1_SendString(buffer);
           memset(receiveBuffer, 0, BUFFER_SIZE); // Clear the buffer
           rxIterator = 0;
        }
    }
}

void MyCaptureHandler(uint16_t value) {
    capturedValues[iCCP] = value;
    
    if(iCCP == 1)
    {
        CCP_Difference = capturedValues[1] - capturedValues[0];
        PIE6bits.CCP1IE = 0; // Disable the CCP1 interrupt
        ccp1_print = true;
    }     
    iCCP ^= 1;
 }
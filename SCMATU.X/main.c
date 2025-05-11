#include "mcc_generated_files/system/system.h"
#include <xc.h>
#include <stdio.h>  // Include the standard I/O library
#include <string.h> // Include the string library
#include "AD9833.h" // Library to control AD9833 signal generator

#define BUFFER_SIZE 64  // Adjust as needed for your application

// UART varaibles
char receiveBuffer[BUFFER_SIZE]; // Buffer to hold the received string
uint8_t bufferIndex = 0;         // Index to track the current position in the buffer
uint8_t dataToSend = 0, receivedData = 0, receivedData2;
int rxError = 0, rxReady = 0, rxIterator = 0;
char buffer[32]; // Buffer para almacenar el mensaje formateado

// UART Functions
void EUSART1_SendString(const char *str);
void UART_Receive();

uint32_t desiredFrequency = 150;

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

    EUSART1_SendString("SCMATU Hello, World!\r\n");
    
    // AD9833 Variables 
    desiredFrequency = 140000;
    
    AD9833Reset();
    AD9833SetRegisterValue(AD9833_OUT_SINUS);
    AD9833SetFrequency(AD9833_REG_FREQ0, desiredFrequency);
    AD9833SetRegisterValue(AD9833_REG_CMD); // Clears RESET, enabling output

    while(1)
    {
        UART_Receive();
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
                sprintf(buffer, "Recibido: %s\r\n",receiveBuffer);
                EUSART1_SendString(buffer);
                desiredFrequency = (uint32_t)atoi(receiveBuffer);
                AD9833SetFrequency(AD9833_REG_FREQ0, desiredFrequency);
                sprintf(buffer, "Frec set to: %ld\r\n",desiredFrequency);
                EUSART1_SendString(buffer);
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
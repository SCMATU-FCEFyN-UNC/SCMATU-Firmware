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
void CCP1_Interrupt_Handler(uint16_t value);
bool CCP1_Print = false;
uint16_t CCP1_Captured_Values[2];
uint16_t CCP1_Difference = 0;
uint8_t iCCP1 = 0;

void CCP2_Interrupt_Handler(uint16_t value);
bool CCP2_Print = false;
uint16_t CCP2_Captured_Values[2];
uint16_t CCP2_Difference = 0;
uint8_t iCCP2 = 0;

bool CCP_Print = false;
uint16_t CCP_Captured_Values[2];
uint16_t CCP_Difference = 0;
bool CCP_Capture_Complete = false;

uint16_t timer_init_value = 0;

uint8_t num_first_ints = 0;
uint8_t num_second_ints = 0;

bool previous_overflow = false;

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
    
    CCP1_SetCallBack(&CCP1_Interrupt_Handler);
    CCP2_SetCallBack(&CCP2_Interrupt_Handler);
    
    EUSART1_SendString("SCMATU Hello, World!\r\n");
    
    // AD9833 Variables 
    desiredFrequency = 140000;
    
    AD9833Reset();
    AD9833SetRegisterValue(AD9833_OUT_SINUS);
    AD9833SetFrequency(AD9833_REG_FREQ0, desiredFrequency);
    AD9833SetRegisterValue(AD9833_REG_CMD); // Clears RESET, enabling output
    
    // Disable the CCP1 interrupt
    PIE6bits.CCP1IE = 0;
    // Disable the CCP2 interrupt
    PIE6bits.CCP2IE = 0;
    
    //TMR1_Reload();
    //TMR1_Stop();
    timer_init_value = TMR1_Read();
    sprintf(buffer, "Timer init value: %u\r\n", timer_init_value);
    EUSART1_SendString(buffer);
    
    while(1)
    {
        UART_Receive();
        /*if (CCP1_Print && CCP1_Difference > 0) {
            sprintf(buffer, "CCP First Capture: %u\r\n", CCP1_Captured_Values[0]);
            EUSART1_SendString(buffer);
            sprintf(buffer, "CCP Second Capture: %u\r\n", CCP1_Captured_Values[1]);
            EUSART1_SendString(buffer);
            sprintf(buffer, "CCP Difference: %u\r\n", CCP1_Difference);
            EUSART1_SendString(buffer);
            CCP1_Print = false;
        }
        if (CCP2_Print && CCP2_Difference > 0) {
            sprintf(buffer, "CCP2 First Capture: %u\r\n", CCP2_Captured_Values[0]);
            EUSART1_SendString(buffer);
            sprintf(buffer, "CCP2 Second Capture: %u\r\n", CCP2_Captured_Values[1]);
            EUSART1_SendString(buffer);
            sprintf(buffer, "CCP2 Difference: %u\r\n", CCP2_Difference);
            EUSART1_SendString(buffer);
            CCP2_Print = false;
        }*/
        if (CCP_Print && CCP_Capture_Complete)
        {
            if(CCP_Captured_Values[1] > CCP_Captured_Values[0])
            {
                //if(!previous_overflow)
                //{
                    CCP_Difference = CCP_Captured_Values[1] - CCP_Captured_Values[0];
                //}
                //else 
               //{
                    CCP_Difference = CCP_Captured_Values[0] - CCP_Captured_Values[1];
                    //previous_overflow = false;
                //}
            }
            else 
            {
                EUSART1_SendString("Overflow Detected\n");
                //previous_overflow = true;
                //CCP_Difference = CCP_Captured_Values[0] - CCP_Captured_Values[1];
                //CCP_Difference = (65535 - CCP_Captured_Values[0]) + CCP_Captured_Values[1] + 1;
                CCP_Difference = (65535 - CCP_Captured_Values[0]) + CCP_Captured_Values[1] + 1;
            }
            //sprintf(buffer, "Num First Capture ints : %u\r\n", num_first_ints);
            //EUSART1_SendString(buffer);
            //sprintf(buffer, "Num Second Capture ints: %u\r\n", num_second_ints);
            //EUSART1_SendString(buffer);
            sprintf(buffer, "CCP First Capture: %u\r\n", CCP_Captured_Values[0]);
            EUSART1_SendString(buffer);
            sprintf(buffer, "CCP Second Capture: %u\r\n", CCP_Captured_Values[1]);
            EUSART1_SendString(buffer);
            sprintf(buffer, "CCP Difference: %u\r\n", CCP_Difference);
            EUSART1_SendString(buffer);
            CCP_Print = false;
            CCP_Capture_Complete = false;
            CCP_Captured_Values[0] = 0;
            CCP_Captured_Values[1] = 0;
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
                    /*iCCP1 = 0;
                    CCP1_Captured_Values[0] = 0;
                    CCP1_Captured_Values[1] = 0; 
                    CCP1_Difference = 0;
                    PIE6bits.CCP1IE = 1;  // Enable CCP1 interrupt
                    iCCP2 = 0;
                    CCP2_Captured_Values[0] = 0;
                    CCP2_Captured_Values[1] = 0; 
                    CCP2_Difference = 0;
                    PIE6bits.CCP2IE = 1;  // Enable CCP2 interrupt*/
                    CCP_Captured_Values[0] = 0;
                    CCP_Captured_Values[1] = 0; 
                    CCP_Difference = 0;
                    //TMR1_Reload();
                    //timer_init_value = TMR1_Read();
                    //sprintf(buffer, "Timer init value: %u\r\n", timer_init_value);
                    //EUSART1_SendString(buffer);
                    //TMR1_Start();
                    PIE6bits.CCP1IE = 1;  // Enable CCP1 interrupt
                    //PIE6bits.CCP2IE = 1;  // Enable CCP2 interrupt
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

void CCP1_Interrupt_Handler(uint16_t value) {
    /*CCP1_Captured_Values[iCCP1] = value;
    
    if(iCCP1 == 1)
    {
        CCP1_Difference = CCP1_Captured_Values[1] - CCP1_Captured_Values[0];
        PIE6bits.CCP1IE = 0; // Disable the CCP1 interrupt
        CCP1_Print = true;
    }     
    iCCP1 ^= 1;*/
    CCP_Captured_Values[0] = value;
    PIE6bits.CCP1IE = 0; // Disable the CCP1 interrupt
    PIE6bits.CCP2IE = 1;  // Enable CCP2 interrupt
    num_first_ints++;
 }

void CCP2_Interrupt_Handler(uint16_t value) {
    /*CCP2_Captured_Values[iCCP2] = value;
    
    if(iCCP2 == 1)
    {
        CCP2_Difference = CCP2_Captured_Values[1] - CCP2_Captured_Values[0];
        PIE6bits.CCP2IE = 0; // Disable the CCP2 interrupt
        CCP2_Print = true;
    }     
    iCCP2 ^= 1;*/
    CCP_Captured_Values[1] = value;
    //TMR1_Reload();
    //TMR1_Stop();
    PIE6bits.CCP2IE = 0; // Disable the CCP2 interrupt
    CCP_Capture_Complete = true;
    //TMR1_Reload();
    num_second_ints++;
    CCP_Print = true;
 }
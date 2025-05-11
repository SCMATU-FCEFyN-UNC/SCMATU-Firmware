#include "AD9833.h"
#include "mcc_generated_files/system/system.h" // SPI
#include <stdio.h>  // For sprintf

float FREC_MULT = 10.73741824f; //uint32_t FREC_MULT = (268435456 / 25000000);

// ---------------------- AD9833 Functions ----------------------
void AD9833SetRegisterValue(uint16_t regValue) {
    uint8_t data[2];
    data[0] = (uint8_t)((regValue & 0xFF00) >> 8);
    data[1] = (uint8_t)(regValue & 0x00FF);

    if(SPI1_Open(0))
    {
        IO_RB6_SetLow(); // SS Low
        SPI1_BufferWrite(&data[0], 1);
        SPI1_BufferWrite(&data[1], 1);
        IO_RB6_SetHigh(); // SS High
        SPI1_Close(); 
    }
}

void AD9833SetFrequency(uint16_t reg, uint32_t freq) {
    uint32_t freqWord = (uint32_t)(freq * FREC_MULT);
    uint16_t freqHi = ((freqWord >> 14) & 0x3FFF) | 0x4000;
    uint16_t freqLo = (freqWord & 0x3FFF) | 0x4000;
    uint32_t FREQREG = ((uint32_t)freqHi << 16) | freqLo;
    
    AD9833SetRegisterValue(AD9833_B28);
    AD9833SetRegisterValue(freqLo);
    AD9833SetRegisterValue(freqHi);
}

void AD9833Reset(void)
{
    AD9833SetRegisterValue(AD9833_REG_CMD | AD9833_RESET);
}
// END ---------------------- AD9833 Functions ----------------------
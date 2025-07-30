#include "mcc_generated_files/system/system.h"
#include <xc.h>

bool EUSART1_SetBaudRate(uint16_t baudrate)
{
    // Assuming Fosc = 16MHz and BRGH = 1 (high-speed)
    // BRG = (Fosc / (4 * baudrate)) - 1
    uint32_t brg = (_XTAL_FREQ / (4UL * baudrate)) - 1;

    if (brg > 0xFFFF)
        return false; // Out of range for 16-bit BRG

    SP1BRGH = (brg >> 8) & 0xFF;
    SP1BRGL = brg & 0xFF;

    return true;
}
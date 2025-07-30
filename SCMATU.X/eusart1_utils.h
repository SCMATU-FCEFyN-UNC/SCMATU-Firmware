#ifndef EUSART1_UTILS_H
#define EUSART1_UTILS_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @ingroup eusart1
 * @brief Sets baudrate to a custom value
 * @param baudrate.
 * @retval true on success
 * @retval false if the chosen value is invalid
 */
bool EUSART1_SetBaudRate(uint16_t baudrate);

#endif

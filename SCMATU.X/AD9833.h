/* 
 * File:   AD9833.h
 * Author: Lucas
 *
 * Created on 11 de mayo de 2025, 17:56
 */

#ifndef AD9833_H
#define	AD9833_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>  // For uint16_t, uint32_t types

/******************************************************************************/
/* AD9833 Register commands                                                   */
/******************************************************************************/
#define AD9833_REG_CMD    (0 << 14)
#define AD9833_REG_FREQ0  (1 << 14)
#define AD9833_REG_FREQ1  (2 << 14)
#define AD9833_REG_PHASE0 (6 << 13)
#define AD9833_REG_PHASE1 (7 << 13)

/* Command Control Bits */
#define AD9833_B28        (1 << 13)
#define AD9833_RESET      (1 << 8)

/* Setup configuration commands */
#define AD9833_OUT_SINUS  ((0 << 5) | (0 << 1) | (0 << 3))
#define AD9833_OUT_TRIANGLE ((0 << 5) | (1 << 1) | (0 << 3))

/* Function Declarations */
void AD9833SetRegisterValue(uint16_t regValue);
void AD9833SetFrequency(uint16_t reg, uint32_t freq);
void AD9833Reset(void);


#ifdef	__cplusplus
}
#endif

#endif	/* AD9833_H */


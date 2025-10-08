/*
 * File:   AD9833.h
 * Author: Lucas
 *
 * Created on 11 de mayo de 2025, 17:56
 */

#ifndef ROBUST_MEASUREMENT_H
#define ROBUST_MEASUREMENT_H

#ifdef __cplusplus
extern "C"
{
#endif
    
#include <stdint.h>  // For uint16_t, uint32_t types

#define MAX_SAMPLES 30
#define TICKS_NS 125
#define TOLERANCE_TICKS 10 
    
uint16_t calculate_median(uint16_t arr[], uint16_t temp[], uint16_t n);
uint16_t abs_diff(uint16_t a, uint16_t b);
uint16_t robust_average_ns(uint16_t arr[], uint16_t median, uint16_t num_samples, uint16_t tolerance);
uint16_t robust_average(uint16_t arr[], uint16_t median, uint16_t num_samples, uint16_t tolerance_ticks);
float calculate_phase_ns(uint16_t diff, float frequency, float tick_ns);

#ifdef __cplusplus
}
#endif

#endif /* ROBUST_MEASUREMENT_H */

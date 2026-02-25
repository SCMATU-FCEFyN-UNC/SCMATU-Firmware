#include "robust_measurement.h"
#include "mcc_generated_files/system/system.h" // SPI
#include <stdio.h>                             // For sprintf

// Median calculation function
uint16_t calculate_median(uint16_t arr[], uint16_t temp[], uint16_t n)
{   
    // Copy array to avoid modifying original
    for (uint16_t i = 0; i < n; i++) 
    {
        temp[i] = arr[i];
    }
    
    // Sort the array (bubble sort for simplicity)
    for (uint16_t i = 0; i < n - 1; i++) 
    {
        for (uint16_t j = 0; j < n - i - 1; j++) 
        {
            if (temp[j] > temp[j + 1]) 
            {
                uint16_t swap = temp[j];
                temp[j] = temp[j + 1];
                temp[j + 1] = swap;
            }
        }
    }
    
    // Return median
    if (n % 2 == 0) 
    {
        // Even number of elements - average middle two
        return (temp[n/2 - 1] + temp[n/2]) / 2;
    }
    else 
    {
        // Odd number of elements
        return temp[n/2];
    }
}

// Helper function to calculate absolute difference (avoiding stdlib dependency)
uint16_t abs_diff(uint16_t a, uint16_t b) {
    if (a > b) {
        return a - b;
    } else {
        return b - a;
    }
}

uint16_t robust_average_ns(uint16_t arr[], uint16_t median, uint16_t num_samples, uint16_t tolerance) 
{
    int valid_samples = 0;
    uint32_t sum = 0;
    
    // Calculate sum of valid samples (within tolerance of median)
    for (uint16_t i = 0; i < num_samples; i++) {
        if (abs_diff(arr[i], median) <= tolerance) {
            sum += arr[i];
            valid_samples++;
        }
    }
    
    if(valid_samples != 0)
    {
        return (uint16_t)(sum / valid_samples);
    }
    else
    {
        return -1;
    } 
}

float calculate_phase_ns(uint16_t diff, float frequency, float tick_ns) {
    // measured time in ns (rise1 - fall2)
    float measured_time_ns = (float)diff * tick_ns;
    
    // signal period in ns
    float period_ns = 1e9 / frequency;
    
    // adjust: fall2 is T/2 after rise2
    float phase_ns = measured_time_ns - (period_ns / 2.0);
    
    // wrap into [-T/2, +T/2)
    if (phase_ns > (period_ns / 2.0)) {
        phase_ns -= period_ns;
    } else if (phase_ns < -(period_ns / 2.0)) {
        phase_ns += period_ns;
    }
    
    return phase_ns;
}

// ---- Robust average (returns ticks) ----
uint16_t robust_average(uint16_t arr[], uint16_t median, uint16_t num_samples, uint16_t tolerance_ticks)
{
    uint32_t sum = 0;
    uint16_t valid = 0;

    for (uint16_t i = 0; i < num_samples; i++) {
        if (abs_diff(arr[i], median) <= tolerance_ticks) {
            sum += arr[i];
            valid++;
        }
    }

    return (valid > 0) ? (uint16_t)(sum / valid) : 0;
}
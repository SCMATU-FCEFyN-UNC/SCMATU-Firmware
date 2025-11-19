#include "mcc_generated_files/system/system.h"
#include <stdbool.h> 
#include "closed_loop_control.h"

extern bool closed_loop_timer_enabled;

void disable_closed_loop_timer()
{
    TMR0_Stop();
    TMR0_TMRInterruptDisable();
    PIR0bits.TMR0IF = 0;
    closed_loop_timer_enabled = false;
}

void enable_closed_loop_timer()
{
    TMR0_Reload();
    TMR0_TMRInterruptEnable();
    TMR0_Start();
    closed_loop_timer_enabled = true;
}


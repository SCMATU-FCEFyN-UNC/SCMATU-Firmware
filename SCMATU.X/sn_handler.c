#include "mcc_generated_files/system/system.h"
#include "sn_handler.h"
#include "nvm_config.h"
#include "closed_loop_control.h"

// -----------------------------------------------------------------------------
// Internal state (module-private)
// -----------------------------------------------------------------------------

static bool sn_write_enabled = false;
static uint8_t sn_enabled_countdown = 0;
static bool sn_write_happened = false;

extern bool sampling_active;
extern bool resonance_auto_detection_running;
extern bool closed_loop_enabled;
extern bool closed_loop_timer_enabled;
extern uint16_t closed_loop_counter;
extern volatile bool closed_loop_trigger_flag;

// -----------------------------------------------------------------------------
// TMR0 periodic handler
// -----------------------------------------------------------------------------

void sn_write_handler(uint16_t *status_reg)
{
    if (sn_write_enabled)
    {
        sn_enabled_countdown++;

        if (sn_enabled_countdown >= SN_WRITE_TIMEOUT)
        {
            // Timeout reached, then disable write
            sn_enabled_countdown = 0;
            sn_write_enabled = false;
            
            // If no write occurred during the window ? return to IDLE
            if (!sn_write_happened)
            {
                *status_reg = SNW_STATUS_IDLE;
            }
            
            // Reset flag for next cycle
            sn_write_happened = false;
        }
    }
}

// -----------------------------------------------------------------------------
// Handle password submission
// -----------------------------------------------------------------------------

void sn_submit_password(uint16_t password, uint16_t *status_reg)
{
    if((resonance_auto_detection_running)||(sampling_active))
    {
        *status_reg = SNW_STATUS_NOT_AVAILABLE;
        return;
    }
    else
    {
        if (!sn_write_enabled)
        {
            if (password == SN_PASSWORD_CORRECT)
            {
                sn_enabled_countdown = 0; // Reset timer
                sn_write_happened = false;

                // Disable closed loop control to avoid conflicts
                closed_loop_enabled = false;
                closed_loop_trigger_flag = false;
                closed_loop_counter = 0;

                enable_closed_loop_timer(); 

                sn_write_enabled = true;           
            }
            else
            {
                *status_reg = SNW_STATUS_WRONG_PASS;
                sn_write_enabled = false;
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Handle serial number write attempt
// Returns true if write was allowed
// -----------------------------------------------------------------------------

bool sn_attempt_write(uint16_t serial_in, mod_bus_registers* modbus_data)
{
    if (!sn_write_enabled)
    {
        // User attempted write without valid password
        if (modbus_data->server_holding_register.sn_write_status != SNW_STATUS_WRONG_PASS)
            modbus_data->server_holding_register.sn_write_status = SNW_STATUS_NOT_AUTHORIZED;

        return false;
    }
    
    // Register that the input register serial_number has been updated
    sn_write_happened = true;
     
    // Disable future writes until a new password is given
    sn_write_enabled = false;
    sn_enabled_countdown = 0;
     
    // Write to EEPROM (optional — remove if you handle EEPROM outside)
    EEPROM_WriteWord(SERIAL_NUMBER_ADDR, serial_in);

    return true;
}

// -----------------------------------------------------------------------------
// Getter
// -----------------------------------------------------------------------------

bool sn_is_write_enabled(void)
{
    return sn_write_enabled;
}
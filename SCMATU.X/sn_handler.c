#include "sn_handler.h"
#include "nvm_config.h"
#include "mcc_generated_files/system/system.h"

// -----------------------------------------------------------------------------
// Internal state (module-private)
// -----------------------------------------------------------------------------

static bool sn_write_enabled = false;
static uint8_t sn_enabled_countdown = 0;
static bool sn_write_happened = false;

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
    if (!sn_write_enabled)
    {
        if (password == SN_PASSWORD_CORRECT)
        {
            sn_enabled_countdown = 0; // Reset timer
            sn_write_happened = false;
            sn_write_enabled = true;           
        }
        else
        {
            *status_reg = SNW_STATUS_WRONG_PASS;
            sn_write_enabled = false;
        }
    }
}

// -----------------------------------------------------------------------------
// Handle serial number write attempt
// Returns true if write was allowed
// -----------------------------------------------------------------------------

bool sn_attempt_write(uint16_t serial_in, uint16_t *status_reg)
{
    if (!sn_write_enabled)
    {
        // User attempted write without valid password
        if (*status_reg != SNW_STATUS_WRONG_PASS)
            *status_reg = SNW_STATUS_NOT_AUTHORIZED;

        return false;
    }
    
    // Serial number write authorized
    *status_reg = SNW_STATUS_SUCCESS;
    
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
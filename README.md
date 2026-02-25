# SCMATU-Firmware

Firmware for the **SCMATU – Ultrasonic Transducer Amplifier Control and Measurement System**.

Developed for the **PIC16F18446** microcontroller.

This firmware controls and monitors a PZT ultrasonic transducer amplifier via **Modbus RTU**, providing:

- Measuring phase between voltage and current signals in ns
- ADC-based voltage and current acquisition
- Resonance frequency auto-detection via frequency sweeps
- External resonance injection (from PC software)
- Periodic frequency sweeps for resonance tracking
- Secure serial number programming
- EEPROM-backed configuration persistence

---

## 📥 Download Latest Ready-to-Program Files

[🔗 Latest HEX](https://github.com/SCMATU-FCEFyN-UNC/SCMATU-Firmware/releases/latest/download/SCMATU.X.production.hex)  
[🔗 Latest ELF](https://github.com/SCMATU-FCEFyN-UNC/SCMATU-Firmware/releases/latest/download/SCMATU.X.production.elf)

- **HEX** → Ready to program with MPLAB IPE
- **ELF** → For debugging in MPLAB X

---

## 🛠 Development Environment

To build this firmware:

- MPLAB X IDE v6.20
- XC8 Compiler v2.46
- MCC (generate code before building)

Target MCU: **PIC16F18446**

---

## ⚙ System Overview

The firmware performs:

### 1️⃣ Frequency Generation
- Controls an AD9833 DDS module
- Frequency stored as 32-bit value (HI/LO registers)
- Supports dynamic frequency updates

### 2️⃣ Transducer Control
- Enable/disable via Coil 0
- ON/OFF pulsed operation using configurable:
  - `on_time_ms`
  - `off_time_ms`
- Adjustable output amplitude via `voltage_level`

---

### 3️⃣ Measurement Capabilities

#### Phase Measurement
- Phase difference between voltage and current
- Timer-based measurement (TMR1 ticks)
- Configurable sample averaging

#### ADC Measurements
- Peak voltage
- Peak current
- Configurable ADC averaging
- Calibration using:
  - Voltage gain
  - Current gain
  - Shunt resistor value

---

### 4️⃣ Automatic Resonance Detection

Triggered via **Coil 5**.

The firmware:

1. Sweeps frequency from:
   ```
   freq_range_start → freq_range_end
   ```
2. Steps using:
   ```
   freq_step
   ```
3. Tracks:
   - Minimum phase frequency
   - Maximum current frequency
4. Applies validation:
   - `phase_curr_max_distance` (avoids anti-resonance)
5. Selects best combined resonance

Results are stored in input registers:
- `res_freq_hi`
- `res_freq_lo`
- `res_freq_phase`
- `res_freq_curr`

Status in:
- `res_freq_status`

Status values:
```
0 – Not obtained
1 – Obtained
2 – Failed
3 – In progress
4 – Obtained by Software
```

---

### 5️⃣ External Resonance Injection

Triggered via **Coil 6**.

PC software can write:

- `external_res_freq_hi`
- `external_res_freq_lo`

Firmware:
- Updates output frequency
- Updates resonance registers
- Sets status = 4

---

### 6️⃣ Periodic Frequency Sweeps - Resonance Tracking

When enabled:

- `closed_loop_control_enable = 1`
- Period defined in `closed_loop_control_period` (seconds)

Firmware periodically:
- Performs sweep around current resonance
- Uses:
  ```
  auto_freq_sweep_width
  ```
- Internally interpreted as **total width**
- Sweep range:
  ```
  resonance ± (auto_freq_sweep_width / 2)
  ```
- Lower bound clamped to 0 to prevent underflow

---

### 7️⃣ Secure Serial Number Programming

To write serial number:

1. Write password to `sn_password`
2. Within 15 seconds:
   - Write serial to `serial_number_in`

Status available in:
- `sn_write_status`

Status values:
```
0 – Idle
1 – Success
2 – Wrong password
3 – Not authorized
4 – Not available
```

---

# 🔘 Modbus Coils (0x01 / 0x05)

| Coil | Function |
|------|----------|
| 0 | Enable/Disable transducer |
| 1 | Measure All (Phase + Power) |
| 2 | Measure Power |
| 3 | Measure Phase |
| 4 | Update Output Frequency |
| 5 | Auto-Detect Resonance |
| 6 | Insert External Resonance |

---

# 📦 Holding Registers (0x03 / 0x06)

Base Address: 40000+

| HR | Name | Description |
|----|------|------------|
| 0 | addr_slave | Modbus slave address |
| 1 | baudrate | RTU baudrate |
| 2–3 | frequency_hi/lo | Current/Desired frequency |
| 4 | voltage_level | Output amplitude (%) |
| 5–6 | on_time/off_time | Pulse control timing |
| 7 | samples_amount | Phase averaging samples |
| 8 | freq_step | Sweep step |
| 9–12 | freq_range_start/end | Sweep range |
| 13 | voltage_adecuator_gain | Voltage calibration |
| 14 | current_adecuator_gain | Current calibration |
| 15 | shunt_res | Current shunt (Ohm) |
| 16 | adc_samples_amount | ADC averaging |
| 17 | phase_curr_max_distance | Anti-resonance filter |
| 18 | auto_freq_sweep_width | Total sweep width |
| 19 | closed_loop_control_enable | Enable periodic sweep |
| 20 | closed_loop_control_period | Period (seconds) |
| 21–22 | external_res_freq_hi/lo | External resonance input |
| 23 | serial_number_in | Serial write register |
| 24 | sn_password | Unlock serial write |
| 25 | sn_write_status | Write result |

---

# 📊 Input Registers (0x04)

Base Address: 30000+

| IR | Name | Description |
|----|------|------------|
| 0 | sensor_type | Device type |
| 1 | serial_number | Board serial |
| 2 | phase_difference | Phase measurement |
| 3 | phase_ready | Phase ready flag |
| 4–5 | ADC_peak_voltage/current | Peak values |
| 6–7 | volt/curr_ready | ADC ready flags |
| 8 | internal_measurement_ready | Internal flag |
| 9 | res_freq_status | Resonance status |
| 10–11 | res_freq_hi/lo | Resonance frequency |
| 12–13 | res_freq_phase/curr | Measurements at resonance |
| 14–17 | best_freq_phase data | Minimum phase freq |
| 18–21 | best_freq_curr data | Maximum current freq |
| 22–24 | test registers | Debug |
| 25 | system_status | System state |
| 26 | last_error | Last error code |


---

# 💾 EEPROM Persistence

The following are stored in EEPROM:

- Slave address
- Calibration values
- Sweep configuration
- Closed-loop parameters
- Serial number

---

# ⚠ Safety Notes

- Ensure frequency range is within amplifier and PZT safe limits.
- Avoid excessive sweep widths at low resonance frequencies.

---


## 📚 Documentation

Detailed system documentation is available in:

- [Firmware Requirements Specification](./docs/Firmware-Requirements-Specification.md)

# LICENSE

This project is licensed under the **GNU General Public License v3.0**.  
See the [LICENSE](./LICENSE) file for details.
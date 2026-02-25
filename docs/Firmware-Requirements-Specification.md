# Firmware Requirements Specification (FRS)
## SCMATU – Ultrasonic Transducer Control Firmware

**Project:** SCMATU – Sistema de Control y Medición de Amplificador para Transductor Ultrasónico  
**Target MCU:** PIC16F18446  

---

# 1. Introduction

## 1.1 Purpose

This document defines the functional requirements of the SCMATU firmware responsible for:

- Exciting a PZT ultrasonic transducer (or known RLC circuit),
- Measuring electrical parameters,
- Automatically determining the resonance frequency.

The firmware operates as the embedded control layer of the SCMATU system and communicates with a supervisory PC application via Modbus RTU.

---

## 1.2 System Objective

The system shall automatically determine the resonance frequency of a PZT transducer (or an RLC circuit with known resonance), defined as:

> The frequency at which the current is maximum and in phase with the applied voltage.

---

# 2. System Functional Requirements

---

## FR-01: Sinusoidal Excitation

The firmware shall be capable of exciting the circuit under test with a sinusoidal signal of:

- Variable frequency within a configurable range.
- Configurable amplitude.

### FR-01.1 Frequency Control
The firmware shall allow frequency adjustment with sufficient resolution to perform resonance sweeps.

### FR-01.2 Amplitude Control
The firmware shall support excitation amplitudes compatible with ±10 V output (designed for operational amplifiers powered at ±12 to ±15 V).

---

## FR-02: Voltage Measurement

The firmware shall measure the peak value of the voltage applied to the device under test.

### FR-02.1 Measurement Type
Voltage measurement shall be performed using the ADC peripheral.

### FR-02.2 Averaging
The firmware shall support configurable sample averaging to improve measurement stability.

---

## FR-03: Current Measurement

The firmware shall measure the peak value of the current flowing through the device under test.

### FR-03.1 Measurement Method
Current shall be measured indirectly via shunt resistor and amplification stage.

### FR-03.2 ADC Acquisition
The firmware shall use ADC sampling to determine the peak current value.

### FR-03.3 Calibration
The firmware shall allow calibration through:
- Current gain parameter
- Shunt resistor value parameter

---

## FR-04: Phase Difference Measurement

The firmware shall determine the phase difference between voltage and current signals.

### FR-04.1 Measurement Principle
Phase shall be calculated based on timing difference between voltage and current zero crossings or equivalent time-domain measurement technique.

### FR-04.2 Precision
The firmware shall determine phase difference with a precision defined by:
- Timer resolution
- Sampling frequency
- Configurable number of samples

### FR-04.3 Averaging
The firmware shall support configurable averaging to improve phase measurement stability.

---

## FR-05: Resonance Frequency Auto-Detection

The firmware shall automatically determine the resonance frequency of the circuit under test.

### FR-05.1 Sweep Operation
The firmware shall:

- Sweep frequency within a configurable range.
- Increment frequency by a configurable step.
- Measure phase and current at each frequency.

### FR-05.2 Resonance Criteria
Resonance shall be identified based on:

- Maximum current magnitude.
- Phase difference approaching zero degrees.

### FR-05.3 Anti-Resonance Protection
The firmware shall include a configurable constraint to avoid selecting anti-resonance points.

---

## FR-06: Resonance Frequency Reporting

After successful detection, the firmware shall:

- Store the detected resonance frequency.
- Report associated:
  - Phase value
  - Current value
- Indicate detection status.

---

## FR-07: Closed-Loop Resonance Tracking

The firmware shall support periodic resonance re-evaluation.

### FR-07.1 Periodic Operation
When enabled, the firmware shall automatically repeat resonance detection at a configurable time interval.

### FR-07.2 Sweep Width Definition
The firmware shall define a sweep window centered around the last detected resonance.

The parameter representing sweep width shall be interpreted as total span.

---

## FR-08: Modbus Communication

The firmware shall expose configuration, measurement results, and control triggers via Modbus RTU.

### FR-08.1 Configuration Parameters
The firmware shall allow external configuration of:

- Frequency
- Sweep range
- Sweep step
- Measurement parameters
- Calibration parameters

### FR-08.2 Measurement Reporting
The firmware shall provide:

- Peak voltage
- Peak current
- Phase difference
- Resonance frequency
- System status indicators

---

# 3. Operational Constraints

- The system shall operate within the safe voltage limits of the analog excitation stage.
- The firmware shall ensure frequency values remain within valid bounds.
- The firmware shall prevent invalid sweep ranges.

---

# 4. Non-Functional Requirements

## NFR-01: Deterministic Behavior
All sweep operations shall follow deterministic and repeatable behavior.

## NFR-02: Measurement Stability
Measurement routines shall support configurable averaging to reduce noise.

## NFR-03: Real-Time Responsiveness
The firmware shall respond to Modbus commands within acceptable RTU timing constraints.

## NFR-04: Parameter Persistence
Configuration parameters shall be stored in non-volatile memory when required.

---

# 5. Definitions

**Resonance Frequency:**  
Frequency at which current magnitude is maximum and phase difference between voltage and current approaches zero.

**Anti-Resonance:**  
Frequency region where impedance peaks and current decreases despite phase proximity.

---

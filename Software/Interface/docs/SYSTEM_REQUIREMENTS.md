# System Requirements Specification

This document defines the baseline requirements for the CellCharger system. Each
requirement uses a stable ID so implementation, tests, calibration records, and
release evidence can be traced back to the specification.

Requirement status values:

- `Target`: required system capability that must be implemented and verified.
- `Interface`: software interface requirement for the Qt application and external
  API.
- `Hardware`: electrical, mechanical, or safety capability provided outside this
  repository, but exposed or monitored by the interface where applicable.

## 5.1 Functional Requirements

| ID | Requirement | Description | Status |
| --- | --- | --- | --- |
| ELE-01 | Voltage Range | DUT voltage shall be programmable and measurable from 0 V to 5.5 V. | Target |
| ELE-02 | Current Range | DUT current shall be programmable and measurable from -100 A to 100 A. | Target |
| ELE-03 | Current Resolution | Minimum programmable current step shall be <= 0.1 A. | Target |
| ELE-04 | Power Capability | Continuous power capability shall be >= 550 W. | Hardware |
| ELE-05 | Voltage Resolution | Voltage resolution shall be <= 1 mV. | Target |
| ELE-06 | Current Accuracy | Current accuracy shall be +/- 0.1% of reading +/- 20 mA. | Target |
| ELE-07 | Voltage Accuracy | Voltage accuracy shall be +/- 0.05% of reading +/- 2 mV. | Target |
| ELE-08 | Power Efficiency | Conversion efficiency shall be >= 90% at full load. | Hardware |

## 5.2 Measurement And Data Acquisition Requirements

| ID | Requirement | Description | Status |
| --- | --- | --- | --- |
| MEAS-01 | Capacity Measurement | Capacity and energy shall be computed by time integration of current and voltage. | Target |
| MEAS-02 | Integration Step | Integration update rate shall be <= 100 ms. | Target |
| MEAS-03 | Capacity Accuracy | Total error shall be <= 0.2% over a full charge/discharge cycle. | Target |
| MEAS-04 | Internal Resistance Method | DC internal resistance shall be measured with a current step pulse and delta-V/delta-I computation. | Target |
| MEAS-05 | Pulse Amplitude | Pulse amplitude shall be programmable from 5 A to 100 A. | Target |
| MEAS-06 | Pulse Width | Pulse width shall be configurable from 10 ms to 5 s. | Target |
| MEAS-07 | IR Measurement Range | Internal resistance measurement range shall be 0.1 mOhm to 100 mOhm. | Target |
| MEAS-08 | IR Resolution | Internal resistance resolution shall be <= 0.01 mOhm. | Target |
| MEAS-09 | IR Accuracy | Internal resistance accuracy shall be +/- 3% of reading +/- 0.01 mOhm. | Target |
| MEAS-10 | Sampling Rate (IR) | Voltage and current sampling during pulse measurement shall be >= 5 kHz. | Target |
| MEAS-11 | Data Logging | Standard logging shall be >= 1 kHz, and high-speed transient logging shall be >= 5 kHz. | Target |
| MEAS-12 | Data Format | Logs shall be stored in timestamped CSV files with cycle and step indices. | Interface |

## 5.3 Test Procedure And Control Requirements

| ID | Requirement | Description | Status |
| --- | --- | --- | --- |
| TEST-01 | Operating Modes | The system shall support CC, CV, CP, and CR modes. | Target |
| TEST-02 | Sequence Programming | The user shall be able to define test sequences with charge, discharge, and rest states. | Interface |
| TEST-03 | Conditional Logic | The system shall support start and stop conditions based on voltage, current, temperature, or pressure. | Target |
| TEST-04 | Cycle Definition | A single test script shall support at least 10,000 cycles. | Interface |
| TEST-05 | 24/7 Operation | The system shall operate continuously at full rated current with proper cooling. | Hardware |
| TEST-06 | End-Of-Test Criteria | End-of-test limits shall be configurable for capacity drop, internal resistance increase, voltage thresholds, and time thresholds. | Interface |
| TEST-07 | Automation Interface | The system shall provide an external API for automated control. | Interface |

## 5.4 Environmental Requirements

| ID | Requirement | Description | Status |
| --- | --- | --- | --- |
| ENV-01 | Temperature Range | DUT environment shall be controllable from -20 deg C to +60 deg C. | Hardware |
| ENV-02 | Temperature Input | The system shall support at least one temperature measurement for DUT monitoring. | Target |
| ENV-03 | Temperature Resolution | Temperature resolution shall be <= 0.1 deg C. | Target |
| ENV-04 | Temperature Accuracy | Temperature accuracy shall be +/- 1 deg C or better. | Target |
| ENV-05 | Pressure Range | DUT environment pressure shall be measurable from 0.5 bar to 2.0 bar absolute. | Target |
| ENV-06 | Pressure Resolution | Pressure resolution shall be <= 1 mbar. | Target |
| ENV-07 | Pressure Accuracy | Pressure accuracy shall be +/- 10 mbar or better. | Target |
| ENV-08 | Environment Safety | High-current outputs shall be disabled when the chamber is open. | Target |
| ENV-09 | Environmental Logging | Temperature and pressure shall be logged synchronously with electrical data. | Interface |

## 5.5 Safety And Protection Requirements

| ID | Requirement | Description | Status |
| --- | --- | --- | --- |
| SAF-01 | Overvoltage Protection | Hardware shall cut off output if DUT voltage exceeds Vmax. | Hardware |
| SAF-02 | Undervoltage Protection | Discharge shall be disabled if DUT voltage is below Vmin. | Target |
| SAF-03 | Overcurrent Protection | Hardware shall limit current at +/- 110 A or 105% of setpoint. | Hardware |
| SAF-04 | Over Temperature Protection | The system shall shut down if DUT or internal temperature exceeds the configured limit. | Target |
| SAF-05 | Reverse Polarity | The system shall detect reversed polarity and block current flow. | Target |
| SAF-06 | Emergency Stop | A hardware E-Stop input shall immediately disconnect the current path. | Hardware |
| SAF-07 | Interlocks | Chamber door or lid interlock input shall be required before current enable. | Target |
| SAF-08 | Fault Handling | Software shall display an error code and safely terminate the active test. | Interface |

## 5.6 Mechanical And Interface Requirements

| ID | Requirement | Description | Status |
| --- | --- | --- | --- |
| MECH-01 | Power Terminals | High-current connectors shall be rated for at least 150 A continuous. | Hardware |
| MECH-02 | Sense Lines | Kelvin voltage-sense connections shall be provided at DUT terminals. | Hardware |
| MECH-03 | Form Factor | The system shall use a rackmount or benchtop chassis with forced-air cooling. | Hardware |
| MECH-04 | Thermal Design | The thermal design shall dissipate at least 500 W continuously. | Hardware |
| MECH-05 | Chamber Feedthroughs | Power and sense cabling shall be rated for temperature/pressure chamber use. | Hardware |
| MECH-06 | User Interface | Front panel indicators shall show Power, Fault, and Test Status. | Hardware |
| MECH-07 | Communication | The system shall provide USB and CAN for control. | Target |

## 5.7 Calibration And Diagnostics Requirements

| ID | Requirement | Description | Status |
| --- | --- | --- | --- |
| CAL-01 | Voltage Calibration | User-accessible voltage calibration against a precision reference shall be supported. | Interface |
| CAL-02 | Current Calibration | Current calibration shall be adjustable using a known shunt or reference sensor. | Interface |
| CAL-03 | Temperature Calibration | Temperature offset and gain calibration using known reference sensors shall be supported. | Interface |
| CAL-04 | Pressure Calibration | Zero and span pressure calibration routines shall be supported. | Interface |
| CAL-05 | Self-Test Routine | On power-up, ADC, shunt, and sensor integrity checks shall run. | Target |
| CAL-06 | Fault Detection | Test start shall be inhibited if any self-check fails. | Target |

## 5.8 Software And Data Management Requirements

| ID | Requirement | Description | Status |
| --- | --- | --- | --- |
| SW-01 | Graphical Interface | A GUI shall support test setup, monitoring, and visualization of voltage, current, temperature, and pressure over time. | Interface |
| SW-02 | Test Recipes | Users shall be able to create, save, and load test scripts with editable parameters. | Interface |
| SW-03 | Real-Time Display | The GUI shall provide live plots of voltage, current, temperature, and pressure. | Interface |
| SW-04 | Data Export | CSV export shall include timestamps, cycle numbers, and test step metadata. | Interface |
| SW-05 | API Access | A programmatic interface shall support remote control and automation. | Interface |
| SW-06 | Alarm Logging | All safety events shall be recorded with timestamps and cause. | Interface |
| SW-07 | Data Backup | Logs shall be saved automatically at periodic intervals to reduce data loss during power failure. | Interface |

## 5.9 Performance And Reliability Requirements

| ID | Requirement | Description | Status |
| --- | --- | --- | --- |
| PERF-01 | System Uptime | Operational uptime shall be >= 99% under nominal conditions. | Target |
| PERF-02 | Thermal Stability | Output current ripple shall be < 0.5% FS from 25 deg C to 60 deg C ambient variation. | Target |
| PERF-03 | Response Time | Current step response shall reach 90% of setpoint in < 5 ms. | Target |
| PERF-04 | Long-Term Stability | Drift shall be < 0.05% FS over 24 h operation. | Target |

## 5.10 Compliance Requirements

| ID | Requirement | Description | Status |
| --- | --- | --- | --- |
| COMP-01 | Electrical Safety | The system shall comply with IEC 61010-1 for laboratory equipment. | Target |
| COMP-02 | EMC | The system shall meet EN 61326-1 electromagnetic compatibility requirements. | Target |
| COMP-03 | Data Integrity | File naming and log structure shall comply with ISO 17025 traceability principles. | Interface |

## Verification Traceability

Each requirement shall be verified by one or more of the following methods:

- `Inspection`: design review, schematic review, code review, or document review.
- `Analysis`: calculation, tolerance analysis, timing analysis, or log review.
- `Test`: automated test, bench test, calibration run, or acceptance test.
- `Certification`: external compliance evidence or certified laboratory report.

| Area | Primary Verification Method |
| --- | --- |
| Electrical performance | Test, Analysis |
| Measurement and data acquisition | Test, Analysis |
| Test procedure and control | Test, Inspection |
| Environmental monitoring and control | Test, Inspection |
| Safety and protection | Test, Inspection |
| Mechanical interface | Inspection, Certification |
| Calibration and diagnostics | Test, Inspection |
| Software and data management | Automated Test, Inspection |
| Performance and reliability | Test, Analysis |
| Compliance | Certification, Inspection |

## Software Implementation Notes

The interface repository is responsible for GUI workflows, recipe management,
communication with hardware/firmware, logging, export, visualization, and
automation-facing behavior. Requirements marked `Hardware` still need to be
visible to the software where state, faults, interlocks, calibration data, or
test evidence affect user operation.

Current software acceptance hooks covered by `make verify` include:

- Requirement ID traceability between this SRS and the code registry.
- Profile setup limits for voltage, current, temperature, and CC/CV/CP/CR modes.
- Command protocol encoding for signed setpoints across the DUT current range.
- Capacity and energy integration with the required update step.
- Internal resistance pulse parameter validation and delta-V/delta-I computation.
- Timestamped CSV logging with cycle and step metadata.
- Alarm logging with timestamp, code, and cause.
- Recipe validation for modes, conditions, and 10,000-cycle scripts.
- Software safety decisions for voltage, current, temperature, polarity, E-Stop,
  self-test, and chamber interlock inputs.
- Calibration coefficient calculation and self-test result handling.

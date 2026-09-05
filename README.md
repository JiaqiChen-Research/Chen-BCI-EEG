# Chen-EEG-BCI

This is my attempt to build an EEG acquisition system from scratch and use it for basic BCI experiments.

> **Status:** Hardware V1.0 — PCB fabricated, bring-up and testing in progress.

## 1. Overview

I am building a compact, battery-powered EEG acquisition device based on the ADS1299 and STM32F405.

The goal of this project is to build an EEG system from scratch and explore the complete process from hardware design and embedded firmware to EEG acquisition, signal processing, and basic BCI experiments.

The initial focus is on:

- EEG signal acquisition
    
- EEG signal processing
    
- Motor imagery classification
    
- Real-time EEG data acquisition
    
- Embedded systems and PCB design
    

## 2. System Architecture

```text
                         EEG Electrodes
                                │
                                ▼
                         ┌─────────────┐
                         │   ADS1299   │
                         │   EEG AFE   │
                         └──────┬──────┘
                                │ SPI
                                ▼
                         ┌─────────────┐
                         │  STM32F405  │
                         │     MCU     │
                         └───┬─────┬───┘
                             │     │
                    ┌────────┘     └────────┐
                    ▼                       ▼
              ┌──────────┐            ┌───────────┐
              │ SD Card  │            │ Bluetooth │
              │ Storage  │            │ Wireless  │
              └──────────┘            └─────┬─────┘
                                            │
                                            ▼
                                      ┌───────────┐
                                      │ Computer  │
                                      └─────┬─────┘
                                            │
                                            ▼
                                  Signal Processing
                                            │
                                            ▼
                                      BCI Application

                         USB Type-C
                              │
                         USB Data
                              │
                              ▼
                         Computer

                         ST-Link
                            │
                           SWD
                            │
                            ▼
                       STM32F405
                    Programming/Debug
```

The STM32F405 is the main controller of the system. EEG data are acquired from the ADS1299 through SPI and can be stored on the SD card or transmitted to a computer via Bluetooth or USB Type-C.

The USB Type-C interface is used for wired data communication, while ST-Link is used separately for firmware programming and debugging.
## 3. Hardware

### Main Components

|Module|Component|
|---|---|
|EEG AFE|TI ADS1299|
|MCU|STM32F405RGT6|
|IMU|Bosch BMI270|
|Wireless|E73-2G4M08S1CX|
|Storage|MicroSD Card|
|Power Management|TPS63070 + MCP1754|
|EEG Supply|±2.5 V analog rails|
|Main Logic Supply|3.3 V|
|Debug Interface|SWD|
|Power Source|Battery|

### EEG Acquisition

The ADS1299 is used as the main analog front end for 8-channel EEG acquisition. It communicates with the STM32F405 through SPI and is powered by a dedicated analog power supply. The PCB design also includes input protection and local decoupling to improve signal integrity.
    
##  4. MCU Interface

The ADS1299 communicates with the STM32F405 through SPI.

### ADS1299 ↔ STM32F405

|ADS1299|STM32F405|
|---|---|
|SCLK|PA5|
|MISO|PA6|
|MOSI|PA7|
|CS|PA4|
|DRDY|PB0|
|RESET|PB1|

The STM32F405 handles ADS1299 configuration and EEG data acquisition, and serves as the main controller for data storage and communication.

## 5. Motion Sensing

A BMI270 IMU is included to record motion information together with EEG.

The IMU data can be used for:

- Motion artifact analysis
    
- Artifact detection
    
- EEG and IMU data synchronization
    
- Future multimodal BCI experiments
    

The current focus is on acquiring and synchronizing motion data with EEG. More advanced artifact removal and multimodal analysis will be explored later.

## 6. Wireless Communication

An E73-2G4M08S1CX Bluetooth module is used for wireless communication between the acquisition device and a computer.

The intended data flow is:

```text
ADS1299
   ↓
STM32F405
   ↓
Data Packet
   ↓
Bluetooth
   ↓
Computer
```

The STM32F405 collects EEG data from the ADS1299, packages the data, and sends it through the Bluetooth module.

The wireless protocol and packet format are still under development.

## 7. Power System

The device is designed to operate from a battery during EEG acquisition.

The current power architecture is:

```text
Battery
   │
   ▼
TPS63070
   │
   ├──► 3.3 V
   │
   ▼
Analog Power Regulation
   │
   ├──► +2.5 V
   │
   └──► -2.5 V
```

The power system uses separate regulation for the digital and analog parts of the circuit. Battery operation also avoids directly powering the EEG acquisition system from the computer's USB port during recording.

Power integrity and system noise will be evaluated during hardware testing.
## 8. PCB

The PCB was designed in JLCEDA and manufactured by JLCPCB.

The PCB integrates the EEG analog front end, STM32F405, power management, motion sensing, wireless communication, and SD card storage on a single board.

### Design considerations

- Compact board layout
    
- Short analog signal paths around the ADS1299
    
- Dedicated ADS1299 analog section
    
- Local decoupling near sensitive components
    
- Separation of analog and digital power/ground considerations
    
- SWD interface for firmware programming and debugging
    
- Battery-powered operation
    
- On-board Bluetooth communication
    
- SD card storage
    

The first PCB version is mainly intended for hardware bring-up and system validation. Further revisions will be made based on testing results.
    

### Current Status

- [x] System architecture
    
- [x] Schematic
    
- [x] PCB layout
    
- [x] PCB fabrication
    
-    Component assembly
    
-  Power-up testing
    
-  ADS1299 bring-up
    
-  STM32 firmware
    
-  EEG signal validation
    
-  Wireless data transmission
    
-  End-to-end BCI test
    

##  9. Software

The software side of the project is planned around Python and commonly used EEG analysis tools.

The planned EEG processing pipeline is:

```text
Raw EEG
   ↓
Filtering
   ↓
Notch Filtering
   ↓
Re-referencing
   ↓
Artifact Removal
   ↓
Epoching
   ↓
Feature Extraction
   ↓
Classification
   ↓
BCI Output
```

Tools and frameworks I have explored or plan to use include:

- Python
    
- MNE-Python
    
- MATLAB
    
- EEGLAB
    
- FieldTrip
    
- scikit-learn
    

Methods being considered for EEG analysis and classification include:

- CSP
    
- LDA
    
- PSD
    
- FFT
    
- STFT
    
- Wavelet analysis
    
- xDAWN
    
- SSD
    

The software pipeline is still under development. The initial goal is to build a basic EEG processing pipeline and eventually use it for motor imagery classification.
## 10. Current BCI Goal

The initial goal is to build a simple motor-imagery BCI using the custom EEG hardware.

The planned pipeline is:

```text
Motor Imagery
     ↓
EEG Acquisition
     ↓
Preprocessing
     ↓
CSP
     ↓
LDA
     ↓
Left / Right Classification
     ↓
Cursor Control
```

The first target is to classify left- and right-side motor imagery from EEG signals. If the basic classification pipeline works, I plan to extend it toward real-time cursor control.

The complete real-time BCI pipeline has not been implemented yet and will be developed step by step as the hardware and software are validated.

## 11. Repository Structure

The repository structure is still being organized and will be expanded as the hardware, firmware, and software components are developed.

## 12. Roadmap

The next step is to validate the V1.0 hardware, bring up the ADS1299 and STM32, and acquire the first EEG signals.

After the hardware is validated, I plan to build the EEG processing pipeline and collect motor imagery data for left/right classification.

The long-term goal is to integrate the acquisition, processing, classification, and cursor-control components into a real-time BCI system.

## 13. Why Build It?

OpenBCI already provides a good platform for EEG experiments, so building another EEG board may seem unnecessary.

For me, the point is to understand what happens underneath the software: analog front-end design, power, PCB layout, embedded acquisition, data transmission, and finally EEG processing.

## 14. Disclaimer

This project is an experimental prototype developed for learning and research purposes.

It is not a medical device and is not intended for medical diagnosis, treatment, or clinical use.

## 15. License

The license for this project has not been decided yet.
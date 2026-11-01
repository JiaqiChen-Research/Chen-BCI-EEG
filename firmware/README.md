# Firmware

The firmware is developed using **STM32CubeIDE** with the STM32 HAL library.

## Main Functions

- Configure ADS1299 registers through SPI.
- Read ADC data frames using the DRDY signal.
- Parse 24-bit ADC data.
- Transmit acquired bio-signals to a PC through UART.

## Development Environment

- IDE: STM32CubeIDE
- MCU: STM32F405RGT6
- ADC: ADS1299
- Communication: SPI and UART

## How to Use

1. Open the STM32CubeIDE project.
2. Check the MCU pin configuration and peripheral settings.
3. Build the project.
4. Connect the STM32 development board using ST-Link.
5. Download the firmware to the MCU.
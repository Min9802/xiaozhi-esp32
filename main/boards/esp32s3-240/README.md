# ESP32S3-240 Board Configuration

## Overview

This is a custom board configuration for the Xiaozhi AI Voice Chat Robot project using ESP32S3 with:
- **Display**: GC9A01 240x240 Round LCD
- **Microphone**: INMP441 Digital MEMS Microphone
- **Speaker**: MAX98357 Audio Amplifier

## Hardware Specifications

### Processor
- ESP32-S3 (Dual-core Xtensa processor)

### Display
- **Model**: GC9A01
- **Resolution**: 240x240 pixels
- **Interface**: SPI
- **Color Depth**: 16-bit

### Audio Input
- **Model**: INMP441
- **Type**: Digital MEMS Microphone (PDM)
- **Sample Rate**: 16 kHz
- **Interface**: I2S (PDM)

### Audio Output
- **Model**: MAX98357A
- **Type**: Digital Audio Class D Amplifier
- **Sample Rate**: 24 kHz
- **Interface**: I2S

## Pin Configuration

### Display Pins
- **SCK (Clock)**: GPIO 8
- **MOSI (Data)**: GPIO 10
- **CS (Chip Select)**: GPIO 9
- **DC (Data/Command)**: GPIO 11
- **RST (Reset)**: GPIO 13
- **Backlight**: GPIO 14 (PWM)

### Audio Pins - INMP441 Microphone
- **WS (Word Select)**: GPIO 4
- **SCK (Clock)**: GPIO 5
- **DIN (Data In)**: GPIO 6

### Audio Pins - MAX98357 Speaker
- **DOUT (Data Out)**: GPIO 7
- **BCLK (Bit Clock)**: GPIO 15
- **LRCK (Left/Right Clock)**: GPIO 16

### Control Pins
- **Boot Button**: GPIO 0

## Software Configuration

### Audio Configuration
The board uses **simplex I2S mode** where:
- Microphone uses I2S0 for input (GPIO 4, 5, 6)
- Speaker uses I2S0 for output (GPIO 7, 15, 16)

### Display Configuration
- Resolution: 240x240
- Mirror X: Enabled
- Mirror Y: Disabled
- Swap XY: Disabled

## Compilation

### Method 1: Using idf.py
```bash
idf.py set-target esp32s3
idf.py menuconfig
# Navigate to: Xiaozhi Assistant -> Board Type -> ESP32S3-240
idf.py build
idf.py flash monitor
```

### Method 2: Using release.py Script (Recommended)
```bash
python scripts/release.py esp32s3-240
```

## LED and Button Support
- **Boot Button**: GPIO 0 - Click to toggle chat state, Double-click to reset WiFi

## Display Support
- 240x240 round LCD with GC9A01 driver
- Font sizes: 16x16 (for 240x240 display)
- Suitable for watch-style or circular display applications

## Audio Performance
- Input: INMP441 digital microphone with I2S interface
- Output: MAX98357 Class-D amplifier with I2S interface
- No I2C codec required - direct I2S digital audio

## Troubleshooting

### No Display Output
- Check SPI pin connections (GPIO 8, 10, 9, 11, 13)
- Verify GC9A01 driver compatibility
- Check reset pin (GPIO 13)
- Verify backlight voltage on GPIO 14

### No Audio Input/Output
- Check I2S microphone pins (GPIO 4, 5, 6)
- Check I2S speaker pins (GPIO 7, 15, 16)
- Verify INMP441 is properly connected and powered
- Check MAX98357 enable pin and speaker connections

### WiFi Connection Issues
- Try pressing boot button (GPIO 0) to reset WiFi configuration
- Check WiFi antenna connection if present

## References

- ESP32-S3 Datasheet: https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
- GC9A01 Display Driver: https://github.com/espressif/esp-lcd-gc9a01
- INMP441 Datasheet: https://www.invensense.com/products/audio/mems-microphones/inmp441/
- MAX98357 Datasheet: https://datasheets.maximintegrated.com/en/ds/MAX98357.pdf

## Notes

- The board configuration is optimized for round 240x240 displays
- INMP441 provides high-quality digital audio input
- MAX98357 provides efficient audio amplification
- Simplex I2S mode allows independent control of input and output

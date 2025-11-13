# ESP32S3-240 Pin Configuration Diagram

## ESP32S3 Pin Assignments

```
ESP32S3 PINOUT DIAGRAM (ESP32-S3 R0 Variant)
============================================

                    ┌─────────────────────────────────────┐
                    │   ESP32-S3 DEVKIT-R1                │
                    │   (38-pin / 43-pin versions)        │
                    └─────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ AUDIO INPUT (INMP441 Microphone)                        │
├─────────────────────────────────────────────────────────┤
│ GPIO 4   ← I2S MIC WS (Word Select / LRCLK)            │
│ GPIO 5   ← I2S MIC SCK (Serial Clock / BCLK)           │
│ GPIO 6   ← I2S MIC DIN (Data In)                       │
│ GND      ← Ground                                       │
│ 3.3V     ← Power Supply                                │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ AUDIO OUTPUT (MAX98357A Amplifier)                      │
├─────────────────────────────────────────────────────────┤
│ GPIO 7   → I2S SPK DOUT (Data Out)                     │
│ GPIO 15  → I2S SPK BCLK (Bit Clock)                    │
│ GPIO 16  → I2S SPK LRCK (Left/Right Clock)             │
│ GND      ← Ground                                       │
│ 5V/3.3V  ← Power Supply (depends on MAX98357 variant)   │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ DISPLAY (GC9A01 240x240 LCD via SPI3)                  │
├─────────────────────────────────────────────────────────┤
│ GPIO 8   → DISPLAY_SPI_SCLK (Clock)                    │
│ GPIO 10  → DISPLAY_SPI_MOSI (Data)                     │
│ GPIO 9   → DISPLAY_SPI_CS (Chip Select)                │
│ GPIO 11  → DISPLAY_SPI_DC (Data/Command)               │
│ GPIO 13  → DISPLAY_SPI_RESET (Reset)                   │
│ GPIO 14  → DISPLAY_BACKLIGHT (PWM brightness)          │
│ GND      ← Ground                                       │
│ 3.3V     ← Power Supply                                │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ CONTROL PINS                                            │
├─────────────────────────────────────────────────────────┤
│ GPIO 0   ← BOOT_BUTTON (Boot button)                   │
│ GND      ← Ground (button press to GND)                │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ RESERVED / NOT USED                                     │
├─────────────────────────────────────────────────────────┤
│ GPIO 1   - Available (TX for debug/serial)             │
│ GPIO 2   - Available                                   │
│ GPIO 3   - Available (RX for debug/serial)             │
│ GPIO 12-19  - Used by flash/RAM (do not use)           │
│ GPIO 20-24  - Available                                │
│ GPIO 25-32  - Available                                │
│ GPIO 33-46  - Available (QSPI shared pins)             │
│ GPIO 47-48  - USB OTG pins (if using USB)             │
└─────────────────────────────────────────────────────────┘
```

## Module Connection Diagram

```
                    ┌────────────────────────────┐
                    │    ESP32-S3 Module         │
                    │  (WiFi + Bluetooth)        │
                    └────────────────────────────┘
                              │
        ┌───────────────────────┼───────────────────────┐
        │                       │                       │
        ▼                       ▼                       ▼
   ┌─────────────┐        ┌─────────────┐         ┌─────────────┐
   │  INMP441    │        │  GC9A01     │         │  MAX98357   │
   │ Microphone  │        │  LCD Screen │         │ Amplifier   │
   │  (I2S In)   │        │  (SPI)      │         │  (I2S Out)  │
   └─────────────┘        └─────────────┘         └─────────────┘
        │                       │                       │
        │ GPIO 4,5,6            │ GPIO 8,9,10,11,13,14 │ GPIO 7,15,16
        │ GND, 3.3V             │ GND, 3.3V            │ GND, 3.3V+
        └───────────────────────┼───────────────────────┘
                                │
                        ┌───────┴────────┐
                        │  GPIO 0 Button │
                        │  (BOOT)        │
                        └────────────────┘
```

## I2S Audio Data Flow

```
MICROPHONE PATH:
═══════════════════════════════════════════════════════════════
INMP441 Microphone
    │
    ├── GPIO 6 (DIN) ─────┐
    │                     ├─→ I2S0 Rx Controller ─→ Audio Buffer
    ├── GPIO 4 (WS) ──────┤
    │                     │
    └── GPIO 5 (SCK) ─────┘


SPEAKER PATH:
═══════════════════════════════════════════════════════════════
Audio Buffer
    │
    ├─→ I2S0 Tx Controller
    │
    ├── GPIO 7 (DOUT) ────┐
    │                     ├─→ MAX98357 ─→ Speaker (2W output)
    ├── GPIO 15 (BCLK) ───┤
    │                     │
    └── GPIO 16 (LRCK) ───┘
```

## SPI Display Interface

```
GC9A01 LCD 240x240
╔════════════════════════════════════════════════════════════════╗
║                    ╔═════════════════╗                         ║
║                    ║  GC9A01 Driver  ║                         ║
║                    ╚═════════════════╝                         ║
║                            │                                   ║
║    ┌────────────────────────┼────────────────────────┐        ║
║    │                        │                        │        ║
║    ▼                        ▼                        ▼        ║
║ ┌──────┐              ┌──────────────┐           ┌────────┐  ║
║ │ MOSI │────────────→ │   Display    │ ←─────── │ RESET  │  ║
║ │GPIO10│              │   Data In    │           │ GPIO13 │  ║
║ └──────┘              └──────────────┘           └────────┘  ║
║                              │                                 ║
║ ┌──────┐                     ▼                                 ║
║ │ SCLK │───────────────→ ┌──────────────┐                    ║
║ │GPIO 8│                │   Display    │                     ║
║ └──────┘                │  Shift Reg   │                     ║
║                         └──────────────┘                      ║
║ ┌──────┐                     │                                 ║
║ │  CS  │────────────────→ ┌──────────────┐                    ║
║ │GPIO 9│                │   Chip Sel   │                     ║
║ └──────┘                └──────────────┘                      ║
║                              │                                 ║
║ ┌──────┐                     ▼                                 ║
║ │  DC  │────────────────→ ┌──────────────┐                    ║
║ │GPIO11│                │  CMD/Data     │                     ║
║ └──────┘                └──────────────┘                      ║
║                              │                                 ║
║ ┌────────────┐               │                                 ║
║ │ Backlight  │───→ PWM ──→ GPIO 14                           ║
║ │ Brightness │               │                                 ║
║ └────────────┘               ▼                                 ║
║                         ┌──────────────┐                      ║
║                         │   240x240    │                      ║
║                         │ Pixel Array  │                      ║
║                         │  (LCD Panel) │                      ║
║                         └──────────────┘                      ║
╚════════════════════════════════════════════════════════════════╝
```

## Power Supply Requirements

```
┌─────────────────────────────────────────────────────────┐
│ POWER SUPPLY SPECIFICATIONS                              │
├─────────────────────────────────────────────────────────┤
│ Component           │ Voltage   │ Max Current │ Typical │
├─────────────────────────────────────────────────────────┤
│ ESP32-S3 Module     │ 3.3V      │ 500mA       │ 200mA   │
│ INMP441 Microphone  │ 3.3V      │ 15mA        │ 10mA    │
│ GC9A01 LCD          │ 3.3V      │ 500mA       │ 100mA   │
│ MAX98357 Amplifier  │ 5V*       │ 500mA       │ 100mA   │
└─────────────────────────────────────────────────────────┘

* Check MAX98357 variant - some support 3.3V input with lower output
** Total typical current draw: ~410mA @ 3.3V + ~100mA @ 5V
```

## Ground Connections

```
GND DISTRIBUTION:
═══════════════════════════════════════════════════════════════
All following should be connected to GND (system ground):

ESP32-S3 GND pins (multiple)
├→ INMP441 GND
├→ GC9A01 GND  
├→ MAX98357 GND
├→ BOOT Button GND
└→ External power supply GND (common ground)

NOTE: Use star grounding where possible for audio quality
```

---

**Last Updated**: 2025-01-12
**Board**: ESP32S3-240
**Components**: GC9A01 + INMP441 + MAX98357

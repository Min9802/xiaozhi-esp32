# ESP32S3-240 Board Creation Summary

## ✅ Board Successfully Created

A new custom board configuration has been created for the Xiaozhi ESP32 project with the following specifications:

### Hardware Configuration

| Component | Model | Interface | Details |
|-----------|-------|-----------|---------|
| **Processor** | ESP32-S3 | - | Dual-core, 240 MHz |
| **Display** | GC9A01 | SPI | 240x240 round LCD |
| **Microphone** | INMP441 | I2S (Simplex) | 16 kHz digital audio |
| **Speaker** | MAX98357 | I2S (Simplex) | 24 kHz Class-D amplifier |

### Pin Configuration

#### Display (SPI3)
```
GPIO 8  - SCLK (Clock)
GPIO 10 - MOSI (Data)
GPIO 9  - CS (Chip Select)
GPIO 11 - DC (Data/Command)
GPIO 13 - RST (Reset)
GPIO 14 - Backlight (PWM)
```

#### Audio Input - INMP441 Microphone
```
GPIO 4  - WS (Word Select)
GPIO 5  - SCK (Clock)
GPIO 6  - DIN (Data In)
```

#### Audio Output - MAX98357 Amplifier
```
GPIO 7  - DOUT (Data Out)
GPIO 15 - BCLK (Bit Clock)
GPIO 16 - LRCK (Left/Right Clock)
```

#### Control
```
GPIO 0  - Boot Button
```

## 📁 Files Created

### Board Directory
`main/boards/esp32s3-240/`

### Files
1. **config.h** - Hardware pin definitions and configuration
2. **config.json** - Build system configuration
3. **esp32s3_240.cc** - Main board implementation class
4. **README.md** - Detailed documentation

## 🔧 Project Files Modified

### 1. `main/Kconfig.projbuild`
Added board type configuration option:
```kconfig
config BOARD_TYPE_ESP32S3_240
    bool "ESP32S3-240 (GC9A01 + INMP441 + MAX98357)"
    depends on IDF_TARGET_ESP32S3
```

### 2. `main/CMakeLists.txt`
Added CMake build configuration:
```cmake
elseif(CONFIG_BOARD_TYPE_ESP32S3_240)
    set(BOARD_TYPE "esp32s3-240")
    set(BUILTIN_TEXT_FONT font_puhui_basic_16_4)
    set(BUILTIN_ICON_FONT font_awesome_16_4)
    set(DEFAULT_EMOJI_COLLECTION twemoji_64)
```

## 🚀 How to Use

### Compilation Method 1: idf.py
```bash
cd xiaozhi-esp32
idf.py set-target esp32s3
idf.py menuconfig
```
Then navigate to: **Xiaozhi Assistant** → **Board Type** → **ESP32S3-240**

```bash
idf.py build
idf.py flash monitor
```

### Compilation Method 2: release.py Script (Recommended)
```bash
python scripts/release.py esp32s3-240
```

## 📋 Key Features

✅ **GC9A01 Display Driver Support**
- 240x240 circular display
- Full color 16-bit RGB
- Hardware mirror and swap support

✅ **INMP441 Microphone**
- Digital PDM microphone
- 16 kHz sampling rate
- Direct I2S interface (no I2C codec required)

✅ **MAX98357 Amplifier**
- Digital Class-D amplifier
- 24 kHz audio output
- Efficient power consumption
- Direct I2S interface

✅ **Simplex I2S Audio Mode**
- Separate I2S channels for input and output
- Allows simultaneous microphone and speaker operation
- Good for voice applications

✅ **WiFi Support**
- WifiBoard base class
- WiFi configuration reset via boot button
- Standard OTA firmware updates

## 🎯 Display Configuration

- **Resolution**: 240x240 pixels
- **Type**: Circular LCD (suitable for watch-style devices)
- **Color Format**: 16-bit RGB
- **Font Size**: 16x16 for optimal readability on 240x240 display
- **Emoji Support**: 64x64 pixel emoji collection

## 🔊 Audio System

**Simplex I2S Mode**:
- Microphone input: I2S0 (separate pins)
- Speaker output: I2S0 (separate pins)
- Allows both simultaneous recording and playback
- No I2C codec chip required (direct digital audio)

## 📚 Documentation

Complete documentation available in:
`main/boards/esp32s3-240/README.md`

Includes:
- Hardware specifications
- Detailed pin configurations
- Compilation instructions
- Troubleshooting guide
- Component datasheets links

## ✨ Technical Highlights

1. **Cost-Effective**: Uses simple digital audio components without complex I2C codec
2. **Compact**: Minimal pin count for audio I/O
3. **Round Display**: Suitable for watch-style or circular device designs
4. **High-Quality Audio**: INMP441 provides clean digital microphone input
5. **Efficient Amplifier**: MAX98357 provides up to 2W audio output

## 🔗 Component References

- **INMP441**: MEMS microphone with PDM output
  - Datasheet: InvenSense INMP441 Digital MEMS Microphone
  
- **MAX98357A**: Audio amplifier with I2S input
  - Datasheet: Maxim Integrated MAX98357A
  
- **GC9A01**: TFT LCD driver
  - Reference: https://github.com/espressif/esp-lcd-gc9a01

## 📝 Notes

- The board is fully integrated into the build system
- Compatible with Xiaozhi ESP32 project v3
- Supports all standard Xiaozhi features (MCP tools, OTA, etc.)
- Board identifier ensures correct firmware during OTA updates

---

**Status**: ✅ Ready for compilation and deployment

**Next Step**: Use either compilation method above to build firmware

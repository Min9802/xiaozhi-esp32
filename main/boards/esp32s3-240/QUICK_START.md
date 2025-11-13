# ESP32S3-240 Quick Start Guide

## 🚀 Getting Started

### Prerequisites
- ESP-IDF 5.1 or later
- Python 3.7+
- esp-idf-tools installed
- USB cable for flashing

### Board Components Checklist
- [ ] ESP32-S3 Development Board
- [ ] GC9A01 240x240 LCD Display (with backlight control)
- [ ] INMP441 Digital Microphone
- [ ] MAX98357A Audio Amplifier
- [ ] Speaker (4Ω, 1W minimum)

---

## 📦 Hardware Assembly

### Step 1: GC9A01 Display Connection
Connect the SPI display:
```
Display Pin  →  ESP32-S3 Pin
GND          →  GND
VCC/3.3V     →  3.3V
SCLK         →  GPIO 8
MOSI         →  GPIO 10
CS           →  GPIO 9
DC           →  GPIO 11
RST          →  GPIO 13
BLK(Backlight) → GPIO 14 (through 220Ω resistor if needed)
```

### Step 2: INMP441 Microphone Connection
```
Microphone Pin  →  ESP32-S3 Pin
GND             →  GND
VDD             →  3.3V
L/R (Left/Right) → GND (for left channel, connect to VDD for right)
WS              →  GPIO 4
CLK             →  GPIO 5
SD              →  GPIO 6
```

### Step 3: MAX98357A Amplifier Connection
```
Amplifier Pin   →  ESP32-S3 Pin / Component
GND             →  GND
VDD             →  5V Power Supply (or 3.3V depending on variant)
DIN             →  GPIO 7
BCLK            →  GPIO 15
LRCLK           →  GPIO 16
SD Mode         →  GND (enables 2W mono or stereo depending on variant)
OUT+            →  Speaker+ (left channel)
OUT-            →  Speaker- (left channel)
```

### Step 4: Boot Button
```
Button Pin      →  ESP32-S3 Pin
One end         →  GPIO 0
Other end       →  GND
```

---

## 💻 Software Setup

### Step 1: Clone and Navigate
```bash
cd ~/esp-idf
. export.sh

cd ~/xiaozhi-esp32
```

### Step 2: Set Target and Configure
```bash
idf.py set-target esp32s3
idf.py menuconfig
```

### Step 3: Select Board Type in menuconfig
Navigate to:
```
→ Xiaozhi Assistant
  → Board Type
    → ESP32S3-240 (GC9A01 + INMP441 + MAX98357)  [Select this]
```

Press `S` to save, then `Q` to exit.

### Step 4: Build
```bash
idf.py build
```

### Step 5: Flash to Device
```bash
idf.py flash monitor
```

This will:
1. Flash the firmware to ESP32-S3
2. Start the serial monitor to show logs

---

## 🧪 Testing

### Test 1: Display (Check Serial Output)
You should see logs like:
```
I (123) ESP32S3-240: Initialize SPI bus
I (124) ESP32S3-240: Init GC9A01 display
I (125) ESP32S3-240: Install panel IO
I (126) ESP32S3-240: Install GC9A01 panel driver
```

The display should light up with the Xiaozhi interface.

### Test 2: Audio Input (Microphone)
- Speak near the microphone
- Check logs for audio frames being captured
- Look for audio processing logs in the monitor

### Test 3: Audio Output (Speaker)
- Device should emit startup sound
- Audio responses should be audible from speaker

### Test 4: WiFi Connection
- Press GPIO 0 (boot button) once to toggle chat
- Device should attempt WiFi connection
- LED/display should indicate connection status

---

## 🔧 Troubleshooting

### Problem: No Display Output
**Solutions:**
1. Check SPI pin connections (GPIO 8, 10, 9, 11, 13)
2. Verify GC9A01 driver is recognized
3. Check reset pin (GPIO 13) voltage transitions
4. Try increasing SPI clock frequency in config.h if display is very close

### Problem: No Audio Playback
**Solutions:**
1. Check MAX98357 power supply (5V or 3.3V as per variant)
2. Verify speaker connections to OUT+ and OUT-
3. Check I2S pins (GPIO 7, 15, 16)
4. Ensure speaker is not damaged (test with multimeter)
5. Check MAX98357 enable pin (may need to connect to 3.3V or GND)

### Problem: Microphone Not Working
**Solutions:**
1. Check INMP441 power supply (3.3V)
2. Verify I2S pins (GPIO 4, 5, 6)
3. Check L/R pin connection (GND or VDD)
4. Verify no loose connections
5. Check if GPIO 4 or 5 are already used by other peripherals

### Problem: WiFi Connection Fails
**Solutions:**
1. Press boot button (GPIO 0) to enter configuration mode
2. Check router SSID and password
3. Verify antenna connection if external antenna is present
4. Try other WiFi networks

### Problem: Board Not Recognized in menuconfig
**Solutions:**
1. Run: `idf.py reconfigure`
2. Clear build: `rm -rf build/`
3. Rebuild configuration: `idf.py confserver` (optional visualization)

---

## 📋 Configuration Examples

### Change Audio Sample Rates
Edit `main/boards/esp32s3-240/config.h`:
```c
#define AUDIO_INPUT_SAMPLE_RATE  16000   // 8000, 16000, 32000, 48000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000   // 16000, 24000, 32000, 48000
```

### Use Duplex I2S Mode (Alternative)
Edit `main/boards/esp32s3-240/config.h`:
```c
// Comment this line to use duplex mode:
// #define AUDIO_I2S_METHOD_SIMPLEX

// Define shared I2S pins instead...
```

### Adjust Display Brightness
The brightness is controlled by PWM on GPIO 14. Default is 80% brightness.
Edit in the board .cc file to change default.

---

## 📚 Additional Resources

### Component Documentation
- **INMP441**: https://www.invensense.com/products/audio/mems-microphones/inmp441/
- **MAX98357**: https://datasheets.maximintegrated.com/en/ds/MAX98357.pdf
- **GC9A01**: https://www.buydisplay.com/download/ic/GC9A01.pdf
- **ESP32-S3**: https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf

### Project Documentation
- Xiaozhi Main README: `../README.md`
- Custom Board Guide: `../docs/custom-board-en.md` or `../docs/custom-board.md`
- MCP Protocol: `../docs/mcp-protocol.md`
- Audio System: `../main/audio/README.md`

---

## 💡 Pro Tips

1. **Use Star Grounding** for audio quality - connect all GNDs to a common point
2. **Add Decoupling Capacitors** (0.1µF) near power pins for stability
3. **Keep Audio Traces Short** to reduce noise pickup
4. **Use 3.3V for INMP441** - voltage affects noise floor
5. **Enable Device AEC** in menuconfig for better voice quality: 
   `Xiaozhi Assistant → Enable Device-Side AEC`

---

## ✅ Verification Checklist

- [ ] Board selects correctly in menuconfig
- [ ] Firmware compiles without errors
- [ ] Firmware flashes successfully
- [ ] Display shows Xiaozhi interface
- [ ] Boot button responds (GPIO 0 press)
- [ ] Microphone captures audio
- [ ] Speaker plays audio
- [ ] Device connects to WiFi
- [ ] Serial monitor shows no critical errors

---

## 🐛 Reporting Issues

If you encounter issues:

1. **Enable Debug Logs**: In menuconfig → Component Config → ESP Log Level
2. **Capture Full Serial Output**: 
   ```bash
   idf.py monitor > debug.log 2>&1
   ```
3. **Check Pin Conflicts**: Some GPIOs may be used by ESP32-S3 internally
4. **Verify Connections**: Use multimeter to check continuity
5. **Review Schematics**: Compare your board layout with provided PIN_DIAGRAM.md

---

**Happy Building! 🎉**

For more information, see:
- `README.md` - Full board documentation
- `PIN_DIAGRAM.md` - Detailed pin diagrams
- `config.h` - All hardware definitions
- `esp32s3_240.cc` - Board initialization code

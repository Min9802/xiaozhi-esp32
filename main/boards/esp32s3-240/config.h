#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// Audio Configuration
#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

// Use simplex I2S mode (separate I2S channels for microphone and speaker)
#define AUDIO_I2S_METHOD_SIMPLEX

#ifdef AUDIO_I2S_METHOD_SIMPLEX
// INMP441 Microphone I2S pins
#define AUDIO_I2S_MIC_GPIO_WS   GPIO_NUM_4
#define AUDIO_I2S_MIC_GPIO_SCK  GPIO_NUM_5
#define AUDIO_I2S_MIC_GPIO_DIN  GPIO_NUM_6

// MAX98357 Speaker I2S pins
#define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_7
#define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_15
#define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_16
#else
// Alternative: Duplex mode (shared I2S)
#define AUDIO_I2S_GPIO_WS GPIO_NUM_4
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_5
#define AUDIO_I2S_GPIO_DIN GPIO_NUM_6
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_7
#endif

// Button Configuration
#define BOOT_BUTTON_GPIO GPIO_NUM_0

// LED Configuration
#define BUILTIN_LED_GPIO GPIO_NUM_NC

// Display Configuration - GC9A01 240x240
#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  240
#define DISPLAY_MIRROR_X true
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY false

#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  0

// Display SPI pins
#define DISPLAY_SPI_SCLK_PIN    GPIO_NUM_8
#define DISPLAY_SPI_MOSI_PIN    GPIO_NUM_10
#define DISPLAY_SPI_CS_PIN      GPIO_NUM_9
#define DISPLAY_SPI_DC_PIN      GPIO_NUM_11
#define DISPLAY_SPI_RESET_PIN   GPIO_NUM_13

// SPI clock frequency for display
#define DISPLAY_SPI_SCLK_HZ     (40 * 1000 * 1000)

// Backlight Configuration
#define DISPLAY_BACKLIGHT_PIN GPIO_NUM_14
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false

#endif // _BOARD_CONFIG_H_

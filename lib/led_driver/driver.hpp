#pragma once

#include <cstdint>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define OLED_WIDTH  128
#define OLED_HEIGHT 32
#define OLED_BUFFER_SIZE (OLED_WIDTH * OLED_HEIGHT / 8) // 512 byts

// Адрес I2C (редко 0x3D, проверить)
#define OLED_I2C_ADDR 0x3C

class SSD1306 {
public:
    SSD1306(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t addr = OLED_I2C_ADDR);

    // Inittialization display
    bool init();

    // Clear buffer
    void clear();

    // Update picture on display
    void update();

    // Tip a pixel (x,y) – colot 1=white, 0=black
    void drawPixel(int16_t x, int16_t y, bool color);

    // Output symbol (шрифт 5x7) to cords (x,y)
    void drawChar(int16_t x, int16_t y, char c, bool color);

    // Output string
    void drawString(int16_t x, int16_t y, const char *str, bool color);

private:
    i2c_inst_t *i2c_;
    uint8_t sda_pin_;
    uint8_t scl_pin_;
    uint8_t addr_;
    uint8_t buffer_[OLED_BUFFER_SIZE];

    // Send command (1 byte)
    void writeCmd(uint8_t cmd);
    // Send data (byts massive)
    void writeData(const uint8_t *data, size_t len);
};
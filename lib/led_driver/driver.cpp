#include "ssd1306.hpp"
#include <cstring>

// ----- Команды SSD1306 (из даташита) -----
#define SSD1306_SET_MEM_MODE       0x20
#define SSD1306_SET_COL_ADDR       0x21
#define SSD1306_SET_PAGE_ADDR      0x22
#define SSD1306_SET_DISP_START_LINE 0x40
#define SSD1306_SET_CONTRAST       0x81
#define SSD1306_SET_CHARGE_PUMP    0x8D
#define SSD1306_SET_SEG_REMAP      0xA0
#define SSD1306_SET_ENTIRE_ON      0xA4
#define SSD1306_SET_NORM_DISP      0xA6
#define SSD1306_SET_MUX_RATIO      0xA8
#define SSD1306_SET_DISP_OFF       0xAE
#define SSD1306_SET_DISP_ON        0xAF
#define SSD1306_SET_COM_OUT_DIR    0xC0
#define SSD1306_SET_DISP_OFFSET    0xD3
#define SSD1306_SET_DISP_CLK_DIV   0xD5
#define SSD1306_SET_PRECHARGE      0xD9
#define SSD1306_SET_COM_PIN_CFG    0xDA
#define SSD1306_SET_VCOM_DET       0xDB

SSD1306::SSD1306(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t addr)
    : i2c_(i2c), sda_pin_(sda_pin), scl_pin_(scl_pin), addr_(addr) {
    memset(buffer_, 0, sizeof(buffer_));
}

// Send command
void SSD1306::writeCmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};  // 0x00 – управляющий байт: команда
    i2c_write_blocking(i2c_, addr_, buf, 2, false);
}

// ----- Send data (graph) -----
void SSD1306::writeData(const uint8_t *data, size_t len) {
    // Для данных управляющий байт 0x40
    uint8_t *buf = new uint8_t[len + 1];
    buf[0] = 0x40;
    memcpy(buf + 1, data, len);
    i2c_write_blocking(i2c_, addr_, buf, len + 1, false);
    delete[] buf;
}

// ----- Initialization display -----
bool SSD1306::init() {
    // 1. Настройка пинов I2C
    i2c_init(i2c_, 400 * 1000);             // 400 кГц
    gpio_set_function(sda_pin_, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin_, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin_);
    gpio_pull_up(scl_pin_);

    // 2. Последовательность инициализации (по даташиту)
    writeCmd(SSD1306_SET_DISP_OFF);          // выключить дисплей
    writeCmd(SSD1306_SET_DISP_CLK_DIV);      // установка делителя тактов
    writeCmd(0x80);                          // значение по умолчанию
    writeCmd(SSD1306_SET_MUX_RATIO);         // мультиплексор
    writeCmd(OLED_HEIGHT - 1);               // для 64 строк – 0x3F, для 32 – 0x1F
    writeCmd(SSD1306_SET_DISP_OFFSET);       // смещение
    writeCmd(0x00);
    writeCmd(SSD1306_SET_DISP_START_LINE);   // стартовая строка
    writeCmd(SSD1306_SET_CHARGE_PUMP);       // включить встроенный DC-DC
    writeCmd(0x14);                          // включить
    writeCmd(SSD1306_SET_MEM_MODE);          // режим адресации
    writeCmd(0x00);                          // горизонтальный
    writeCmd(SSD1306_SET_SEG_REMAP | 0x01);  // ремап сегментов (column 127 -> SEG0)
    writeCmd(SSD1306_SET_COM_OUT_DIR | 0x08); // ремап COM (сканирование сверху вниз)
    writeCmd(SSD1306_SET_COM_PIN_CFG);       // конфигурация выводов COM
    writeCmd(0x02);                          // для 64 строк, для 32 строк – 0x02)
    writeCmd(SSD1306_SET_CONTRAST);          // контраст
    writeCmd(0x7F);
    writeCmd(SSD1306_SET_ENTIRE_ON);         // отображать весь буфер
    writeCmd(SSD1306_SET_NORM_DISP);         // нормальный (не инвертированный)
    writeCmd(SSD1306_SET_DISP_ON);           // включить дисплей

    clear();
    update();
    return true;
}

// ----- Clear buffer -----
void SSD1306::clear() {
    memset(buffer_, 0, sizeof(buffer_));
}

// ----- Send buffer on display -----
void SSD1306::update() {

    writeCmd(SSD1306_SET_COL_ADDR);
    writeCmd(0);                      // начальная колонка 0
    writeCmd(OLED_WIDTH - 1);         // конечная колонка 127
    writeCmd(SSD1306_SET_PAGE_ADDR);
    writeCmd(0);                      // начальная страница 0
    writeCmd((OLED_HEIGHT / 8) - 1);  // последняя страница (7 для 64, 3 для 32)

    writeData(buffer_, sizeof(buffer_));
}

// ----- Draw pixel -----
void SSD1306::drawPixel(int16_t x, int16_t y, bool color) {
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    uint16_t idx = x + (y / 8) * OLED_WIDTH;
    uint8_t bit = 1 << (y % 8);
    if (color) buffer_[idx] |= bit;
    else       buffer_[idx] &= ~bit;
}

// ----- Шрифт 5x7 (символы ASCII 32..127) -----
// Каждый символ – 5 байт (по 5 столбцов), биты строки 0..6 (7 строк)
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 "
};

// В реальном коде вставьте полную таблицу (95 символов).
// Я приведу минимальный набор для примера, но лучше скачать готовый массив.

// ----- Output symbols (с использованием шрифта) -----
void SSD1306::drawChar(int16_t x, int16_t y, char c, bool color) {
    if (c < 32 || c > 127) c = 32; // заменяем непечатный на пробел
    uint8_t idx = c - 32;
    for (int col = 0; col < 5; col++) {
        uint8_t line = font5x7[idx][col];
        for (int row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                drawPixel(x + col, y + row, color);
            }
        }
    }
}

// ----- Output string -----
void SSD1306::drawString(int16_t x, int16_t y, const char *str, bool color) {
    while (*str) {
        drawChar(x, y, *str++, color);
        x += 6; // 5 пикселей ширина + 1 пробел
        if (x + 5 >= OLED_WIDTH) { // перенос строки
            x = 0;
            y += 8;
        }
    }
}
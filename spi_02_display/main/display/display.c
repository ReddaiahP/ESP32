#include "display.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"

#define PIN_NUM_MOSI 23
#define PIN_NUM_MISO -1
#define PIN_NUM_CLK  19
#define PIN_NUM_CS   5
#define PIN_NUM_DC   2
#define PIN_NUM_RST  4
#define PIN_NUM_LED  15

spi_device_handle_t spi;

void display_gpio_init(void) {
    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_LED, 1);

    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
}

void display_spi_init(void) {
    spi_bus_config_t spi_config = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096
    };

    spi_device_interface_config_t spi_device_config = {
        .clock_speed_hz = 10 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
        .flags = 0
    };

    spi_bus_initialize(SPI2_HOST, &spi_config, SPI_DMA_CH_AUTO);
    spi_bus_add_device(SPI2_HOST, &spi_device_config, &spi);
}

void send_cmd(uint8_t cmd) {
    gpio_set_level(PIN_NUM_DC, 0);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    spi_device_transmit(spi, &t);
}

void send_data(const uint8_t *data, int len) {
    gpio_set_level(PIN_NUM_DC, 1);
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    spi_device_transmit(spi, &t);
}

void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t caset[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
    uint8_t raset[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};

    send_cmd(0x2A); send_data(caset, sizeof(caset));
    send_cmd(0x2B); send_data(raset, sizeof(raset));
    send_cmd(0x2C);
}

void fill_color(uint16_t color) {
    clear_screen(color);
}

/* 1. New: Clear entire screen */
void clear_screen(uint16_t color) {
    set_window(0, 0, 239, 319);

    static uint8_t line_buf[240 * 2];
    for (int i = 0; i < 240; i++) {
        line_buf[i * 2] = color >> 8;
        line_buf[i * 2 + 1] = color & 0xFF;
    }

    gpio_set_level(PIN_NUM_DC, 1);
    for (int y = 0; y < 320; y++) {
        spi_transaction_t t = {
            .length = 240 * 16,
            .tx_buffer = line_buf,
        };
        spi_device_transmit(spi, &t);
    }
}

/* 2. New: Clear specific region */
void clear_region(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
    if (x1 < x0 || y1 < y0) return; // Safety check

    set_window(x0, y0, x1, y1);

    int width = x1 - x0 + 1;
    static uint8_t buf[480]; // up to 240 pixels per line (2 bytes each)
    for (int i = 0; i < width; i++) {
        buf[i * 2] = color >> 8;
        buf[i * 2 + 1] = color & 0xFF;
    }

    gpio_set_level(PIN_NUM_DC, 1);
    for (int y = y0; y <= y1; y++) {
        spi_transaction_t t = {
            .length = width * 16,
            .tx_buffer = buf,
        };
        spi_device_transmit(spi, &t);
    }
}

/* 3. New: Draw image from provided pixel buffer */
void draw_image(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, const uint16_t *image_data) {
    set_window(x0, y0, x0 + w - 1, y0 + h - 1);

    gpio_set_level(PIN_NUM_DC, 1);
    int total_pixels = w * h;
    const uint8_t *data_ptr = (const uint8_t *)image_data;

    // Transfer in chunks to avoid overflow
    for (int offset = 0; offset < total_pixels * 2; offset += 4096) {
        int chunk = ((total_pixels * 2 - offset) > 4096) ? 4096 : (total_pixels * 2 - offset);
        spi_transaction_t t = {
            .length = chunk * 8,
            .tx_buffer = data_ptr + offset,
        };
        spi_device_transmit(spi, &t);
    }
}

void ili9341_init(void) {
    send_cmd(0xEF); uint8_t ef[] = {0x03, 0x80, 0x02}; send_data(ef, 3);
    send_cmd(0xCF); uint8_t cf[] = {0x00, 0xC1, 0x30}; send_data(cf, 3);
    send_cmd(0xED); uint8_t ed[] = {0x64, 0x03, 0x12, 0x81}; send_data(ed, 4);
    send_cmd(0xE8); uint8_t e8[] = {0x85, 0x00, 0x78}; send_data(e8, 3);
    send_cmd(0xCB); uint8_t cb[] = {0x39, 0x2C, 0x00, 0x34, 0x02}; send_data(cb, 5);
    send_cmd(0xF7); uint8_t f7[] = {0x20}; send_data(f7, 1);
    send_cmd(0xEA); uint8_t ea[] = {0x00, 0x00}; send_data(ea, 2);
    send_cmd(0xC0); uint8_t c0[] = {0x23}; send_data(c0, 1);
    send_cmd(0xC1); uint8_t c1[] = {0x10}; send_data(c1, 1);
    send_cmd(0xC5); uint8_t c5[] = {0x3e, 0x28}; send_data(c5, 2);
    send_cmd(0xC7); uint8_t c7[] = {0x86}; send_data(c7, 1);
    send_cmd(0x36); uint8_t m[] = {0x48}; send_data(m, 1);
    send_cmd(0x3A); uint8_t pix[] = {0x55}; send_data(pix, 1);
    send_cmd(0xB1); uint8_t b1[] = {0x00, 0x18}; send_data(b1, 2);
    send_cmd(0xB6); uint8_t b6[] = {0x08, 0x82, 0x27}; send_data(b6, 3);
    send_cmd(0xF2); uint8_t f2[] = {0x00}; send_data(f2, 1);
    send_cmd(0x26); uint8_t gamma[] = {0x01}; send_data(gamma, 1);
    send_cmd(0xE0); uint8_t e0[] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00}; send_data(e0, 15);
    send_cmd(0xE1); uint8_t e1[] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F}; send_data(e1, 15);
    send_cmd(0x11); vTaskDelay(pdMS_TO_TICKS(120));
    send_cmd(0x29); vTaskDelay(pdMS_TO_TICKS(120));
}

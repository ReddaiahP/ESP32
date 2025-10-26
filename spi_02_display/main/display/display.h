#ifndef DISPLAY_H
#define DISPLAY_H

#include "driver/spi_master.h"
#include <stdint.h>

void display_gpio_init(void);
void display_spi_init(void);
void ili9341_init(void);
void fill_color(uint16_t color);

// New functions
void clear_screen(uint16_t color);
void clear_region(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void draw_image(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, const uint16_t *image_data);

#endif

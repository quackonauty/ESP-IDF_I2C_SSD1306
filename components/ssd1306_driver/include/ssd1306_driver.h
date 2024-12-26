#pragma once

#include <driver/i2c_master.h>
#include "esp_err.h"
#include "esp_log.h"
#include <string.h>
#include "freertos/FreeRTOS.h"

#define SSD1306_TAG "SSD1306 Driver"

#define I2C_MASTER_TIMEOUT_MS 1000

/**
 * @brief SSD1306 display wise type
 *
 * This enumeration defines the wise of the SSD1306 display.
 */
typedef enum
{
    SSD1306_TOP_TO_BOTTOM,
    SSD1306_BOTTOM_TO_TOP
} ssd1306_wise_t;

/**
 * @brief SSD1306 page type
 *
 * This structure stores the segments of a page in the SSD1306 display.
 */

typedef struct
{
    uint8_t *segment;
} ssd1306_page_t;

/**
 * @brief I2C SSD1306 handle type
 *
 * This structure stores the configuration of the SSD1306 display and the I2C master device.
 */

typedef struct
{
    i2c_master_dev_handle_t i2c_master_dev;
    uint8_t i2c_addr;
    uint32_t scl_speed_hz;
    uint8_t width;
    uint8_t height;
    uint8_t total_pages;
    ssd1306_page_t *page;
} i2c_ssd1306_handle_t;

esp_err_t i2c_ssd1306_init(i2c_ssd1306_handle_t *i2c_ssd1306, i2c_master_bus_handle_t i2c_master_bus, uint8_t i2c_addr, uint32_t i2c_scl_speed_hz, uint8_t width, uint8_t height, ssd1306_wise_t wise);
void i2c_ssd1306_buffer_check(i2c_ssd1306_handle_t *i2c_ssd1306);
void i2c_ssd1306_buffer_clear(i2c_ssd1306_handle_t *i2c_ssd1306);
void i2c_ssd1306_buffer_fill(i2c_ssd1306_handle_t *i2c_ssd1306, bool fill);
void i2c_ssd1306_buffer_fill_pixel(i2c_ssd1306_handle_t *i2c_ssd1306, uint8_t x, uint8_t y, bool fill);
void i2c_ssd1306_buffer_fill_space(i2c_ssd1306_handle_t *i2c_ssd1306, uint8_t x1, uint8_t x2, uint8_t y1, uint8_t y2, bool fill);
void i2c_ssd1306_buffer_text(i2c_ssd1306_handle_t *i2c_ssd1306, uint8_t x, uint8_t y, const char *text, bool invert);
void i2c_ssd1306_buffer_int(i2c_ssd1306_handle_t *i2c_ssd1306, uint8_t x, uint8_t y, int value, bool invert);
void i2c_ssd1306_buffer_float(i2c_ssd1306_handle_t *i2c_ssd1306, uint8_t x, uint8_t y, float value, uint8_t decimals, bool invert);
void i2c_ssd1306_buffer_image(i2c_ssd1306_handle_t *i2c_ssd1306, uint8_t x, uint8_t y, const uint8_t *image, uint8_t width, uint8_t height, bool invert);
void i2c_ssd1306_segment_to_ram(i2c_ssd1306_handle_t *i2c_ssd1306, uint8_t page, uint8_t segment);
void i2c_ssd1306_segments_to_ram(i2c_ssd1306_handle_t *i2c_ssd1306, uint8_t page, uint8_t initial_segment, uint8_t final_segment);
void i2c_ssd1306_page_to_ram(i2c_ssd1306_handle_t *i2c_ssd1306, uint8_t page);
void i2c_ssd1306_pages_to_ram(i2c_ssd1306_handle_t *i2c_ssd1306);
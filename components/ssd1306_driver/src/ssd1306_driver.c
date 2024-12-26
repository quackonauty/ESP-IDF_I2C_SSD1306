#include "ssd1306_driver.h"
#include "ssd1306_cmd.h"

/**
 * @brief Initialize the I2C SSD1306 driver device
 *
 * This function initializes the I2C SSD1306 master device.
 *
 * @param i2c_ssd1306 Pointer to the I2C SSD1306 handle.
 * @param i2c_master_bus Initialized I2C master bus handle.
 * @param i2c_addr I2C address of the SSD1306 device.
 * @param i2c_scl_speed_hz I2C SCL speed in Hz, maximum is 400kHz.
 * @param width Width of the SSD1306 display, maximum 128.
 * @param height Height of the SSD1306 display, between 16 and 64, multiple of 8.
 * @param wise Wise of the SSD1306 display.
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Invalid argument
 *     - ESP_ERR_NO_MEM Memory allocation failed
 *     - ESP_FAIL Failed
 */
esp_err_t i2c_ssd1306_init(i2c_ssd1306_handle_t *i2c_ssd1306, i2c_master_bus_handle_t i2c_master_bus, uint8_t i2c_addr, uint32_t i2c_scl_speed_hz, uint8_t width, uint8_t height, ssd1306_wise_t wise)
{
    if (i2c_scl_speed_hz > 400000 || width > 128 || height < 16 || height > 64 || height % 8 != 0)
        return ESP_ERR_INVALID_ARG;

    esp_err_t ret;
    i2c_device_config_t i2c_device_config = {
        .dev_addr_length = I2C_ADDR_BIT_7,
        .device_address = i2c_addr,
        .scl_speed_hz = i2c_scl_speed_hz,
    };
    ret = i2c_master_bus_add_device(i2c_master_bus, &i2c_device_config, &i2c_ssd1306->i2c_master_dev);
    if (ret != ESP_OK)
        return ret;
    else
        ESP_LOGI(SSD1306_TAG, "I2C SSD1306 device added successfully");

    uint8_t ssd1306_init_cmd[] = {
        OLED_CONTROL_BYTE_CMD,
        OLED_CMD_DISPLAY_OFF,
        OLED_CMD_SET_MUX_RATIO, (height - 1),
        OLED_CMD_SET_VERT_DISPLAY_OFFSET, 0x00,
        OLED_MASK_DISPLAY_START_LINE | 0x00,
        0x00,
        0x00,
        OLED_CMD_SET_COM_PIN_HARDWARE_MAP, 0x12,
        OLED_CMD_SET_MEMORY_ADDR_MODE, 0x02,
        OLED_CMD_SET_CONTRAST_CONTROL, 0xFF,
        OLED_CMD_SET_DISPLAY_CLK_DIVIDE, 0x80,
        OLED_CMD_ENABLE_DISPLAY_RAM,
        OLED_CMD_NORMAL_DISPLAY,
        OLED_CMD_SET_CHARGE_PUMP, 0x14,
        OLED_CMD_DISPLAY_ON};

    if (wise == SSD1306_TOP_TO_BOTTOM)
    {
        ssd1306_init_cmd[7] = OLED_CMD_COM_SCAN_DIRECTION_NORMAL;
        ssd1306_init_cmd[8] = OLED_CMD_SEGMENT_REMAP_LEFT_TO_RIGHT;
    }
    else if (wise == SSD1306_BOTTOM_TO_TOP)
    {
        ssd1306_init_cmd[7] = OLED_CMD_COM_SCAN_DIRECTION_REMAP;
        ssd1306_init_cmd[8] = OLED_CMD_SEGMENT_REMAP_RIGHT_TO_LEFT;
    }
    ret = i2c_master_transmit(i2c_ssd1306->i2c_master_dev, ssd1306_init_cmd, sizeof(ssd1306_init_cmd), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (ret != ESP_OK)
        return ret;
    else
        ESP_LOGI(SSD1306_TAG, "I2C SSD1306 device initialized successfully");

    i2c_ssd1306->i2c_addr = i2c_addr;
    i2c_ssd1306->scl_speed_hz = i2c_scl_speed_hz;
    i2c_ssd1306->width = width;
    i2c_ssd1306->height = height;
    i2c_ssd1306->total_pages = height / 8;

    i2c_ssd1306->page = (ssd1306_page_t *)calloc(i2c_ssd1306->total_pages, sizeof(ssd1306_page_t));
    if (i2c_ssd1306->page == NULL)
        return ESP_ERR_NO_MEM;

    for (uint8_t i = 0; i < i2c_ssd1306->total_pages; i++)
    {
        i2c_ssd1306->page[i].segment = (uint8_t *)calloc(width, sizeof(uint8_t));
        if (i2c_ssd1306->page[i].segment == NULL)
            return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(SSD1306_TAG, "I2C SSD1306 page allocated successfully");

    return ESP_OK;
}

/**
 * @brief Check the buffer of the SSD1306 device
 *
 * This function checks the buffer of the SSD1306 device by printing the hexadecimal values of each segment in each page.
 *
 * @param i2c_ssd1306 Pointer to the I2C SSD1306 handle.
 */
void i2c_ssd1306_buffer_check(i2c_ssd1306_handle_t *i2c_ssd1306)
{
    for (uint8_t i = 0; i < i2c_ssd1306->total_pages; i++)
    {
        for (uint8_t j = 0; j < i2c_ssd1306->width; j++)
        {
            printf("%02X ", i2c_ssd1306->page[i].segment[j]);
        }
        printf("\n");
    }
}

/**
 * @brief Clear the buffer of the SSD1306 device
 *
 * This function clears the buffer of the SSD1306 device, setting all segments to 0x00.
 *
 * @param i2c_ssd1306 Pointer to the I2C SSD1306 handle.
 */
void i2c_ssd1306_buffer_clear(i2c_ssd1306_handle_t *i2c_ssd1306)
{
    for (uint8_t i = 0; i < i2c_ssd1306->total_pages; i++)
    {
        memset(i2c_ssd1306->page[i].segment, 0x00, i2c_ssd1306->width);
    }
}

/**
 * @brief Fill the buffer of the SSD1306 device
 *
 * This function fills each segment in each page of the buffer of the SSD1306 device.
 *
 * @param i2c_ssd1306 Pointer to the I2C SSD1306 handle.
 * @param fill Fill the buffer with 0xFF if true, fill the buffer with 0x00 if false.
 */
void i2c_ssd1306_buffer_fill(i2c_ssd1306_handle_t *i2c_ssd1306, bool fill)
{
    for (uint8_t i = 0; i < i2c_ssd1306->total_pages; i++)
    {
        if (fill)
            memset(i2c_ssd1306->page[i].segment, 0xFF, i2c_ssd1306->width);
        else
            memset(i2c_ssd1306->page[i].segment, 0x00, i2c_ssd1306->width);
    }
}

/**
 * @brief Fill a pixel in the buffer of the SSD1306 device
 *
 * This function fills a pixel in the buffer of the SSD1306 device.
 *
 * @param i2c_ssd1306 Pointer to the I2C SSD1306 handle.
 * @param x X coordinate of the pixel.
 * @param y Y coordinate of the pixel.
 * @param fill Fill the pixel if true, clear the pixel if false.
 */
void i2c_ssd1306_buffer_fill_pixel(i2c_ssd1306_handle_t *i2c_ssd1306, uint8_t x, uint8_t y, bool fill)
{
    if (x >= i2c_ssd1306->width || y >= i2c_ssd1306->height)
    {
        ESP_LOGE(SSD1306_TAG, "Invalid pixel coordinates, 'x' must be between 0 and %d, 'y' must be between 0 and %d", i2c_ssd1306->width - 1, i2c_ssd1306->height - 1);
        return;
    }

    if (fill)
        i2c_ssd1306->page[y / 8].segment[x] |= (1 << (y % 8));
    else
        i2c_ssd1306->page[y / 8].segment[x] &= ~(1 << (y % 8));
}

/**
 * @brief Fill a range of pixels in the buffer of the SSD1306 device
 *
 * This function fills a range of pixels in the buffer of the SSD1306 device.
 *
 * @param i2c_ssd1306 Pointer to the I2C SSD1306 handle.
 * @param x1 X coordinate of the first pixel.
 * @param x2 X coordinate of the last pixel.
 * @param y1 Y coordinate of the first pixel.
 * @param y2 Y coordinate of the last pixel.
 * @param fill Fill the pixels if true, clear the pixels if false.
 */
void i2c_ssd1306_buffer_fill_space(i2c_ssd1306_handle_t *i2c_ssd1306, uint8_t x1, uint8_t x2, uint8_t y1, uint8_t y2, bool fill)
{
    if (x1 >= i2c_ssd1306->width || x2 >= i2c_ssd1306->width || y1 >= i2c_ssd1306->height || y2 >= i2c_ssd1306->height || x1 > x2 || y1 > y2)
    {
        ESP_LOGE(SSD1306_TAG, "Invalid space coordinates, 'x1' and 'x2' must be between 0 and %d, 'y1' and 'y2' must be between 0 and %d, 'x1' must be less than 'x2', 'y1' must be less than 'y2'", i2c_ssd1306->width - 1, i2c_ssd1306->height - 1);
        return;
    }

    for (uint8_t i = y1; i <= y2; i++)
    {
        for (uint8_t j = x1; j <= x2; j++)
        {
            if (fill)
                i2c_ssd1306->page[i / 8].segment[j] |= (1 << (i % 8));
            else
                i2c_ssd1306->page[i / 8].segment[j] &= ~(1 << (i % 8));
        }
    }
}

/**
 * @brief Copy 8x8 characters that represent a text to the buffer of the SSD1306 device
 *
 * This function writes a text to the buffer of the SSD1306 device.
 *
 * @param i2c_ssd1306 Pointer to the I2C SSD1306 handle.
 * @param x X coordinate of the text.
 * @param y Y coordinate of the text.
 * @param text Text to copy to the buffer.
 * @param invert Invert the text if true.
 */
void i2c_ssd1306_buffer_text(i2c_ssd1306_handle_t *i2c_ssd1306, uint8_t x, uint8_t y, const char *text, bool invert)
{
    if (x >= i2c_ssd1306->width || y >= i2c_ssd1306->height)
    {
        ESP_LOGE(SSD1306_TAG, "Invalid text coordinates, 'x' must be between 0 and %d, 'y' must be between 0 and %d", i2c_ssd1306->width - 1, i2c_ssd1306->height - 1);
        return;
    }

    uint8_t len = strlen(text);
    uint8_t page = y / 8;
    uint8_t y_offset = y % 8;
    if (y_offset == 0)
    {
        for (uint8_t i = 0; i < len; i++)
        {
            if (x + 8 > i2c_ssd1306->width)
            {
                ESP_LOGE(SSD1306_TAG, "Text exceeds the width of the display");
                return;
            }

            for (uint8_t j = 0; j < 8; j++)
            {
                if (invert)
                    i2c_ssd1306->page[page].segment[x + j] = ~font8x8[(uint8_t)text[i]][j];
                else
                    i2c_ssd1306->page[page].segment[x + j] = font8x8[(uint8_t)text[i]][j];
            }
            x += 8;
        }
    }
    else
    {
        if (page + 1 >= i2c_ssd1306->total_pages)
        {
            ESP_LOGE(SSD1306_TAG, "Text exceeds the height of the display");
            return;
        }

        for (uint8_t i = 0; i < len; i++)
        {
            if (x + 8 > i2c_ssd1306->width)
            {
                ESP_LOGE(SSD1306_TAG, "Text exceeds the width of the display");
                return;
            }
            for (uint8_t j = 0; j < 8; j++)
            {
                if (invert)
                {
                    i2c_ssd1306->page[page].segment[x + j] |= ~font8x8[(uint8_t)text[i]][j] << y_offset;
                    i2c_ssd1306->page[page + 1].segment[x + j] |= ~font8x8[(uint8_t)text[i]][j] >> (8 - y_offset);
                }
                else
                {
                    i2c_ssd1306->page[page].segment[x + j] |= font8x8[(uint8_t)text[i]][j] << y_offset;
                    i2c_ssd1306->page[page + 1].segment[x + j] |= font8x8[(uint8_t)text[i]][j] >> (8 - y_offset);
                }
            }
            x += 8;
        }
    }
}

/**
 * @brief Copy 8x8 characters that represent an integer to the buffer of the SSD1306 device
 *
 * This function writes an integer to the buffer of the SSD1306 device.
 *
 * @param i2c_ssd1306 Pointer to the I2C SSD1306 handle.
 * @param x X coordinate of the integer.
 * @param y Y coordinate of the integer.
 * @param value Integer to copy to the buffer.
 * @param invert Invert the integer if true.
 */
void i2c_ssd1306_buffer_int(i2c_ssd1306_handle_t *i2c_ssd1306, uint8_t x, uint8_t y, int value, bool invert)
{
    char text[16];
    sprintf(text, "%d", value);
    i2c_ssd1306_buffer_text(i2c_ssd1306, x, y, text, invert);
}

/**
 * @brief Copy 8x8 characters that represent a float to the buffer of the SSD1306 device
 *
 * This function writes a float to the buffer of the SSD1306 device.
 *
 * @param i2c_ssd1306 Pointer to the I2C SSD1306 handle.
 * @param x X coordinate of the float.
 * @param y Y coordinate of the float.
 * @param value Float to copy to the buffer.
 * @param decimals Number of decimal places.
 * @param invert Invert the float if true.
 */
void i2c_ssd1306_buffer_float(i2c_ssd1306_handle_t *i2c_ssd1306, uint8_t x, uint8_t y, float value, uint8_t decimals, bool invert)
{
    char text[16];
    sprintf(text, "%.*f", decimals, value);
    i2c_ssd1306_buffer_text(i2c_ssd1306, x, y, text, invert);
}

/**
 * @brief Copy an image to the buffer of the SSD1306 device
 *
 * This function copies an image to the buffer of the SSD1306 device.
 *
 * @param i2c_ssd1306 Pointer to the I2C SSD1306 handle.
 * @param x X coordinate of the image.
 * @param y Y coordinate of the image.
 * @param image Image to copy to the buffer.
 * @param width Width of the image.
 * @param height Height of the image.
 * @param invert Invert the image if true.
 */
void i2c_ssd1306_buffer_image(i2c_ssd1306_handle_t *i2c_ssd1306, uint8_t x, uint8_t y, const uint8_t *image, uint8_t width, uint8_t height, bool invert)
{
    if (x >= i2c_ssd1306->width || y >= i2c_ssd1306->height || width > i2c_ssd1306->width || height > i2c_ssd1306->height || x + width > i2c_ssd1306->width || y + height > i2c_ssd1306->height)
    {
        ESP_LOGE(SSD1306_TAG, "Invalid image coordinates, 'x' must be between 0 and %d, 'y' must be between 0 and %d, 'width' must be between 1 and %d, 'height' must be between 1 and %d, 'x + width' must be less than or equal to %d, 'y + height' must be less than or equal to %d", i2c_ssd1306->width - 1, i2c_ssd1306->height - 1, i2c_ssd1306->width, i2c_ssd1306->height, i2c_ssd1306->width, i2c_ssd1306->height);
        return;
    }

    uint8_t initial_page = y / 8;
    uint8_t final_page = (y + height - 1) / 8;
    uint8_t page_range = final_page - initial_page;
    uint8_t y_offset = y % 8;
    if (y_offset == 0)
    {
        for (uint8_t i = 0; i <= page_range; i++)
        {
            for (uint8_t j = 0; j < width; j++)
            {
                if (invert)
                    i2c_ssd1306->page[initial_page + i].segment[x + j] = ~image[i * width + j];
                else
                    i2c_ssd1306->page[initial_page + i].segment[x + j] = image[i * width + j];
            }
        }
    }
    else
    {
        for (uint8_t i = 0; i < page_range; i++)
        {
            for (uint8_t j = 0; j < width; j++)
            {
                if (invert)
                {
                    i2c_ssd1306->page[initial_page + i].segment[x + j] |= (~image[i * width + j] << y_offset) & (0xFF << y_offset);
                    i2c_ssd1306->page[initial_page + i + 1].segment[x + j] |= (~image[i * width + j] >> (8 - y_offset)) & (0xFF >> (8 - y_offset));
                }
                else
                {
                    i2c_ssd1306->page[initial_page + i].segment[x + j] |= (image[i * width + j] << y_offset) & (0xFF << y_offset);
                    i2c_ssd1306->page[initial_page + i + 1].segment[x + j] |= (image[i * width + j] >> (8 - y_offset)) & (0xFF >> (8 - y_offset));
                }
            }
        }
    }
}

/**
 * @brief Transfer a buffer segment to the RAM of the SSD1306 device
 *
 * This function transfers a buffer segment to the RAM of the SSD1306 device.
 *
 * @param i2c_ssd1306 Pointer to the I2C SSD1306 handle.
 * @param page Page number to transfer the buffer segment to the RAM.
 * @param segment Segment number to transfer the buffer segment to the RAM.
 */
void i2c_ssd1306_segment_to_ram(i2c_ssd1306_handle_t *i2c_ssd1306, uint8_t page, uint8_t segment)
{
    if (page >= i2c_ssd1306->total_pages)
    {
        ESP_LOGE(SSD1306_TAG, "Invalid page number, must be between 0 and %d", i2c_ssd1306->total_pages - 1);
        return;
    }

    if (segment >= i2c_ssd1306->width)
    {
        ESP_LOGE(SSD1306_TAG, "Invalid segment number, must be between 0 and %d", i2c_ssd1306->width - 1);
        return;
    }

    uint8_t ram_addr_cmd[] = {
        OLED_CONTROL_BYTE_CMD,
        OLED_MASK_PAGE_ADDR | page,
        OLED_MASK_LSB_NIBBLE_SEG_ADDR | (segment & 0x0F),
        OLED_MASK_HSB_NIBBLE_SEG_ADDR | (segment >> 4 & 0x0F)};
    ESP_ERROR_CHECK(i2c_master_transmit(i2c_ssd1306->i2c_master_dev, ram_addr_cmd, sizeof(ram_addr_cmd), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));

    uint8_t ram_data_cmd[] = {
        OLED_CONTROL_BYTE_DATA,
        i2c_ssd1306->page[page].segment[segment]};
    ESP_ERROR_CHECK(i2c_master_transmit(i2c_ssd1306->i2c_master_dev, ram_data_cmd, sizeof(ram_data_cmd), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
}

/**
 * @brief Transfer a range of buffer segments to the RAM of the SSD1306 device
 *
 * This function transfers a range of buffer segments to the RAM of the SSD1306 device.
 *
 * @param i2c_ssd1306 Pointer to the I2C SSD1306 handle.
 * @param page Page number to transfer the buffer segments to the RAM.
 * @param initial_segment Initial segment of the range to transfer the buffer to the RAM.
 * @param final_segment Final segment of the range to transfer the buffer to the RAM.
 */
void i2c_ssd1306_segments_to_ram(i2c_ssd1306_handle_t *i2c_ssd1306, uint8_t page, uint8_t initial_segment, uint8_t final_segment)
{
    if (page >= i2c_ssd1306->total_pages)
    {
        ESP_LOGE(SSD1306_TAG, "Invalid page number, must be between 0 and %d", i2c_ssd1306->total_pages - 1);
        return;
    }

    if (initial_segment >= i2c_ssd1306->width || final_segment >= i2c_ssd1306->width || initial_segment > final_segment)
    {
        ESP_LOGE(SSD1306_TAG, "Invalid segment range, must be between 0 and %d", i2c_ssd1306->width - 1);
        return;
    }

    uint8_t ram_addr_cmd[] = {
        OLED_CONTROL_BYTE_CMD,
        OLED_MASK_PAGE_ADDR | page,
        OLED_MASK_LSB_NIBBLE_SEG_ADDR | (initial_segment & 0x0F),
        OLED_MASK_HSB_NIBBLE_SEG_ADDR | (initial_segment >> 4 & 0x0F)};
    ESP_ERROR_CHECK(i2c_master_transmit(i2c_ssd1306->i2c_master_dev, ram_addr_cmd, sizeof(ram_addr_cmd), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));

    uint8_t ram_data_cmd[final_segment - initial_segment + 2];
    ram_data_cmd[0] = OLED_CONTROL_BYTE_DATA;
    for (uint8_t i = 0; i < final_segment - initial_segment + 1; i++)
    {
        ram_data_cmd[i + 1] = i2c_ssd1306->page[page].segment[initial_segment + i];
    }
    ESP_ERROR_CHECK(i2c_master_transmit(i2c_ssd1306->i2c_master_dev, ram_data_cmd, sizeof(ram_data_cmd), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
}

/**
 * @brief Transfer the buffer of a page to the RAM of the SSD1306 device
 *
 * This function transfers the buffer of a page to the RAM of the SSD1306 device.
 *
 * @param i2c_ssd1306 Pointer to the I2C SSD1306 handle.
 * @param page Page number to transfer the buffer to the RAM.
 */
void i2c_ssd1306_page_to_ram(i2c_ssd1306_handle_t *i2c_ssd1306, uint8_t page)
{
    if (page >= i2c_ssd1306->total_pages)
    {
        ESP_LOGE(SSD1306_TAG, "Invalid page number, must be between 0 and %d", i2c_ssd1306->total_pages - 1);
        return;
    }

    uint8_t ram_addr_cmd[] = {
        OLED_CONTROL_BYTE_CMD,
        OLED_MASK_PAGE_ADDR | page,
        OLED_MASK_LSB_NIBBLE_SEG_ADDR | (0x00 & 0x0F),
        OLED_MASK_HSB_NIBBLE_SEG_ADDR | (0x00 >> 4 & 0x0F)};
    ESP_ERROR_CHECK(i2c_master_transmit(i2c_ssd1306->i2c_master_dev, ram_addr_cmd, sizeof(ram_addr_cmd), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));

    uint8_t ram_data_cmd[i2c_ssd1306->width + 1];
    ram_data_cmd[0] = OLED_CONTROL_BYTE_DATA;
    for (uint8_t i = 0; i < i2c_ssd1306->width; i++)
    {
        ram_data_cmd[i + 1] = i2c_ssd1306->page[page].segment[i];
    }
    ESP_ERROR_CHECK(i2c_master_transmit(i2c_ssd1306->i2c_master_dev, ram_data_cmd, sizeof(ram_data_cmd), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
}

/**
 * @brief Transfer the buffer of all pages to the RAM of the SSD1306 device
 *
 * This function transfers the buffer of all pages to the RAM of the SSD1306 device.
 *
 * @param i2c_ssd1306 Pointer to the I2C SSD1306 handle.
 */
void i2c_ssd1306_pages_to_ram(i2c_ssd1306_handle_t *i2c_ssd1306)
{
    for (uint8_t i = 0; i < i2c_ssd1306->total_pages; i++)
    {
        i2c_ssd1306_page_to_ram(i2c_ssd1306, i);
    }
}
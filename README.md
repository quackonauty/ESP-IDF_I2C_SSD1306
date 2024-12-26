
# _I2C SSD1306 OLED Driver_

## I. Overview
This library is a lightweight and efficient driver designed to interface OLED displays using the SSD1306 controller. It supports a range of OLED modules, including 128x64, 128x32, 72x40, and 64x48 configurations. While this driver has been rigorously tested on an ESP32 DEV MODULE (38 and 30-pin variants) with a 128x64 OLED display (0.96"), it may require adjustments for other ESP boards or display configurations. Use with caution if testing on unverified hardware setups.

For more technical details, the datasheet for the SSD1306 controller can be found [here](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf).

### Component Background

This component is built upon a previous version that you can find [here](https://github.com/quackonaut/ESP-IDF-I2C_SSD1306). The primary difference between the two is that the earlier version relied on the legacy `i2c.h` header, which is now deprecated and will be removed in future. This new implementation uses the new driver `i2c_master.h`.

Legacy ESP-IDF I2C Driver:

- `i2c.h`, is the header file of legacy I2C APIs.

New ESP-IDF I2C Driver:

- `i2c_master.h`, is the header file that provides standard communication mode specific APIs (for apps using new driver with master mode).

- `i2c_slave.h`, is the header file that provides standard communication mode specific APIs (for apps using new driver with slave mode).

## II. Incorporating the Driver into Your Project

By following these steps, you can quickly get started and adapt the driver to your own projects.

### 1. Set up the Hardware

- **Connect the OLED Display**: By default, the ESP32 uses GPIO22 as SCL (clock) and GPIO21 as SDA (data) for I2C communication. Connect these pins to the corresponding SCL and SDA pins on your OLED module.

- **Optional Custom Pins**: If you wish to use other GPIO pins for SCL and SDA, ensure they are not reserved for other functions on your ESP32 board. Refer to the board's pinout diagram for details.

- **Pull-Up Resistors**: For stable I2C communication, ensure pull-up resistors (typically 4.7kΩ) are connected to both SCL and SDA lines. If the chosen GPIO pins do not have internal pull-ups, you will need to add external resistors.

### 2. Install the Driver

You can incorporate this driver into an existing project or start fresh by creating a new project using the ESP-IDF project template.

1. **Add the SSD1306 Driver**

    Once you have your project ready—whether it's an existing one or newly created—its structure will typically include a components folder. If this folder doesn't exist, you can create it manually.

    Inside the components folder, you will need to add a subfolder named ssd1306_driver, which is available in this repository, to house the driver files. Below is an example representation of how your directory structure should look for a new project:

    ```
    ├── main
    │   ├── CMakeLists.txt
    │   └── main.c
    ├── components
    │   └── ssd1306_driver
    │       ├── CMakeLists.txt
    │       ├── include
    │       │   ├── ssd1306_cmd.h
    │       │   └── ssd1306_driver.h
    │       └── src
    │           └── ssd1306_driver.c
    ├── CMakeLists.txt
    └── README.md
    ```
    Before using the driver, it is crucial to initialize an I2C master bus. To demonstrate this, I have provided a workflow in the main.c file within this repository. The example outlines how the library functions. Below is a summary of the workflow: 

2. **Initialize the I2C Master Bus and SSD1306 Driver**

    First, we need to initialize the `i2c_master_bus_handle_t`. I have created a function to handle this initialization. Next, we must create an instance of `i2c_ssd1306_handle_t`, which will allow us to implement all the features of this component. After that, initialize the structure using the `i2c_ssd1306_init` function. This function requires the OLED specifications as well as basic I2C communication settings. It should be enclosed in an `ESP_ERROR_CHECK` to ensure the security and integrity of the code, in case the driver cannot be initialized. Below is the code implementation along with the necessary parameters for the i2c_ssd1306_init function.

    ``` c
    #include <stdio.h>
    #include "esp_log.h"
    #include "driver/i2c_master.h"

    #include "ssd1306_driver.h"

    #define I2C_MASTER_TAG "I2C_MASTER"

    void i2c_master_bus_init(i2c_master_bus_handle_t *i2c_master_bus)
    {
        esp_err_t ret;
        i2c_master_bus_config_t i2c_master_bus_config = {
            .i2c_port = I2C_NUM_0,
            .scl_io_num = 22,
            .sda_io_num = 21,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true};
        ret = i2c_new_master_bus(&i2c_master_bus_config, i2c_master_bus);
        if (ret != ESP_OK)
            ESP_LOGE(I2C_MASTER_TAG, "Failed to initialize I2C master bus: %s", esp_err_to_name(ret));
        else
            ESP_LOGI(I2C_MASTER_TAG, "I2C master bus initialized successfully");
    }

    void app_main(void)
    {
        i2c_master_bus_handle_t i2c_master_bus;
        i2c_master_bus_init(&i2c_master_bus);

        i2c_ssd1306_handle_t i2c_ssd1306;
        ESP_ERROR_CHECK(i2c_ssd1306_init(&i2c_ssd1306, i2c_master_bus, 0x3C, 400000, 128, 64, SSD1306_TOP_TO_BOTTOM));
    }

    ```

3. **Creation of an Instance of `i2c_ssd1306_handle_t`**

    The `i2c_ssd1306_init` function initializes the I2C interface for the SSD1306 OLED display. It configures the display’s communication settings, sends the necessary initialization commands to the device, and allocates memory for managing the display's pages and segments. This function ensures that the device is correctly set up to display content based on the given configuration.

    **_Paramenters_**

    - `i2c_ssd1306`: Pointer to the SSD1306 handle. This structure will hold the initialized settings and memory allocations for the display.

    - `i2c_master_bus`: Handle to the already initialized I2C master bus. This bus will be used to communicate with the SSD1306 device.

    - `i2c_addr`: The I2C address of the SSD1306 device. This address is used to identify the device on the I2C bus.

    - `i2c_scl_speed_hz`: The speed of the I2C clock (SCL) in Hz. The maximum allowed speed is 400 kHz.

    - `width`: The width of the SSD1306 display in pixels. The maximum value is 128 pixels.

    - `height`: The height of the SSD1306 display in pixels. The value must be between 16 and 64 pixels, and it should be a multiple of 8.

    - `wise`: Defines the scanning direction of the SSD1306 display. The options are: SSD1306_TOP_TO_BOTTOM or SSD1306_BOTTOM_TO_TOP

    **_Return Values_**

    - `ESP_OK`: The function executed successfully, and the device was initialized.

    - `ESP_ERR_INVALID_ARG`: One or more of the provided arguments are invalid (e.g., speed, width, or height out of range).

    - `ESP_ERR_NO_MEM`: Memory allocation failed when creating the page and segment buffers.

    - `ESP_FAIL`: A general failure occurred during the initialization process, such as a communication error with the I2C device.

With the driver now included in your project structure, you are ready to implement it in your application. 

### 3. Functions and Methods of the Driver

The driver is designed to perform a wide range of tasks, from drawing individual pixels to printing text, integers, floating-point numbers, and even images. It operates by creating a structure `i2c_ssd1306_handle_t` that acts as an object, which contains an internal buffer composed of pages and segments. This buffer is dynamically sized based on the dimensions of the OLED display being used.

To update the display, the workflow is divided into two main groups of functions:

1. **Functions for Modifying the Internal Buffer**

    These functions allow you to manipulate the internal buffer, where all graphical and textual data is first written. The internal buffer represents the screen content in memory before it is displayed.

    - `i2c_ssd1306_buffer_check`: Prints the hexadecimal values of each segment in every page of the buffer to check its current state.

    - `i2c_ssd1306_buffer_clear`: Clears the buffer by setting all segments in all pages to 0x00.

    - `i2c_ssd1306_buffer_fill`: Fills the buffer with a specified value (0xFF for all bits set or 0x00 for all bits cleared).

    - `i2c_ssd1306_buffer_fill_pixel`: Fills or clears a specific pixel in the buffer at the given coordinates.

    - `i2c_ssd1306_buffer_fill_space`: Fills or clears a rectangular region of pixels within the buffer.

    - `i2c_ssd1306_buffer_text`: Copies a text string to the buffer, starting at the specified coordinates. Optionally inverts the text.

    - `i2c_ssd1306_buffer_int`: Copies an integer value to the buffer as text, starting at the specified coordinates.

    - `i2c_ssd1306_buffer_float`: Copies a floating-point value to the buffer as text, starting at the specified coordinates.

    - `i2c_ssd1306_buffer_image`:Copies an image to the buffer, starting at the specified coordinates. Optionally inverts the image.

2. **Functions for Transferring the Buffer to the SSD1306 RAM**

    Once the internal buffer is updated, this second group of functions is used to send the buffer data to the SSD1306 controller’s RAM. These functions are responsible for ensuring that the OLED display accurately reflects the contents of the internal buffer.

    - `i2c_ssd1306_segment_to_ram`: Transfers a specific segment of the page buffer to the SSD1306 device's RAM.

    - `i2c_ssd1306_segments_to_ram`: Transfers a range of segments (from initial_segment to final_segment) from a page buffer to the SSD1306 device's RAM.

    - `i2c_ssd1306_page_to_ram`: Transfers the entire page buffer to the SSD1306 device's RAM.

    - `i2c_ssd1306_pages_to_ram`: Transfers all page buffers to the SSD1306 device's RAM.


### 4. Driver Implementation

- ![example1](/md/example1.jpg)
- ![example2](/md/example2.jpg)
- ![example3](/md/example3.jpg)
## III. Convert an Image to a C Array for OLED Display with Python

The repository also contains a Python script that converts an image into a C array that can be used to display on an OLED screen. 

### 1. Requirements

Before using the script, make sure you have the following installed:

- **Python 3.x**
- **Pillow library**: This is used for image manipulation. Install it using:
    ```bash
    pip install pillow
    ```

### 2. Features
- **Automatic Thresholding**: The script uses Otsu's method to determine the optimal threshold for binarization, ensuring that the image retains the best contrast for monochrome displays.

- **Invert Colors Option**: You can invert the monochrome colors with a simple parameter to better suit your display needs.

- **C Array Formatting**: Automatically formats the output into a C array that is ready to use in your embedded application.

- **Customizable Resolution**: Supports arbitrary resizing to match the resolution of your display (e.g., 128x64 or 64x64).

### 3. How to Use
  1. Place your image in the desired path and note its resolution.

  2. Edit the Python script to point to the image's file path and set your desired output resolution.

  3. Run the script:

        ```bash
        python script.py
        ```
  4. The script outputs:
     - A preview of the processed image.
     - The byte array in Python format for debugging or visualization.
     - A formatted C array printed to the console.

### 4. Example Code and Usage
Here is an example of how to use the script:

```python
image_path = r"C:\Path\to\your\image.png"
width, height = 64, 64  # Set desired resolution

byte_array = image_to_byte_array(image_path, width, height)
c_array = format_as_c_array(byte_array)

# Print the byte array as a visualization
print_array(byte_array)

# Print the formatted C array
print(c_array)
```

### 5. Function Descriptions

- `image_to_byte_array`: Converts an image into a 2D byte array representation suitable for monochrome OLED displays.

    **_Parameters_**:

    - **image_path (str)**: Path to the image file.
    - **width (int)**: Output image width (e.g., 128 for SSD1306).
    - **height (int)**: Output image height (e.g., 64 for SSD1306).
    - **invert (bool, optional)**: Inverts black and white in the output image. Default is False.
    - **threshold (int, optional)**: Manually set binarization threshold (0-255). Default is None (automatic calculation).
    - **Returns**: A 2D list of integers representing the image in byte form.

- `format_as_c_array`: Formats a 2D byte array into a C array string.

    **_Parameters:_**

    - **py_array (List[List[int]])**: The input 2D byte array.
    - **c_array_name (str, optional)**: The name of the C array. Default is "img".
    - **line_break (int, optional)**: Number of bytes per line in the output. Default is 16.
    - **Returns**: A string representing the formatted C array.

- `print_array`: Visualizes a 2D byte array in the console, simulating an OLED display.

    **_Parameters:_**

    - **py_array (List[List[int]])**: The 2D byte array to visualize.
    - **Returns**: None.

### 6. Notes
- The default maximum resolution for the SSD1306 display is 128x64. Ensure that the width and height values align with your display specifications.
- The invert parameter is especially useful for displays with inverted logic (e.g., white-on-black vs. black-on-white).
- This script is ideal for embedded projects that require static or semi-static images, such as logos or icons.

## **Do you have any questions, suggestions, or have you found any errors?**

If you have any questions or suggestions about the operation of the component, or if you encountered any errors while compiling it on your ESP32 board, please don't hesitate to leave your comment.

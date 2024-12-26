from PIL import Image


def print_array(py_array):
    """
    Prints a 2D byte array as a monochrome image, simulating the OLED display.

    @param py_array: List[List[int]]
        A 2D list of byte blocks representing the monochrome image.

    @return: None
    """
    for array in py_array:
        segment = ["" for _ in range(8)]

        for byte in array:
            bin = format(byte, "08b")
            for i in range(8):
                segment[7 - i] += "█" if bin[i] == "1" else " "

        for linea in segment:
            print(linea)


def format_as_c_array(py_array, c_array_name="img", line_break=16):
    """
    Formats a 2D byte array as a C array string.

    @param py_array: List[List[int]]
        A 2D list of byte blocks representing the monochrome image.
    @param c_array_name: str, optional
        Name of the C array. Defaults to "img".
    @param line_break: int, optional
        Number of bytes per line. Defaults to 16.

    @return: str
        A string representing the C array.
    """
    rows = len(py_array)
    cols = len(py_array[0]) if rows > 0 else 0

    c_array = f"uint8_t {c_array_name}[{rows}][{cols}] = {{\n"
    for row in byte_array:
        c_array += "    {"
        for i in range(0, len(row), line_break):
            chunk = row[i : i + line_break]
            c_array += ", ".join(f"0x{byte:02X}" for byte in chunk)
            if i + line_break < len(row):
                c_array += ",\n     "
        c_array += "},\n"
    c_array = c_array.rstrip(",\n") + "\n};"
    return c_array


def image_to_byte_array(image_path, width, height, invert=False, threshold=None):
    """
    Converts an image to a byte array representation for use in monochrome displays.

    @param image_path: str
        Path to the image file.
    @param width: int
        Width of the output image, maximun for SSD1306 is 128.
    @param height: int
        Height of the output image, maximun for SSD1306 is 64.
    @param invert: bool, optional
        Whether to invert the colors in the output. Defaults to False.
    @param threshold: int, optional
        Threshold for binarization, from 0 to 255. If None, it is calculated automatically.

    @return: List[List[int]]
        A 2D list of byte blocks representing the monochrome image.
    """
    image = Image.open(image_path).convert("L")
    image = image.resize((width, height))

    if threshold is None:
        histogram = image.histogram()
        total_pixels = sum(histogram)
        sum_brightness = sum(i * histogram[i] for i in range(256))
        sum_b = 0
        max_variance = 0
        threshold = 0
        w_b = 0

        for t in range(256):
            w_b += histogram[t]
            w_f = total_pixels - w_b
            if w_b == 0 or w_f == 0:
                continue

            sum_b += t * histogram[t]
            m_b = sum_b / w_b
            m_f = (sum_brightness - sum_b) / w_f
            variance_between = w_b * w_f * (m_b - m_f) ** 2

            if variance_between > max_variance:
                max_variance = variance_between
                threshold = t

    print(f"Threshold: {threshold}")

    image = image.point(lambda p: 255 if p > threshold else 0, mode="1")
    if invert:
        image = image.point(lambda p: 255 if p == 0 else 0, mode="1")
    image.show()

    pixels = list(image.getdata())

    rows = [pixels[i * width : (i + 1) * width] for i in range(height)]

    byte_blocks = []
    for group_start in range(0, height, 8):
        group = rows[group_start : group_start + 8]
        block = []
        for col in range(width):
            byte = 0
            for row_idx, row in enumerate(group):
                if row[col] == 0:
                    byte |= 1 << row_idx
            block.append(byte)
        byte_blocks.append(block)

    return byte_blocks


image_path = r"C:\Path\to\your\image.png"
width, height = 64, 64

byte_array = image_to_byte_array(image_path, width, height, True)
c_array = format_as_c_array(byte_array)
print_array(byte_array)
print(c_array)

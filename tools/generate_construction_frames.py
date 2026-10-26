import struct
import sys
from pathlib import Path


PALETTE_OFFSET = 48
PALETTE_SIZE = 768
TRANSPARENT_RGB = (255, 0, 255)


def read_exact(file, size):
    data = file.read(size)

    if len(data) != size:
        raise EOFError(
            f"Ocekavano {size} B, nacteno {len(data)} B"
        )

    return data


def load_rx_sprite(rx_path, target_id):
    with open(rx_path, "rb") as file:
        sprite_count = struct.unpack(
            "<I",
            read_exact(file, 4)
        )[0]

        used = read_exact(file, sprite_count)

        for sprite_id in range(1, sprite_count + 1):
            if used[sprite_id - 1] != 1:
                continue

            width, height = struct.unpack(
                "<hh",
                read_exact(file, 4)
            )

            pivot_x, pivot_y = struct.unpack(
                "<ii",
                read_exact(file, 8)
            )

            pixels = read_exact(
                file,
                width * height
            )

            if sprite_id == target_id:
                return {
                    "id": sprite_id,
                    "width": width,
                    "height": height,
                    "pivot_x": pivot_x,
                    "pivot_y": pivot_y,
                    "pixels": pixels,
                }

    raise ValueError(
        f"Sprite ID {target_id} nebyl nalezen"
    )


def load_palette(path):
    data = Path(path).read_bytes()

    palette_data = data[
        PALETTE_OFFSET:
        PALETTE_OFFSET + PALETTE_SIZE
    ]

    if len(palette_data) != PALETTE_SIZE:
        raise ValueError("Neplatna paleta")

    return [
        tuple(palette_data[index:index + 3])
        for index in range(0, PALETTE_SIZE, 3)
    ]


def write_bmp(path, width, height, rgb_pixels):
    row_size = ((width * 3 + 3) // 4) * 4
    pixel_data_size = row_size * height
    file_size = 54 + pixel_data_size

    with open(path, "wb") as output:
        output.write(b"BM")
        output.write(
            struct.pack("<IHHI", file_size, 0, 0, 54)
        )

        output.write(
            struct.pack(
                "<IIIHHIIIIII",
                40,
                width,
                height,
                1,
                24,
                0,
                pixel_data_size,
                2835,
                2835,
                0,
                0
            )
        )

        padding = b"\x00" * (
            row_size - width * 3
        )

        for y in range(height - 1, -1, -1):
            for x in range(width):
                red, green, blue = rgb_pixels[
                    y * width + x
                ]

                output.write(
                    bytes((blue, green, red))
                )

            output.write(padding)


def generate_frames(
    rx_path,
    palette_path,
    rgb_id,
    mask_id,
    step_count,
    output_dir
):
    palette = load_palette(palette_path)
    graphic = load_rx_sprite(rx_path, rgb_id)
    mask = load_rx_sprite(rx_path, mask_id)

    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    offset_x = (
        mask["pivot_x"] - graphic["pivot_x"]
    )

    offset_y = (
        mask["pivot_y"] - graphic["pivot_y"]
    )

    print(
        f"RGB {rgb_id}: "
        f"{graphic['width']}x{graphic['height']}, "
        f"pivot {graphic['pivot_x']},"
        f"{graphic['pivot_y']}"
    )

    print(
        f"Mask {mask_id}: "
        f"{mask['width']}x{mask['height']}, "
        f"pivot {mask['pivot_x']},"
        f"{mask['pivot_y']}"
    )

    print(
        f"Posun masky do RGB: "
        f"{offset_x}, {offset_y}"
    )

    # Kontrolni faze vcetne zacatku a konce.
    requested_steps = {
        0,
        1,
        step_count // 4,
        step_count // 2,
        (step_count * 3) // 4,
        step_count,
    }

    for current_step in sorted(requested_steps):
        output_pixels = [
            TRANSPARENT_RGB
            for _ in range(
                graphic["width"] *
                graphic["height"]
            )
        ]

        for mask_y in range(mask["height"]):
            for mask_x in range(mask["width"]):
                mask_value = mask["pixels"][
                    mask_y * mask["width"] + mask_x
                ]

                if mask_value == 0:
                    continue

                # Hodnota masky urcuje stavebni krok.
                if mask_value >= current_step:
                    continue

                graphic_x = mask_x + offset_x
                graphic_y = mask_y + offset_y

                if (
                    graphic_x < 0 or
                    graphic_y < 0 or
                    graphic_x >= graphic["width"] or
                    graphic_y >= graphic["height"]
                ):
                    continue

                graphic_index = (
                    graphic_y * graphic["width"]
                    + graphic_x
                )

                palette_index = graphic["pixels"][
                    graphic_index
                ]

                if palette_index == 0:
                    continue

                output_pixels[graphic_index] = (
                    palette[palette_index]
                )

        output_path = (
            output_dir
            / f"step_{current_step:03}.bmp"
        )

        write_bmp(
            output_path,
            graphic["width"],
            graphic["height"],
            output_pixels
        )

        print(f"Vytvoren: {output_path}")


if __name__ == "__main__":
    if len(sys.argv) != 7:
        raise SystemExit(
            "Pouziti:\n"
            "generate_construction_frames.py "
            "houses.rx pal0.bbm "
            "rgb_id mask_id step_count vystupni_slozka"
        )

    generate_frames(
        sys.argv[1],
        sys.argv[2],
        int(sys.argv[3]),
        int(sys.argv[4]),
        int(sys.argv[5]),
        sys.argv[6]
    )

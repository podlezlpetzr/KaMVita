import struct
import sys
from pathlib import Path

TRANSPARENT_RGB = (255, 0, 255)
PALETTE_OFFSET = 48
PALETTE_SIZE = 768


def read_exact(file, size):
    data = file.read(size)

    if len(data) != size:
        raise EOFError(
            f"Neocekavany konec souboru: "
            f"ocekavano {size} B, nacteno {len(data)} B"
        )

    return data


def load_palette(path):
    data = Path(path).read_bytes()

    palette_data = data[
        PALETTE_OFFSET:
        PALETTE_OFFSET + PALETTE_SIZE
    ]

    if len(palette_data) != PALETTE_SIZE:
        raise ValueError(
            f"Neplatna paleta: nacteno {len(palette_data)} B"
        )

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

        padding = b"\x00" * (row_size - width * 3)

        for y in range(height - 1, -1, -1):
            for x in range(width):
                red, green, blue = rgb_pixels[
                    y * width + x
                ]

                output.write(
                    bytes((blue, green, red))
                )

            output.write(padding)


def extract_sprite(
    rx_path,
    palette_path,
    sprite_id,
    output_path
):
    palette = load_palette(palette_path)

    with open(rx_path, "rb") as file:
        house_qty = struct.unpack(
            "<I",
            read_exact(file, 4)
        )[0]

        used_flags = read_exact(file, house_qty)

        if sprite_id < 1 or sprite_id > house_qty:
            raise ValueError(
                f"ID {sprite_id} je mimo rozsah "
                f"1 az {house_qty}"
            )

        for current_id in range(1, house_qty + 1):
            if used_flags[current_id - 1] != 1:
                continue

            width, height = struct.unpack(
                "<hh",
                read_exact(file, 4)
            )

            pivot_x, pivot_y = struct.unpack(
                "<ii",
                read_exact(file, 8)
            )

            pixel_count = width * height

            if width <= 0 or height <= 0:
                raise ValueError(
                    f"Neplatny sprite {current_id}: "
                    f"{width} x {height}"
                )

            pixel_indices = read_exact(
                file,
                pixel_count
            )

            if current_id != sprite_id:
                continue

            rgb_pixels = []

            for palette_index in pixel_indices:
                if palette_index == 0:
                    rgb_pixels.append(
                        TRANSPARENT_RGB
                    )
                else:
                    rgb_pixels.append(
                        palette[palette_index]
                    )

            output_path = Path(output_path)
            output_path.parent.mkdir(
                parents=True,
                exist_ok=True
            )

            write_bmp(
                output_path,
                width,
                height,
                rgb_pixels
            )

            metadata_path = output_path.with_suffix(
                ".txt"
            )

            metadata_path.write_text(
                f"id={current_id}\n"
                f"width={width}\n"
                f"height={height}\n"
                f"pivot_x={pivot_x}\n"
                f"pivot_y={pivot_y}\n",
                encoding="utf-8"
            )

            print(f"Sprite ID: {current_id}")
            print(f"Rozmery: {width} x {height}")
            print(f"Pivot: {pivot_x}, {pivot_y}")
            print(f"BMP: {output_path}")
            print(f"Metadata: {metadata_path}")
            return

    raise ValueError(
        f"Sprite ID {sprite_id} nebyl nalezen"
    )


if __name__ == "__main__":
    if len(sys.argv) != 5:
        raise SystemExit(
            "Pouziti:\n"
            "python3 extract_house.py "
            "houses.rx pal0.bbm sprite_id vystup.bmp"
        )

    extract_sprite(
        sys.argv[1],
        sys.argv[2],
        int(sys.argv[3]),
        sys.argv[4]
    )

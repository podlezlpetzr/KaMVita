import struct
import sys
from pathlib import Path

SPRITE_ID = 138
TRANSPARENT_RGB = (255, 0, 255)

def read_exact(file, size):
    data = file.read(size)
    if len(data) != size:
        raise EOFError(
            f"Neocekavany konec souboru: "
            f"ocekavano {size} B, nacteno {len(data)} B"
        )
    return data

def write_bmp(path, width, height, rgb_pixels):
    row_size = ((width * 3 + 3) // 4) * 4
    pixel_data_size = row_size * height
    file_size = 54 + pixel_data_size

    with open(path, "wb") as out:
        out.write(b"BM")
        out.write(struct.pack("<IHHI", file_size, 0, 0, 54))

        out.write(struct.pack(
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
        ))

        padding = b"\x00" * (row_size - width * 3)

        # BMP uklada radky zdola nahoru a kanaly jako BGR
        for y in range(height - 1, -1, -1):
            for x in range(width):
                r, g, b = rgb_pixels[y * width + x]
                out.write(bytes((b, g, r)))

            out.write(padding)

def extract(rx_path, palette_path, output_path):
    palette_file = Path(palette_path).read_bytes()
    palette_data = palette_file[48:48 + 768]

    if len(palette_data) < 768:
        raise ValueError(
            f"Paleta je prilis kratka: {len(palette_data)} B"
        )

    palette = [
        tuple(palette_data[index:index + 3])
        for index in range(0, 768, 3)
    ]

    with open(rx_path, "rb") as file:
        house_qty = struct.unpack("<I", read_exact(file, 4))[0]
        used_flags = read_exact(file, house_qty)

        for sprite_id in range(1, house_qty + 1):
            if used_flags[sprite_id - 1] != 1:
                continue

            width, height = struct.unpack(
                "<hh",
                read_exact(file, 4)
            )

            pivot_x, pivot_y = struct.unpack(
                "<ii",
                read_exact(file, 8)
            )

            pixel_indices = read_exact(file, width * height)

            if sprite_id != SPRITE_ID:
                continue

            rgb_pixels = []

            for palette_index in pixel_indices:
                if palette_index == 0:
                    rgb_pixels.append(TRANSPARENT_RGB)
                else:
                    rgb_pixels.append(palette[palette_index])

            write_bmp(
                output_path,
                width,
                height,
                rgb_pixels
            )

            print(f"Sprite ID: {sprite_id}")
            print(f"Rozmery: {width} x {height}")
            print(f"Pivot: {pivot_x}, {pivot_y}")
            print(f"Vystup: {output_path}")
            return

    raise ValueError(f"Sprite {SPRITE_ID} nebyl nalezen")

if __name__ == "__main__":
    if len(sys.argv) != 4:
        raise SystemExit(
            "Pouziti: extract_house_sprite.py "
            "houses.rx pal0.bbm vystup.bmp"
        )

    extract(sys.argv[1], sys.argv[2], sys.argv[3])

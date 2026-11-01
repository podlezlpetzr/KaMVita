import struct
import sys
from pathlib import Path

TRANSPARENT_RGB = (255, 0, 255)

TARGET_IDS = {49, *range(351, 379)}


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

    # KaM BBM paleta pouzivana stejne jako v existujicim
    # tools/extract_house_sprite.py
    palette_data = data[48:48 + 768]

    if len(palette_data) != 768:
        raise ValueError(
            f"Neplatna paleta: potrebuji 768 B, mam {len(palette_data)} B"
        )

    return [
        tuple(palette_data[i:i + 3])
        for i in range(0, 768, 3)
    ]


def write_bmp(path, width, height, pixels, palette):
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

        for y in range(height - 1, -1, -1):
            for x in range(width):
                index = pixels[y * width + x]

                if index == 0:
                    r, g, b = TRANSPARENT_RGB
                else:
                    r, g, b = palette[index]

                out.write(bytes((b, g, r)))

            out.write(padding)


def extract(rx_path, palette_path, output_dir):
    rx_path = Path(rx_path)
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    palette = load_palette(palette_path)

    found = set()

    with rx_path.open("rb") as file:
        sprite_count = struct.unpack(
            "<I",
            read_exact(file, 4)
        )[0]

        flags = read_exact(file, sprite_count)

        print(f"GUI sprite count: {sprite_count}")
        print(f"Used sprites: {sum(v == 1 for v in flags)}")
        print()

        for sprite_id in range(1, sprite_count + 1):
            if flags[sprite_id - 1] != 1:
                continue

            width, height = struct.unpack(
                "<hh",
                read_exact(file, 4)
            )

            pivot_x, pivot_y = struct.unpack(
                "<ii",
                read_exact(file, 8)
            )

            if width <= 0 or height <= 0:
                raise ValueError(
                    f"Neplatne rozmery sprite {sprite_id}: "
                    f"{width}x{height}"
                )

            pixels = read_exact(file, width * height)

            if sprite_id not in TARGET_IDS:
                continue

            bmp_path = output_dir / f"gui_{sprite_id:04d}.bmp"
            txt_path = output_dir / f"gui_{sprite_id:04d}.txt"

            write_bmp(
                bmp_path,
                width,
                height,
                pixels,
                palette
            )

            txt_path.write_text(
                f"id={sprite_id}\n"
                f"width={width}\n"
                f"height={height}\n"
                f"pivot_x={pivot_x}\n"
                f"pivot_y={pivot_y}\n",
                encoding="utf-8"
            )

            nonzero = sum(pixel != 0 for pixel in pixels)

            print(
                f"ID={sprite_id:3d} "
                f"SIZE={width}x{height} "
                f"PIVOT={pivot_x},{pivot_y} "
                f"NONZERO={nonzero} "
                f"-> {bmp_path}"
            )

            found.add(sprite_id)

    print()
    print("=== SUMMARY ===")

    for sprite_id in sorted(TARGET_IDS):
        state = "OK" if sprite_id in found else "MISSING"
        print(f"{sprite_id:3d}: {state}")

    missing = TARGET_IDS - found

    if missing:
        raise SystemExit(
            "Chybi sprite ID: " +
            ", ".join(str(i) for i in sorted(missing))
        )


if __name__ == "__main__":
    if len(sys.argv) != 4:
        raise SystemExit(
            "Pouziti: extract_gui_wares.py "
            "gui.rx pal0.bbm vystupni_adresar"
        )

    extract(sys.argv[1], sys.argv[2], sys.argv[3])

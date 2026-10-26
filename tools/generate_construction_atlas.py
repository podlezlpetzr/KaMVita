import math
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

            pixels = read_exact(file, width * height)

            if sprite_id == target_id:
                return {
                    "id": sprite_id,
                    "width": width,
                    "height": height,
                    "pivot_x": pivot_x,
                    "pivot_y": pivot_y,
                    "pixels": pixels,
                }

    raise ValueError(f"Sprite {target_id} nebyl nalezen")


def load_palette(path):
    raw = Path(path).read_bytes()
    palette_data = raw[
        PALETTE_OFFSET:
        PALETTE_OFFSET + PALETTE_SIZE
    ]

    if len(palette_data) != PALETTE_SIZE:
        raise ValueError("Neplatna paleta")

    return [
        tuple(palette_data[i:i + 3])
        for i in range(0, PALETTE_SIZE, 3)
    ]


def write_bmp(path, width, height, pixels):
    row_size = ((width * 3 + 3) // 4) * 4
    image_size = row_size * height
    file_size = 54 + image_size

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
                image_size,
                2835,
                2835,
                0,
                0
            )
        )

        padding = b"\x00" * (row_size - width * 3)

        for y in range(height - 1, -1, -1):
            for x in range(width):
                red, green, blue = pixels[
                    y * width + x
                ]
                output.write(bytes((blue, green, red)))

            output.write(padding)


def generate_frame(graphic, mask, palette, step):
    output = [
        TRANSPARENT_RGB
        for _ in range(
            graphic["width"] * graphic["height"]
        )
    ]

    offset_x = mask["pivot_x"] - graphic["pivot_x"]
    offset_y = mask["pivot_y"] - graphic["pivot_y"]

    for mask_y in range(mask["height"]):
        for mask_x in range(mask["width"]):
            mask_value = mask["pixels"][
                mask_y * mask["width"] + mask_x
            ]

            if mask_value == 0:
                continue

            if mask_value >= step:
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

            output[graphic_index] = palette[palette_index]

    return output


def generate_atlas(
    rx_path,
    palette_path,
    graphic_id,
    mask_id,
    step_count,
    columns,
    output_path
):
    palette = load_palette(palette_path)
    graphic = load_rx_sprite(rx_path, graphic_id)
    mask = load_rx_sprite(rx_path, mask_id)

    frame_width = graphic["width"]
    frame_height = graphic["height"]

    # Obsahuje krok 0 i kompletni krok step_count.
    frame_count = step_count + 1
    rows = math.ceil(frame_count / columns)

    atlas_width = frame_width * columns
    atlas_height = frame_height * rows

    atlas = [
        TRANSPARENT_RGB
        for _ in range(atlas_width * atlas_height)
    ]

    for step in range(frame_count):
        frame = generate_frame(
            graphic,
            mask,
            palette,
            step
        )

        frame_column = step % columns
        frame_row = step // columns

        destination_x = frame_column * frame_width
        destination_y = frame_row * frame_height

        for y in range(frame_height):
            for x in range(frame_width):
                atlas_index = (
                    (destination_y + y) * atlas_width
                    + destination_x + x
                )

                frame_index = y * frame_width + x
                atlas[atlas_index] = frame[frame_index]

    output_path = Path(output_path)
    output_path.parent.mkdir(
        parents=True,
        exist_ok=True
    )

    write_bmp(
        output_path,
        atlas_width,
        atlas_height,
        atlas
    )

    metadata_path = output_path.with_suffix(".txt")
    metadata_path.write_text(
        f"graphic_id={graphic_id}\n"
        f"mask_id={mask_id}\n"
        f"frame_width={frame_width}\n"
        f"frame_height={frame_height}\n"
        f"frame_count={frame_count}\n"
        f"step_count={step_count}\n"
        f"columns={columns}\n"
        f"rows={rows}\n"
        f"pivot_x={graphic['pivot_x']}\n"
        f"pivot_y={graphic['pivot_y']}\n",
        encoding="utf-8"
    )

    print(f"Atlas: {output_path}")
    print(f"Rozmer atlasu: {atlas_width} x {atlas_height}")
    print(f"Snimek: {frame_width} x {frame_height}")
    print(f"Pocet snimku: {frame_count}")
    print(f"Mrizka: {columns} x {rows}")
    print(f"Metadata: {metadata_path}")


if __name__ == "__main__":
    if len(sys.argv) != 8:
        raise SystemExit(
            "Pouziti:\n"
            "generate_construction_atlas.py "
            "houses.rx pal0.bbm "
            "graphic_id mask_id step_count "
            "columns vystup.bmp"
        )

    generate_atlas(
        sys.argv[1],
        sys.argv[2],
        int(sys.argv[3]),
        int(sys.argv[4]),
        int(sys.argv[5]),
        int(sys.argv[6]),
        sys.argv[7]
    )

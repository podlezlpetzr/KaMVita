from pathlib import Path

from generate_construction_atlas import (
    TRANSPARENT_RGB,
    load_palette,
    load_rx_sprite,
    write_bmp,
)

ROOT = Path.home() / "KaMVita"

RX_PATH = (
    ROOT /
    "resources/original_kam/houses.rx"
)

PALETTE_PATH = (
    ROOT /
    "resources/original_kam/pal0.bbm"
)

OUTPUT_DIR = (
    ROOT /
    "assets/buildings/bakery/runtime"
)

FLAGPOLE_ID = 1200

FLAG1_IDS = [
    1181,
    1182,
    1183,
    1204,
    1205,
    1206,
    1207,
    1208,
    1179,
    1180,
]

FLAGPOLE_MOVE = (54, -64)
FLAG1_MOVE = (103, -56)


def write_single_sprite(
    name,
    sprite_id,
    move,
    palette,
):
    frame = load_rx_sprite(
        RX_PATH,
        sprite_id
    )

    width = frame["width"]
    height = frame["height"]

    pixels = [
        TRANSPARENT_RGB
        for _ in range(width * height)
    ]

    for y in range(height):
        for x in range(width):
            src = y * width + x

            palette_index = (
                frame["pixels"][src]
            )

            if palette_index == 0:
                continue

            pixels[src] = (
                palette[palette_index]
            )

    bmp_path = (
        OUTPUT_DIR /
        f"{name}.bmp"
    )

    write_bmp(
        bmp_path,
        width,
        height,
        pixels
    )

    txt_path = (
        OUTPUT_DIR /
        f"{name}.txt"
    )

    txt_path.write_text(
        "\n".join([
            f"id={frame['id']}",
            f"width={width}",
            f"height={height}",
            f"pivot_x={frame['pivot_x']}",
            f"pivot_y={frame['pivot_y']}",
            f"move_x={move[0]}",
            f"move_y={move[1]}",
            "source=original houses.rx",
        ]) + "\n",
        encoding="utf-8"
    )

    print(f"{name.upper()}={bmp_path}")
    print(
        f"{name.upper()}_SIZE="
        f"{width}x{height}"
    )
    print(
        f"{name.upper()}_PIVOT="
        f"{frame['pivot_x']},"
        f"{frame['pivot_y']}"
    )


def write_flag_atlas(
    name,
    sprite_ids,
    move,
    palette,
):
    frames = [
        load_rx_sprite(
            RX_PATH,
            sprite_id
        )
        for sprite_id in sprite_ids
    ]

    min_x = min(
        f["pivot_x"]
        for f in frames
    )

    min_y = min(
        f["pivot_y"]
        for f in frames
    )

    max_x = max(
        f["pivot_x"] +
        f["width"]
        for f in frames
    )

    max_y = max(
        f["pivot_y"] +
        f["height"]
        for f in frames
    )

    cell_width = max_x - min_x
    cell_height = max_y - min_y

    atlas_width = (
        cell_width *
        len(frames)
    )

    atlas_height = cell_height

    atlas = [
        TRANSPARENT_RGB
        for _ in range(
            atlas_width *
            atlas_height
        )
    ]

    for frame_index, frame in enumerate(
        frames
    ):
        origin_x = (
            frame_index *
            cell_width +
            frame["pivot_x"] -
            min_x
        )

        origin_y = (
            frame["pivot_y"] -
            min_y
        )

        for y in range(frame["height"]):
            for x in range(frame["width"]):
                src = (
                    y *
                    frame["width"] +
                    x
                )

                palette_index = (
                    frame["pixels"][src]
                )

                if palette_index == 0:
                    continue

                dst_x = origin_x + x
                dst_y = origin_y + y

                dst = (
                    dst_y *
                    atlas_width +
                    dst_x
                )

                atlas[dst] = (
                    palette[palette_index]
                )

    bmp_path = (
        OUTPUT_DIR /
        f"{name}.bmp"
    )

    write_bmp(
        bmp_path,
        atlas_width,
        atlas_height,
        atlas
    )

    txt_path = (
        OUTPUT_DIR /
        f"{name}.txt"
    )

    lines = [
        f"cell_width={cell_width}",
        f"cell_height={cell_height}",
        f"frame_count={len(frames)}",
        f"anchor_x={min_x}",
        f"anchor_y={min_y}",
        f"move_x={move[0]}",
        f"move_y={move[1]}",
        "source=original houses.rx",
    ]

    for i, frame in enumerate(frames):
        prefix = f"frame_{i:02d}"

        lines.extend([
            f"{prefix}_id={frame['id']}",
            f"{prefix}_width={frame['width']}",
            f"{prefix}_height={frame['height']}",
            f"{prefix}_pivot_x={frame['pivot_x']}",
            f"{prefix}_pivot_y={frame['pivot_y']}",
        ])

    txt_path.write_text(
        "\n".join(lines) + "\n",
        encoding="utf-8"
    )

    print(f"{name.upper()}={bmp_path}")
    print(
        f"{name.upper()}_CELL="
        f"{cell_width}x{cell_height}"
    )
    print(
        f"{name.upper()}_ANCHOR="
        f"{min_x},{min_y}"
    )


def main():
    palette = load_palette(
        PALETTE_PATH
    )

    OUTPUT_DIR.mkdir(
        parents=True,
        exist_ok=True
    )

    write_single_sprite(
        "flagpole",
        FLAGPOLE_ID,
        FLAGPOLE_MOVE,
        palette,
    )

    write_flag_atlas(
        "flag1_atlas",
        FLAG1_IDS,
        FLAG1_MOVE,
        palette,
    )

    print("BAKERY_FLAG2=EMPTY")
    print("BAKERY_FLAG3=EMPTY")
    print("SOURCE=ORIGINAL_HOUSES_RX")
    print("BAKERY_FLAGS=READY")


if __name__ == "__main__":
    main()

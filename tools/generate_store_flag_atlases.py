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
    "assets/buildings/store/runtime"
)

FLAG1_IDS = [
    1166, 1167, 1168, 1164, 1165
]

FLAG2_IDS = [
    1164, 1165, 1166, 1167, 1168
]


def load_frames(ids):
    return [
        load_rx_sprite(
            RX_PATH,
            sprite_id
        )
        for sprite_id in ids
    ]


def build_atlas(
    name,
    frames,
    move_x,
    move_y,
    palette,
):
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

    if (
        cell_width <= 0 or
        cell_height <= 0
    ):
        raise RuntimeError(
            f"Invalid {name} bounds"
        )

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

    for frame_index, frame in enumerate(frames):
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
                src_index = (
                    y *
                    frame["width"] +
                    x
                )

                palette_index = (
                    frame["pixels"][
                        src_index
                    ]
                )

                if palette_index == 0:
                    continue

                dst_x = origin_x + x
                dst_y = origin_y + y

                if (
                    dst_x < 0 or
                    dst_x >= atlas_width or
                    dst_y < 0 or
                    dst_y >= atlas_height
                ):
                    raise RuntimeError(
                        f"{name}: sprite outside cell"
                    )

                dst_index = (
                    dst_y *
                    atlas_width +
                    dst_x
                )

                atlas[dst_index] = (
                    palette[palette_index]
                )

    OUTPUT_DIR.mkdir(
        parents=True,
        exist_ok=True
    )

    atlas_path = (
        OUTPUT_DIR /
        f"{name}_atlas.bmp"
    )

    write_bmp(
        atlas_path,
        atlas_width,
        atlas_height,
        atlas
    )

    lines = [
        f"cell_width={cell_width}",
        f"cell_height={cell_height}",
        f"frame_count={len(frames)}",
        f"anchor_x={min_x}",
        f"anchor_y={min_y}",
        f"move_x={move_x}",
        f"move_y={move_y}",
    ]

    for index, frame in enumerate(frames):
        prefix = f"frame_{index:02d}"

        lines.extend([
            f"{prefix}_id={frame['id']}",
            f"{prefix}_width={frame['width']}",
            f"{prefix}_height={frame['height']}",
            f"{prefix}_pivot_x={frame['pivot_x']}",
            f"{prefix}_pivot_y={frame['pivot_y']}",
        ])

    metadata = (
        OUTPUT_DIR /
        f"{name}_atlas.txt"
    )

    metadata.write_text(
        "\n".join(lines) + "\n",
        encoding="utf-8"
    )

    print(f"ATLAS={atlas_path}")
    print(
        f"CELL={cell_width}x"
        f"{cell_height}"
    )
    print(
        f"ANCHOR={min_x},{min_y}"
    )
    print(
        f"MOVE={move_x},{move_y}"
    )
    print(
        "SEQUENCE=" +
        ",".join(
            str(f["id"])
            for f in frames
        )
    )


def main():
    palette = load_palette(
        PALETTE_PATH
    )

    print("=== STORE FLAG1 ===")

    build_atlas(
        "flag1",
        load_frames(FLAG1_IDS),
        15,
        -74,
        palette,
    )

    print()
    print("=== STORE FLAG2 ===")

    build_atlas(
        "flag2",
        load_frames(FLAG2_IDS),
        -54,
        -76,
        palette,
    )

    print()
    print("STORE_FLAGPOLE=NONE")
    print("SOURCE=ORIGINAL_HOUSES_RX")
    print("STORE_FLAGS=READY")


if __name__ == "__main__":
    main()

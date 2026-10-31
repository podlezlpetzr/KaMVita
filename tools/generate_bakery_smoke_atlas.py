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

# Bakery ha_Smoke:
# DAT 888..891
# RX  889..892
SPRITE_IDS = [
    889,
    890,
    891,
    892,
]


def main():
    palette = load_palette(
        PALETTE_PATH
    )

    frames = [
        load_rx_sprite(
            RX_PATH,
            sprite_id
        )
        for sprite_id in SPRITE_IDS
    ]

    min_x = min(
        frame["pivot_x"]
        for frame in frames
    )

    min_y = min(
        frame["pivot_y"]
        for frame in frames
    )

    max_x = max(
        frame["pivot_x"] +
        frame["width"]
        for frame in frames
    )

    max_y = max(
        frame["pivot_y"] +
        frame["height"]
        for frame in frames
    )

    cell_width = max_x - min_x
    cell_height = max_y - min_y

    if (
        cell_width <= 0 or
        cell_height <= 0
    ):
        raise SystemExit(
            "ERROR: invalid Bakery Smoke bounds"
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
        destination_origin_x = (
            frame_index *
            cell_width +
            frame["pivot_x"] -
            min_x
        )

        destination_origin_y = (
            frame["pivot_y"] -
            min_y
        )

        for y in range(frame["height"]):
            for x in range(frame["width"]):
                source_index = (
                    y *
                    frame["width"] +
                    x
                )

                palette_index = (
                    frame["pixels"][
                        source_index
                    ]
                )

                if palette_index == 0:
                    continue

                destination_x = (
                    destination_origin_x +
                    x
                )

                destination_y = (
                    destination_origin_y +
                    y
                )

                destination_index = (
                    destination_y *
                    atlas_width +
                    destination_x
                )

                atlas[destination_index] = (
                    palette[palette_index]
                )

    OUTPUT_DIR.mkdir(
        parents=True,
        exist_ok=True
    )

    atlas_path = (
        OUTPUT_DIR /
        "smoke_atlas.bmp"
    )

    write_bmp(
        atlas_path,
        atlas_width,
        atlas_height,
        atlas
    )

    metadata_lines = [
        f"cell_width={cell_width}",
        f"cell_height={cell_height}",
        "frame_count=4",
        f"anchor_x={min_x}",
        f"anchor_y={min_y}",
        "move_x=18",
        "move_y=5",
    ]

    for frame_index, frame in enumerate(frames):
        prefix = (
            f"frame_{frame_index}"
        )

        metadata_lines.extend([
            f"{prefix}_id={frame['id']}",
            f"{prefix}_width={frame['width']}",
            f"{prefix}_height={frame['height']}",
            f"{prefix}_pivot_x={frame['pivot_x']}",
            f"{prefix}_pivot_y={frame['pivot_y']}",
        ])

    metadata_path = (
        OUTPUT_DIR /
        "smoke_atlas.txt"
    )

    metadata_path.write_text(
        "\n".join(metadata_lines) + "\n",
        encoding="utf-8"
    )

    print(f"ATLAS={atlas_path}")
    print(
        f"CELL={cell_width}x{cell_height}"
    )
    print(f"ANCHOR={min_x},{min_y}")
    print("MOVE=18,5")
    print("FRAMES=4")
    print("RX_SEQUENCE=889,890,891,892")
    print(f"METADATA={metadata_path}")


if __name__ == "__main__":
    main()

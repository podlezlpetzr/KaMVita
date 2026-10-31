from pathlib import Path

from generate_construction_atlas import (
    TRANSPARENT_RGB,
    load_palette,
    load_rx_sprite,
    write_bmp,
)

ROOT = Path.home() / "KaMVita"

RX_PATH = ROOT / "resources/original_kam/houses.rx"
PALETTE_PATH = ROOT / "resources/original_kam/pal0.bbm"

OUTPUT_DIR = (
    ROOT /
    "assets/buildings/bakery/runtime"
)

# Bakery SupplyOut[1]:
# houses.dat DAT 104..108
# houses.rx  RX  105..109
SPRITE_IDS = [105, 106, 107, 108, 109]


def main():
    palette = load_palette(PALETTE_PATH)

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

    if cell_width <= 0 or cell_height <= 0:
        raise SystemExit(
            "ERROR: invalid Bread SupplyOut bounds"
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
        dst_origin_x = (
            frame_index *
            cell_width +
            frame["pivot_x"] -
            min_x
        )

        dst_origin_y = (
            frame["pivot_y"] -
            min_y
        )

        for y in range(frame["height"]):
            for x in range(frame["width"]):
                source_index = (
                    y * frame["width"] + x
                )

                palette_index = (
                    frame["pixels"][
                        source_index
                    ]
                )

                if palette_index == 0:
                    continue

                dst_x = (
                    dst_origin_x + x
                )

                dst_y = (
                    dst_origin_y + y
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
        "bread_supply_atlas.bmp"
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
        "frame_count=5",
        f"anchor_x={min_x}",
        f"anchor_y={min_y}",
    ]

    for index, frame in enumerate(frames):
        prefix = f"frame_{index}"

        lines.extend([
            f"{prefix}_count={index + 1}",
            f"{prefix}_id={frame['id']}",
            f"{prefix}_width={frame['width']}",
            f"{prefix}_height={frame['height']}",
            f"{prefix}_pivot_x={frame['pivot_x']}",
            f"{prefix}_pivot_y={frame['pivot_y']}",
        ])

    metadata_path = (
        OUTPUT_DIR /
        "bread_supply_atlas.txt"
    )

    metadata_path.write_text(
        "\n".join(lines) + "\n",
        encoding="utf-8"
    )

    print(f"ATLAS={atlas_path}")
    print(
        f"CELL={cell_width}x{cell_height}"
    )
    print(f"ANCHOR={min_x},{min_y}")
    print("FRAMES=5")
    print("RX_SEQUENCE=105,106,107,108,109")
    print(f"METADATA={metadata_path}")


if __name__ == "__main__":
    main()

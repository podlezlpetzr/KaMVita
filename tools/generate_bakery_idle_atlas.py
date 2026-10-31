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

OUTPUT_DIR = ROOT / "assets/buildings/bakery/runtime"

SPRITE_IDS = [
    1077,1077,1077,1077,1077,1077,
    1077,1077,1077,1077,1077,
    1078,1079,1080,1081,
    1082,1082,1082,1082,1082,
    1082,1082,1082,1082,
    1081,1080,1079,1078,1077,1077,
]

MOVE_X = 18
MOVE_Y = 5


def main():
    palette = load_palette(PALETTE_PATH)

    frames = [
        load_rx_sprite(RX_PATH, sprite_id)
        for sprite_id in SPRITE_IDS
    ]

    min_x = min(f["pivot_x"] for f in frames)
    min_y = min(f["pivot_y"] for f in frames)

    max_x = max(
        f["pivot_x"] + f["width"]
        for f in frames
    )

    max_y = max(
        f["pivot_y"] + f["height"]
        for f in frames
    )

    cell_width = max_x - min_x
    cell_height = max_y - min_y

    atlas_width = cell_width * len(frames)
    atlas_height = cell_height

    atlas = [
        TRANSPARENT_RGB
        for _ in range(atlas_width * atlas_height)
    ]

    for frame_index, frame in enumerate(frames):
        origin_x = (
            frame_index * cell_width +
            frame["pivot_x"] -
            min_x
        )

        origin_y = (
            frame["pivot_y"] -
            min_y
        )

        for y in range(frame["height"]):
            for x in range(frame["width"]):
                source_index = (
                    y * frame["width"] + x
                )

                palette_index = frame["pixels"][
                    source_index
                ]

                if palette_index == 0:
                    continue

                destination_x = origin_x + x
                destination_y = origin_y + y

                destination_index = (
                    destination_y * atlas_width +
                    destination_x
                )

                atlas[destination_index] = (
                    palette[palette_index]
                )

    OUTPUT_DIR.mkdir(
        parents=True,
        exist_ok=True
    )

    atlas_path = OUTPUT_DIR / "idle_atlas.bmp"

    write_bmp(
        atlas_path,
        atlas_width,
        atlas_height,
        atlas
    )

    metadata = [
        f"cell_width={cell_width}",
        f"cell_height={cell_height}",
        "frame_count=30",
        f"anchor_x={min_x}",
        f"anchor_y={min_y}",
        f"move_x={MOVE_X}",
        f"move_y={MOVE_Y}",
    ]

    for index, frame in enumerate(frames):
        prefix = f"frame_{index:02d}"

        metadata.extend([
            f"{prefix}_id={frame['id']}",
            f"{prefix}_width={frame['width']}",
            f"{prefix}_height={frame['height']}",
            f"{prefix}_pivot_x={frame['pivot_x']}",
            f"{prefix}_pivot_y={frame['pivot_y']}",
        ])

    metadata_path = OUTPUT_DIR / "idle_atlas.txt"

    metadata_path.write_text(
        "\n".join(metadata) + "\n",
        encoding="utf-8"
    )

    print(f"ATLAS={atlas_path}")
    print(f"CELL={cell_width}x{cell_height}")
    print(f"ANCHOR={min_x},{min_y}")
    print("MOVE=18,5")
    print("FRAMES=30")
    print(
        "RX_SEQUENCE=" +
        ",".join(map(str, SPRITE_IDS))
    )
    print("SOURCE=ORIGINAL_HOUSES_RX")


if __name__ == "__main__":
    main()

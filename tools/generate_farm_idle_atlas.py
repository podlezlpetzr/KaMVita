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
    "assets/buildings/farm/runtime"
)


# Original KaM houses.dat:
#
# FARM [house index 8]
# ha_Idle
# Count = 30
# Move  = (-16, -11)
#
# DAT:
# 1082 x14,
# 1083,1084,1085,1086,
# 1087 x6,
# 1086,1085,1084,1083,
# 1082 x2
#
# RX IDs are DAT + 1.
SPRITE_IDS = [
    1083, 1083, 1083, 1083, 1083,
    1083, 1083, 1083, 1083, 1083,
    1083, 1083, 1083, 1083,
    1084, 1085, 1086, 1087,
    1088, 1088, 1088,
    1088, 1088, 1088,
    1087, 1086, 1085, 1084,
    1083, 1083,
]

MOVE_X = -16
MOVE_Y = -11


def main():
    if not RX_PATH.is_file():
        raise SystemExit(
            f"ERROR: houses.rx missing: {RX_PATH}"
        )

    if not PALETTE_PATH.is_file():
        raise SystemExit(
            f"ERROR: palette missing: {PALETTE_PATH}"
        )

    if len(SPRITE_IDS) != 30:
        raise SystemExit(
            "ERROR: Farm ha_Idle must have 30 frames"
        )

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
            "ERROR: invalid Farm idle cell geometry"
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

        for y in range(
            frame["height"]
        ):
            for x in range(
                frame["width"]
            ):
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
                    origin_x + x
                )

                destination_y = (
                    origin_y + y
                )

                destination_index = (
                    destination_y *
                    atlas_width +
                    destination_x
                )

                atlas[
                    destination_index
                ] = palette[
                    palette_index
                ]

    OUTPUT_DIR.mkdir(
        parents=True,
        exist_ok=True
    )

    atlas_path = (
        OUTPUT_DIR /
        "idle_atlas.bmp"
    )

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
        "house_index=8",
        "house_action=ha_Idle",
    ]

    for index, frame in enumerate(
        frames
    ):
        prefix = f"frame_{index:02d}"

        metadata.extend([
            (
                f"{prefix}_id="
                f"{frame['id']}"
            ),
            (
                f"{prefix}_width="
                f"{frame['width']}"
            ),
            (
                f"{prefix}_height="
                f"{frame['height']}"
            ),
            (
                f"{prefix}_pivot_x="
                f"{frame['pivot_x']}"
            ),
            (
                f"{prefix}_pivot_y="
                f"{frame['pivot_y']}"
            ),
        ])

    metadata_path = (
        OUTPUT_DIR /
        "idle_atlas.txt"
    )

    metadata_path.write_text(
        "\n".join(metadata) + "\n",
        encoding="utf-8"
    )

    print(
        "=== FARM 70: ORIGINAL FARM IDLE ==="
    )
    print()
    print(f"ATLAS={atlas_path}")
    print(
        f"ATLAS_SIZE="
        f"{atlas_width}x{atlas_height}"
    )
    print(
        f"CELL="
        f"{cell_width}x{cell_height}"
    )
    print(
        f"ANCHOR={min_x},{min_y}"
    )
    print(
        f"MOVE={MOVE_X},{MOVE_Y}"
    )
    print("FRAMES=30")
    print("HOUSE_INDEX=8")
    print("ACTION=ha_Idle")
    print(
        "RX_SEQUENCE=" +
        ",".join(
            map(str, SPRITE_IDS)
        )
    )
    print("SOURCE=ORIGINAL_HOUSES_RX")

    print()
    print("=== FRAME GEOMETRY ===")

    for index, frame in enumerate(
        frames
    ):
        print(
            f"{index:02d}: "
            f"id={frame['id']} "
            f"size="
            f"{frame['width']}x"
            f"{frame['height']} "
            f"pivot=("
            f"{frame['pivot_x']},"
            f"{frame['pivot_y']})"
        )

    print()
    print("=== VERIFY ===")

    if not atlas_path.is_file():
        raise SystemExit(
            "ERROR: Farm idle atlas missing"
        )

    if atlas_path.stat().st_size <= 54:
        raise SystemExit(
            "ERROR: Farm idle atlas invalid"
        )

    if not metadata_path.is_file():
        raise SystemExit(
            "ERROR: Farm idle metadata missing"
        )

    print("OK: original Farm ha_Idle extracted")
    print("OK: 30-frame sequence preserved")
    print("OK: houses.dat Move (-16,-11) preserved")
    print("OK: RX sprite sequence 1083-1088 used")
    print("OK: per-sprite pivots preserved in atlas")
    print("OK: no PIL dependency")


if __name__ == "__main__":
    main()

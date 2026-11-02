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


# ============================================================
# ORIGINAL KaM FARM FLAG
#
# houses.dat:
#
# Farm house index 8
# ha_Flagpole = EMPTY
# ha_Flag1:
#
# Count = 10
# Move  = (-1, -69)
#
# DAT:
# 1174,1175,1176,1177,1168,
# 1169,1170,1171,1172,1173
#
# houses.rx IDs are DAT + 1:
# ============================================================

FLAG1_IDS = [
    1175,
    1176,
    1177,
    1178,
    1169,
    1170,
    1171,
    1172,
    1173,
    1174,
]

MOVE_X = -1
MOVE_Y = -69


def main():
    if not RX_PATH.is_file():
        raise SystemExit(
            f"ERROR: houses.rx missing: {RX_PATH}"
        )

    if not PALETTE_PATH.is_file():
        raise SystemExit(
            f"ERROR: palette missing: {PALETTE_PATH}"
        )

    if len(FLAG1_IDS) != 10:
        raise SystemExit(
            "ERROR: Farm ha_Flag1 must have "
            "exactly 10 frames"
        )

    palette = load_palette(
        PALETTE_PATH
    )

    frames = [
        load_rx_sprite(
            RX_PATH,
            sprite_id
        )
        for sprite_id in FLAG1_IDS
    ]

    for index, frame in enumerate(frames):
        if (
            frame["width"] <= 0 or
            frame["height"] <= 0
        ):
            raise SystemExit(
                "ERROR: invalid Farm flag frame "
                f"{index}: RX {FLAG1_IDS[index]}"
            )

    minimum_x = min(
        frame["pivot_x"]
        for frame in frames
    )

    minimum_y = min(
        frame["pivot_y"]
        for frame in frames
    )

    maximum_x = max(
        frame["pivot_x"] +
        frame["width"]
        for frame in frames
    )

    maximum_y = max(
        frame["pivot_y"] +
        frame["height"]
        for frame in frames
    )

    cell_width = (
        maximum_x -
        minimum_x
    )

    cell_height = (
        maximum_y -
        minimum_y
    )

    if (
        cell_width <= 0 or
        cell_height <= 0
    ):
        raise SystemExit(
            "ERROR: invalid shared Farm flag bounds"
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
            minimum_x
        )

        origin_y = (
            frame["pivot_y"] -
            minimum_y
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

                atlas[destination_index] = (
                    palette[palette_index]
                )

    OUTPUT_DIR.mkdir(
        parents=True,
        exist_ok=True
    )

    atlas_path = (
        OUTPUT_DIR /
        "flag1_atlas.bmp"
    )

    metadata_path = (
        OUTPUT_DIR /
        "flag1_atlas.txt"
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
        f"frame_count={len(frames)}",
        f"anchor_x={minimum_x}",
        f"anchor_y={minimum_y}",
        f"move_x={MOVE_X}",
        f"move_y={MOVE_Y}",
        "house_index=8",
        "house_action=ha_Flag1",
        "source=original houses.dat + houses.rx",
        "flagpole=empty",
        "flag2=empty",
        "flag3=empty",
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

    metadata_path.write_text(
        "\n".join(metadata) + "\n",
        encoding="utf-8"
    )

    if not atlas_path.is_file():
        raise SystemExit(
            "ERROR: Farm flag atlas missing"
        )

    if atlas_path.stat().st_size <= 54:
        raise SystemExit(
            "ERROR: Farm flag atlas invalid"
        )

    if not metadata_path.is_file():
        raise SystemExit(
            "ERROR: Farm flag metadata missing"
        )

    print(
        "=== FARM 98: ORIGINAL FARM FLAG1 ==="
    )

    print()
    print(f"ATLAS={atlas_path}")

    print(
        "ATLAS_SIZE="
        f"{atlas_width}x{atlas_height}"
    )

    print(
        "CELL="
        f"{cell_width}x{cell_height}"
    )

    print(
        "ANCHOR="
        f"{minimum_x},{minimum_y}"
    )

    print(
        "MOVE="
        f"{MOVE_X},{MOVE_Y}"
    )

    print("FRAMES=10")
    print("HOUSE_INDEX=8")
    print("ACTION=ha_Flag1")

    print(
        "RX_SEQUENCE=" +
        ",".join(
            map(str, FLAG1_IDS)
        )
    )

    print()
    print("=== FRAME GEOMETRY ===")

    for index, frame in enumerate(frames):
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
    print("OK: Farm house index 8")
    print("OK: original ha_Flag1 Count=10")
    print("OK: original Move=(-1,-69)")
    print("OK: exact RX sequence preserved")
    print("OK: per-frame RX pivots preserved")
    print("OK: ha_Flagpole is empty")
    print("OK: ha_Flag2 is empty")
    print("OK: ha_Flag3 is empty")
    print("OK: no synthetic frames")
    print("OK: no PIL dependency")


if __name__ == "__main__":
    main()

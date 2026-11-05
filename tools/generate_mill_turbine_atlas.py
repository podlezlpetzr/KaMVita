from pathlib import Path
import sys

ROOT = Path.home() / "KaMVita"

sys.path.insert(
    0,
    str(ROOT / "tools")
)

from generate_construction_atlas import (
    TRANSPARENT_RGB,
    load_palette,
    load_rx_sprite,
    write_bmp,
)

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
    "assets/buildings/mill/runtime"
)

# Original houses.dat, real Mill record 22:
#
# ha_Flagpole
# Count = 10
# Move  = (-17, -80)
#
# DAT IDs: 347..356
# RX IDs : 348..357
SPRITE_IDS = [
    348,
    349,
    350,
    351,
    352,
    353,
    354,
    355,
    356,
    357,
]

MOVE_X = -17
MOVE_Y = -80

HOUSE_INDEX = 22
HOUSE_ACTION = "ha_Flagpole"


def main():
    if not RX_PATH.is_file():
        raise SystemExit(
            f"ERROR: houses.rx missing: {RX_PATH}"
        )

    if not PALETTE_PATH.is_file():
        raise SystemExit(
            f"ERROR: palette missing: {PALETTE_PATH}"
        )

    if len(SPRITE_IDS) != 10:
        raise SystemExit(
            "ERROR: Mill turbine must have 10 frames"
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

    if cell_width <= 0 or cell_height <= 0:
        raise SystemExit(
            "ERROR: invalid Mill turbine geometry"
        )

    columns = 5
    rows = 2

    atlas_width = cell_width * columns
    atlas_height = cell_height * rows

    atlas = [
        TRANSPARENT_RGB
        for _ in range(
            atlas_width * atlas_height
        )
    ]

    for frame_index, frame in enumerate(frames):
        column = frame_index % columns
        row = frame_index // columns

        cell_x = column * cell_width
        cell_y = row * cell_height

        origin_x = (
            cell_x +
            frame["pivot_x"] -
            min_x
        )

        origin_y = (
            cell_y +
            frame["pivot_y"] -
            min_y
        )

        for source_y in range(
            frame["height"]
        ):
            for source_x in range(
                frame["width"]
            ):
                source_index = (
                    source_y *
                    frame["width"] +
                    source_x
                )

                palette_index = (
                    frame["pixels"][
                        source_index
                    ]
                )

                if palette_index == 0:
                    continue

                destination_x = (
                    origin_x + source_x
                )

                destination_y = (
                    origin_y + source_y
                )

                if (
                    destination_x < cell_x or
                    destination_x >=
                        cell_x + cell_width or
                    destination_y < cell_y or
                    destination_y >=
                        cell_y + cell_height
                ):
                    raise RuntimeError(
                        "ERROR: turbine sprite "
                        "exceeds atlas cell"
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
        "turbine_atlas.bmp"
    )

    write_bmp(
        atlas_path,
        atlas_width,
        atlas_height,
        atlas
    )

    metadata = [
        f"house_index={HOUSE_INDEX}",
        f"house_action={HOUSE_ACTION}",
        f"cell_width={cell_width}",
        f"cell_height={cell_height}",
        f"frame_count={len(frames)}",
        f"columns={columns}",
        f"rows={rows}",
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

    metadata_path = (
        OUTPUT_DIR /
        "turbine_atlas.txt"
    )

    metadata_path.write_text(
        "\n".join(metadata) + "\n",
        encoding="utf-8"
    )

    print("=== ORIGINAL MILL TURBINE ===")
    print(f"ATLAS={atlas_path}")
    print(
        f"ATLAS_SIZE="
        f"{atlas_width}x{atlas_height}"
    )
    print(
        f"CELL={cell_width}x{cell_height}"
    )
    print(f"GRID={columns}x{rows}")
    print(f"FRAMES={len(frames)}")
    print(f"ANCHOR={min_x},{min_y}")
    print(f"MOVE={MOVE_X},{MOVE_Y}")
    print(
        "RX_SEQUENCE=" +
        ",".join(
            map(str, SPRITE_IDS)
        )
    )
    print(f"METADATA={metadata_path}")
    print()

    print("=== FRAME GEOMETRY ===")

    for index, frame in enumerate(frames):
        print(
            f"{index:02d}: "
            f"RX={frame['id']} "
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
            "ERROR: turbine atlas missing"
        )

    if atlas_path.stat().st_size <= 54:
        raise SystemExit(
            "ERROR: turbine atlas invalid"
        )

    if not metadata_path.is_file():
        raise SystemExit(
            "ERROR: turbine metadata missing"
        )

    print("OK: original Mill ha_Flagpole extracted")
    print("OK: RX 348..357 preserved")
    print("OK: 10-frame sequence preserved")
    print("OK: Move (-17,-80) preserved")
    print("OK: per-frame pivots preserved")
    print("MILL_TURBINE_ATLAS_READY")


if __name__ == "__main__":
    main()

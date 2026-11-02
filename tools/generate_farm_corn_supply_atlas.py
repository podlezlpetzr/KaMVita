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
# ORIGINAL KaM FARM SUPPLYOUT
#
# houses.dat:
#
# Farm house index 8
#
# SupplyOut[1]:
# DAT 386,387,388,389,390
#
# houses.rx:
# RX 387,388,389,390,391
#
# Counts:
# 1,2,3,4,5 Corn
# ============================================================

SPRITE_IDS = [
    387,
    388,
    389,
    390,
    391,
]

EXPECTED_GEOMETRY = [
    (18, 24, 47, -3),
    (21, 23, 44, -2),
    (21, 27, 44, -1),
    (45, 26, 20,  0),
    (45, 31, 20,  0),
]

EXPECTED_CELL_WIDTH = 45
EXPECTED_CELL_HEIGHT = 34

EXPECTED_ANCHOR_X = 20
EXPECTED_ANCHOR_Y = -3


def main():
    if not RX_PATH.is_file():
        raise SystemExit(
            f"ERROR: missing {RX_PATH}"
        )

    if not PALETTE_PATH.is_file():
        raise SystemExit(
            f"ERROR: missing {PALETTE_PATH}"
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

    if len(frames) != 5:
        raise SystemExit(
            "ERROR: expected exactly 5 "
            "Farm SupplyOut frames"
        )

    print(
        "=== FARM 87: ORIGINAL FARM "
        "CORN SUPPLYOUT ATLAS ==="
    )
    print()

    print(
        "RX_SEQUENCE=" +
        ",".join(
            map(str, SPRITE_IDS)
        )
    )

    print("COUNTS=1,2,3,4,5")
    print("RESOURCE=wt_Corn")
    print()

    # ========================================================
    # VERIFY SOURCE GEOMETRY
    # ========================================================

    for index, frame in enumerate(frames):
        expected = EXPECTED_GEOMETRY[
            index
        ]

        actual = (
            frame["width"],
            frame["height"],
            frame["pivot_x"],
            frame["pivot_y"],
        )

        if actual != expected:
            raise SystemExit(
                "ERROR: RX geometry mismatch "
                f"for sprite "
                f"{SPRITE_IDS[index]}: "
                f"expected={expected} "
                f"actual={actual}"
            )

        print(
            f"frame={index} "
            f"count={index + 1} "
            f"id={frame['id']} "
            f"size="
            f"{frame['width']}x"
            f"{frame['height']} "
            f"pivot=("
            f"{frame['pivot_x']},"
            f"{frame['pivot_y']})"
        )

    # ========================================================
    # CALCULATE SHARED BOUNDING BOX
    # ========================================================

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

    cell_width = (
        max_x -
        min_x
    )

    cell_height = (
        max_y -
        min_y
    )

    if (
        cell_width !=
            EXPECTED_CELL_WIDTH or
        cell_height !=
            EXPECTED_CELL_HEIGHT
    ):
        raise SystemExit(
            "ERROR: shared Farm SupplyOut "
            "geometry mismatch: "
            f"{cell_width}x{cell_height}"
        )

    if (
        min_x != EXPECTED_ANCHOR_X or
        min_y != EXPECTED_ANCHOR_Y
    ):
        raise SystemExit(
            "ERROR: shared Farm SupplyOut "
            "anchor mismatch: "
            f"({min_x},{min_y})"
        )

    print()
    print(
        f"CELL="
        f"{cell_width}x{cell_height}"
    )

    print(
        f"ANCHOR="
        f"{min_x},{min_y}"
    )

    # ========================================================
    # COMPOSE ATLAS
    #
    # 5 horizontal cells:
    #
    # [1 Corn][2 Corn][3 Corn][4 Corn][5 Corn]
    #
    # Every original sprite is positioned according to its
    # own RX pivot inside the shared 45x34 coordinate system.
    # ========================================================

    atlas_width = (
        cell_width *
        len(frames)
    )

    atlas_height = (
        cell_height
    )

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

                # Original transparent palette entry.
                if palette_index == 0:
                    continue

                destination_x = (
                    origin_x + x
                )

                destination_y = (
                    origin_y + y
                )

                if (
                    destination_x <
                        frame_index *
                        cell_width or
                    destination_x >=
                        (
                            frame_index + 1
                        ) *
                        cell_width or
                    destination_y < 0 or
                    destination_y >=
                        cell_height
                ):
                    raise SystemExit(
                        "ERROR: sprite exceeds "
                        "atlas cell bounds"
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
        "corn_supply_atlas.bmp"
    )

    metadata_path = (
        OUTPUT_DIR /
        "corn_supply_atlas.txt"
    )

    write_bmp(
        atlas_path,
        atlas_width,
        atlas_height,
        atlas
    )

    # ========================================================
    # METADATA
    # ========================================================

    metadata = [
        f"cell_width={cell_width}",
        f"cell_height={cell_height}",
        "frame_count=5",
        f"anchor_x={min_x}",
        f"anchor_y={min_y}",
        "house_index=8",
        "supply_slot=1",
        "ware=wt_Corn",
        "kamvita_resource=resourceOut[0]",
    ]

    for index, frame in enumerate(
        frames
    ):
        prefix = (
            f"frame_{index}"
        )

        metadata.extend([
            (
                f"{prefix}_count="
                f"{index + 1}"
            ),
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

    metadata_path.write_text(
        "\n".join(metadata) + "\n",
        encoding="utf-8"
    )

    # ========================================================
    # VERIFY OUTPUT
    # ========================================================

    if not atlas_path.is_file():
        raise SystemExit(
            "ERROR: atlas file missing"
        )

    if atlas_path.stat().st_size <= 54:
        raise SystemExit(
            "ERROR: atlas BMP invalid"
        )

    if not metadata_path.is_file():
        raise SystemExit(
            "ERROR: metadata missing"
        )

    print()
    print("=== OUTPUT ===")
    print(
        f"ATLAS={atlas_path}"
    )
    print(
        f"ATLAS_SIZE="
        f"{atlas_width}x"
        f"{atlas_height}"
    )
    print(
        f"METADATA={metadata_path}"
    )

    print()
    print("=== VERIFY ===")

    print(
        "OK: Farm SupplyOut[1] "
        "uses RX 387..391"
    )

    print(
        "OK: 5 Corn quantity frames "
        "preserved"
    )

    print(
        "OK: shared cell is 45x34"
    )

    print(
        "OK: shared anchor is 20,-3"
    )

    print(
        "OK: per-frame original "
        "RX pivots preserved"
    )

    print(
        "OK: transparent magenta "
        "colorkey preserved"
    )

    print(
        "OK: no PIL dependency"
    )


if __name__ == "__main__":
    main()

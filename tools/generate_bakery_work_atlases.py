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


WORK2_IDS = [
    694, 695, 696, 697, 698, 697,
    696, 695, 694, 695, 696, 697,
    698, 697, 696, 695, 694, 695,
    696, 697, 698, 697, 696, 695,
]

WORK3_IDS = [
    699, 700, 701, 702, 701, 700,
    699, 913, 704, 705, 706, 707,
    708, 707, 706, 705, 704, 703,
    704, 705, 700, 701, 702, 701,
    700, 913, 698, 697, 696, 695,
]

MOVE_X = 18
MOVE_Y = 5


def load_frames(sprite_ids):
    return [
        load_rx_sprite(
            RX_PATH,
            sprite_id
        )
        for sprite_id in sprite_ids
    ]


def get_common_bounds(frame_groups):
    frames = [
        frame
        for group in frame_groups
        for frame in group
    ]

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

    return (
        minimum_x,
        minimum_y,
        maximum_x,
        maximum_y,
    )


def build_atlas(
    name,
    action_name,
    frames,
    palette,
    minimum_x,
    minimum_y,
    maximum_x,
    maximum_y,
):
    cell_width = maximum_x - minimum_x
    cell_height = maximum_y - minimum_y

    if (
        cell_width <= 0 or
        cell_height <= 0
    ):
        raise RuntimeError(
            "Neplatny pivotovy prostor atlasu"
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
            minimum_x
        )

        destination_origin_y = (
            frame["pivot_y"] -
            minimum_y
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
                    destination_origin_x +
                    source_x
                )

                destination_y = (
                    destination_origin_y +
                    source_y
                )

                if (
                    destination_x < 0 or
                    destination_x >= atlas_width or
                    destination_y < 0 or
                    destination_y >= atlas_height
                ):
                    raise RuntimeError(
                        "Sprite presahuje atlasovou bunku"
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
        exist_ok=True,
    )

    atlas_path = (
        OUTPUT_DIR /
        f"{name}_atlas.bmp"
    )

    write_bmp(
        atlas_path,
        atlas_width,
        atlas_height,
        atlas,
    )

    metadata = [
        f"action={action_name}",
        f"cell_width={cell_width}",
        f"cell_height={cell_height}",
        f"frame_count={len(frames)}",
        f"anchor_x={minimum_x}",
        f"anchor_y={minimum_y}",
        f"move_x={MOVE_X}",
        f"move_y={MOVE_Y}",
    ]

    for frame_index, frame in enumerate(frames):
        prefix = f"frame_{frame_index:02d}"

        metadata.extend([
            f"{prefix}_id={frame['id']}",
            f"{prefix}_width={frame['width']}",
            f"{prefix}_height={frame['height']}",
            f"{prefix}_pivot_x={frame['pivot_x']}",
            f"{prefix}_pivot_y={frame['pivot_y']}",
        ])

    metadata_path = (
        OUTPUT_DIR /
        f"{name}_atlas.txt"
    )

    metadata_path.write_text(
        "\n".join(metadata) + "\n",
        encoding="utf-8",
    )

    print(f"ATLAS={atlas_path}")
    print(
        f"SIZE={atlas_width}x"
        f"{atlas_height}"
    )
    print(
        f"CELL={cell_width}x"
        f"{cell_height}"
    )
    print(f"FRAMES={len(frames)}")
    print(
        f"ANCHOR={minimum_x},"
        f"{minimum_y}"
    )
    print(f"MOVE={MOVE_X},{MOVE_Y}")
    print(f"METADATA={metadata_path}")


def main():
    if not RX_PATH.is_file():
        raise SystemExit(
            f"Chybi {RX_PATH}"
        )

    if not PALETTE_PATH.is_file():
        raise SystemExit(
            f"Chybi {PALETTE_PATH}"
        )

    palette = load_palette(
        PALETTE_PATH
    )

    work2_frames = load_frames(
        WORK2_IDS
    )

    work3_frames = load_frames(
        WORK3_IDS
    )

    (
        minimum_x,
        minimum_y,
        maximum_x,
        maximum_y,
    ) = get_common_bounds([
        work2_frames,
        work3_frames,
    ])

    print("=== COMMON PIVOT SPACE ===")
    print(f"MIN={minimum_x},{minimum_y}")
    print(f"MAX={maximum_x},{maximum_y}")
    print()

    print("=== WORK2 ===")
    build_atlas(
        "work2",
        "ha_Work2",
        work2_frames,
        palette,
        minimum_x,
        minimum_y,
        maximum_x,
        maximum_y,
    )

    print()
    print("=== WORK3 ===")
    build_atlas(
        "work3",
        "ha_Work3",
        work3_frames,
        palette,
        minimum_x,
        minimum_y,
        maximum_x,
        maximum_y,
    )

    print()
    print("ORIGINAL_HOUSES_RX=YES")
    print("COMMON_PIVOT_SPACE=YES")
    print("TEMPORARY_ASSETS=NONE")
    print("BAKERY_WORK_ATLASES=READY")


if __name__ == "__main__":
    main()

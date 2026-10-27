import sys
from pathlib import Path

from generate_construction_atlas import (
    TRANSPARENT_RGB,
    load_palette,
    load_rx_sprite,
    write_bmp,
)


SPRITE_IDS = [
    814, 815, 816, 817, 818,
    861, 862, 863, 864, 865,
    866, 867, 868, 869, 870,
    871, 872, 873, 874, 875,
    876, 877, 878, 879, 880,
]

CELL_WIDTH = 16
CELL_HEIGHT = 20
MOVE_X = -17
MOVE_Y = 5


def generate_school_work_atlas(
    rx_path,
    palette_path,
    output_path,
):
    palette = load_palette(palette_path)

    sprites = [
        load_rx_sprite(rx_path, sprite_id)
        for sprite_id in SPRITE_IDS
    ]

    for sprite in sprites:
        if (
            sprite["width"] > CELL_WIDTH or
            sprite["height"] > CELL_HEIGHT
        ):
            raise ValueError(
                f"Sprite {sprite['id']} ma rozmer "
                f"{sprite['width']}x{sprite['height']}, "
                f"ale bunka je "
                f"{CELL_WIDTH}x{CELL_HEIGHT}"
            )

    atlas_width = CELL_WIDTH * len(sprites)
    atlas_height = CELL_HEIGHT

    atlas = [
        TRANSPARENT_RGB
        for _ in range(atlas_width * atlas_height)
    ]

    for frame_index, sprite in enumerate(sprites):
        destination_x = frame_index * CELL_WIDTH

        for y in range(sprite["height"]):
            for x in range(sprite["width"]):
                source_index = (
                    y * sprite["width"] + x
                )

                palette_index = sprite["pixels"][
                    source_index
                ]

                if palette_index == 0:
                    continue

                atlas_index = (
                    y * atlas_width +
                    destination_x +
                    x
                )

                atlas[atlas_index] = palette[
                    palette_index
                ]

    output_path = Path(output_path)
    output_path.parent.mkdir(
        parents=True,
        exist_ok=True,
    )

    write_bmp(
        output_path,
        atlas_width,
        atlas_height,
        atlas,
    )

    metadata_lines = [
        f"cell_width={CELL_WIDTH}",
        f"cell_height={CELL_HEIGHT}",
        f"frame_count={len(sprites)}",
        f"move_x={MOVE_X}",
        f"move_y={MOVE_Y}",
    ]

    for frame_index, sprite in enumerate(sprites):
        prefix = f"frame_{frame_index:02d}"

        metadata_lines.extend([
            f"{prefix}_id={sprite['id']}",
            f"{prefix}_width={sprite['width']}",
            f"{prefix}_height={sprite['height']}",
            f"{prefix}_pivot_x={sprite['pivot_x']}",
            f"{prefix}_pivot_y={sprite['pivot_y']}",
        ])

    metadata_path = output_path.with_suffix(".txt")

    metadata_path.write_text(
        "\n".join(metadata_lines) + "\n",
        encoding="utf-8",
    )

    print(f"Atlas: {output_path}")
    print(
        f"Rozmer: {atlas_width}x{atlas_height}"
    )
    print(f"Framu: {len(sprites)}")
    print(f"Metadata: {metadata_path}")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        raise SystemExit(
            "Pouziti:\n"
            "generate_school_work_atlas.py "
            "houses.rx pal0.bbm vystup.bmp"
        )

    generate_school_work_atlas(
        sys.argv[1],
        sys.argv[2],
        sys.argv[3],
    )

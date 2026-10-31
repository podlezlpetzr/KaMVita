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
    "resources/original_kam/units.rx"
)

PALETTE_PATH = (
    ROOT /
    "resources/original_kam/pal0.bbm"
)

OUTPUT_DIR = (
    ROOT /
    "assets/units/thoughts"
)

# KM_Defaults.pas ThoughtBounds.
#
# Bounds in Remake are DAT-style values.
# AddUnitThought selects Id from bounds and then works
# with units.rx sprite IDs in the same renderer convention.
THOUGHTS = {
    "eat": list(range(6251, 6259)),
    "death": list(range(6291, 6299)),
    "quest": list(range(6299, 6307)),
}


def build_atlas(
    name,
    sprite_ids,
    palette,
):
    frames = [
        load_rx_sprite(
            RX_PATH,
            sprite_id
        )
        for sprite_id in sprite_ids
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

    frame_count = len(frames)

    atlas_width = (
        cell_width *
        frame_count
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

    bmp_path = (
        OUTPUT_DIR /
        f"{name}_atlas.bmp"
    )

    txt_path = (
        OUTPUT_DIR /
        f"{name}_atlas.txt"
    )

    write_bmp(
        bmp_path,
        atlas_width,
        atlas_height,
        atlas
    )

    lines = [
        f"thought={name}",
        f"cell_width={cell_width}",
        f"cell_height={cell_height}",
        f"frame_count={frame_count}",
        f"anchor_x={min_x}",
        f"anchor_y={min_y}",
        "animation=reverse",
        "source=original units.rx",
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

    txt_path.write_text(
        "\n".join(lines) + "\n",
        encoding="utf-8"
    )

    print(
        f"{name.upper()}_ATLAS="
        f"{bmp_path}"
    )

    print(
        f"{name.upper()}_CELL="
        f"{cell_width}x{cell_height}"
    )

    print(
        f"{name.upper()}_ANCHOR="
        f"{min_x},{min_y}"
    )

    print(
        f"{name.upper()}_RX="
        + ",".join(
            str(frame["id"])
            for frame in frames
        )
    )


def main():
    palette = load_palette(
        PALETTE_PATH
    )

    print(
        "=== ORIGINAL KaM UNIT THOUGHT ATLASES ==="
    )

    for name, sprite_ids in THOUGHTS.items():
        print()

        build_atlas(
            name,
            sprite_ids,
            palette
        )

    manifest = (
        OUTPUT_DIR /
        "manifest.txt"
    )

    manifest.write_text(
        "\n".join([
            "source=original units.rx",
            "thoughts=eat,death,quest",
            "eat_bounds=6250,6257",
            "death_bounds=6290,6297",
            "quest_bounds=6298,6305",
            "frames_per_thought=8",
            "animation=reverse",
            "thought_x_offset=0.02",
            "custom_assets=none",
            "temporary_assets=none",
        ]) + "\n",
        encoding="utf-8"
    )

    print()
    print(f"MANIFEST={manifest}")
    print("THOUGHT_ATLASES=READY")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3

from pathlib import Path
import struct

from generate_construction_atlas import (
    TRANSPARENT_RGB,
    load_palette,
    load_rx_sprite,
    write_bmp,
)

ROOT = Path.home() / "KaMVita"

UNIT_DAT = (
    ROOT /
    "resources/original_kam/unit.dat"
)

RX_CANDIDATES = [
    ROOT / "resources/original_kam/units.rx",
    ROOT / "resources/original_kam/unit.rx",
]

UNITS_RX = next(
    (
        path
        for path in RX_CANDIDATES
        if path.is_file()
    ),
    None,
)

if UNITS_RX is None:
    raise SystemExit(
        "ERROR: units.rx ani unit.rx nenalezen"
    )

PALETTE_PATH = (
    ROOT /
    "resources/original_kam/pal0.bbm"
)

OUTPUT_ROOT = (
    ROOT /
    "assets/units/baker/complete"
)

UNIT_COUNT = 41
WARE_COUNT = 28

UNIT_DAT_SIZE = 22
ANIM_LOOP_SIZE = 70

ACTION_COUNT = 14
DIRECTION_COUNT = 8

UNIT_SPRITE_SIZE = (
    ACTION_COUNT *
    DIRECTION_COUNT *
    ANIM_LOOP_SIZE
)

UNIT_SPRITE2_SIZE = 36

SERF_CARRY_SIZE = (
    WARE_COUNT *
    DIRECTION_COUNT *
    ANIM_LOOP_SIZE
)

RECORD_SIZE = (
    UNIT_DAT_SIZE +
    UNIT_SPRITE_SIZE +
    UNIT_SPRITE2_SIZE
)

EXPECTED_SIZE = (
    SERF_CARRY_SIZE +
    UNIT_COUNT * RECORD_SIZE
)

BAKER_INDEX = 6

BAKER_RECORD_OFFSET = (
    SERF_CARRY_SIZE +
    BAKER_INDEX * RECORD_SIZE
)

BAKER_SPRITE_OFFSET = (
    BAKER_RECORD_OFFSET +
    UNIT_DAT_SIZE
)

DIRECTIONS = [
    "n",
    "ne",
    "e",
    "se",
    "s",
    "sw",
    "w",
    "nw",
]

# unit.dat action indices.
ACTIONS = {
    "walk": 0,
    "work": 1,
    "die": 3,
    "eat": 7,
}

# Remake UnitStillFrames for 8 actual directions.
#
# Source array also includes direction None at index 0.
# N,NE,E,SE,S,SW,W,NW:
STILL_FRAMES = [
    3,
    2,
    2,
    1,
    6,
    7,
    6,
    6,
]


def read_anim_loop(
    data,
    action_index,
    direction_index,
):
    loop_index = (
        action_index *
        DIRECTION_COUNT +
        direction_index
    )

    offset = (
        BAKER_SPRITE_OFFSET +
        loop_index * ANIM_LOOP_SIZE
    )

    steps = list(
        struct.unpack_from(
            "<30h",
            data,
            offset
        )
    )

    count = struct.unpack_from(
        "<h",
        data,
        offset + 60
    )[0]

    move_x = struct.unpack_from(
        "<i",
        data,
        offset + 62
    )[0]

    move_y = struct.unpack_from(
        "<i",
        data,
        offset + 66
    )[0]

    if count < 0 or count > 30:
        raise RuntimeError(
            f"Invalid frame count {count} "
            f"for action {action_index}, "
            f"direction {direction_index}"
        )

    dat_ids = [
        value
        for value in steps[:count]
        if value >= 0
    ]

    rx_ids = [
        value + 1
        for value in dat_ids
    ]

    return {
        "count": count,
        "move_x": move_x,
        "move_y": move_y,
        "dat_ids": dat_ids,
        "rx_ids": rx_ids,
    }


def gather_action(
    data,
    action_name,
    action_index,
):
    directions = []

    for direction_index, direction_name in enumerate(
        DIRECTIONS
    ):
        animation = read_anim_loop(
            data,
            action_index,
            direction_index,
        )

        # count=1 with DAT=-1 means an empty placeholder,
        # not a drawable animation.
        if not animation["rx_ids"]:
            raise RuntimeError(
                f"{action_name}/{direction_name}: "
                "empty original RX sequence"
            )

        frames = [
            load_rx_sprite(
                UNITS_RX,
                sprite_id
            )
            for sprite_id
            in animation["rx_ids"]
        ]

        directions.append({
            "name": direction_name,
            "animation": animation,
            "frames": frames,
        })

    return directions


def compute_bounds(directions):
    all_frames = [
        frame
        for direction in directions
        for frame in direction["frames"]
    ]

    min_x = min(
        frame["pivot_x"]
        for frame in all_frames
    )

    min_y = min(
        frame["pivot_y"]
        for frame in all_frames
    )

    max_x = max(
        frame["pivot_x"] +
        frame["width"]
        for frame in all_frames
    )

    max_y = max(
        frame["pivot_y"] +
        frame["height"]
        for frame in all_frames
    )

    return {
        "anchor_x": min_x,
        "anchor_y": min_y,
        "cell_width": max_x - min_x,
        "cell_height": max_y - min_y,
    }


def build_action_atlas(
    action_name,
    action_index,
    directions,
    palette,
):
    bounds = compute_bounds(
        directions
    )

    cell_width = bounds["cell_width"]
    cell_height = bounds["cell_height"]

    max_frames = max(
        len(direction["frames"])
        for direction in directions
    )

    atlas_width = (
        max_frames *
        cell_width
    )

    atlas_height = (
        DIRECTION_COUNT *
        cell_height
    )

    atlas = [
        TRANSPARENT_RGB
        for _ in range(
            atlas_width *
            atlas_height
        )
    ]

    for direction_index, direction in enumerate(
        directions
    ):
        for frame_index, frame in enumerate(
            direction["frames"]
        ):
            origin_x = (
                frame_index *
                cell_width +
                frame["pivot_x"] -
                bounds["anchor_x"]
            )

            origin_y = (
                direction_index *
                cell_height +
                frame["pivot_y"] -
                bounds["anchor_y"]
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

                    if (
                        destination_x < 0 or
                        destination_x >= atlas_width or
                        destination_y < 0 or
                        destination_y >= atlas_height
                    ):
                        raise RuntimeError(
                            f"{action_name}: frame "
                            "outside atlas bounds"
                        )

                    destination_index = (
                        destination_y *
                        atlas_width +
                        destination_x
                    )

                    atlas[destination_index] = (
                        palette[palette_index]
                    )

    action_dir = (
        OUTPUT_ROOT /
        "actions"
    )

    action_dir.mkdir(
        parents=True,
        exist_ok=True
    )

    atlas_path = (
        action_dir /
        f"{action_name}_atlas.bmp"
    )

    write_bmp(
        atlas_path,
        atlas_width,
        atlas_height,
        atlas
    )

    metadata_lines = [
        f"action={action_name}",
        f"action_index={action_index}",
        f"cell_width={cell_width}",
        f"cell_height={cell_height}",
        f"direction_count={DIRECTION_COUNT}",
        f"max_frames={max_frames}",
        f"anchor_x={bounds['anchor_x']}",
        f"anchor_y={bounds['anchor_y']}",
        "source=original unit.dat + units.rx",
        "unit_index=6",
    ]

    for direction_index, direction in enumerate(
        directions
    ):
        prefix = (
            f"direction_{direction_index:02d}_"
            f"{direction['name']}"
        )

        animation = direction["animation"]

        metadata_lines.extend([
            (
                f"{prefix}_frame_count="
                f"{len(direction['frames'])}"
            ),
            (
                f"{prefix}_move_x="
                f"{animation['move_x']}"
            ),
            (
                f"{prefix}_move_y="
                f"{animation['move_y']}"
            ),
            (
                f"{prefix}_rx="
                + ",".join(
                    map(
                        str,
                        animation["rx_ids"]
                    )
                )
            ),
        ])

        for frame_index, frame in enumerate(
            direction["frames"]
        ):
            frame_prefix = (
                f"{prefix}_frame_"
                f"{frame_index:02d}"
            )

            metadata_lines.extend([
                f"{frame_prefix}_id={frame['id']}",
                (
                    f"{frame_prefix}_width="
                    f"{frame['width']}"
                ),
                (
                    f"{frame_prefix}_height="
                    f"{frame['height']}"
                ),
                (
                    f"{frame_prefix}_pivot_x="
                    f"{frame['pivot_x']}"
                ),
                (
                    f"{frame_prefix}_pivot_y="
                    f"{frame['pivot_y']}"
                ),
            ])

    metadata_path = (
        action_dir /
        f"{action_name}_atlas.txt"
    )

    metadata_path.write_text(
        "\n".join(metadata_lines) + "\n",
        encoding="utf-8"
    )

    print(
        f"{action_name.upper()}_ATLAS="
        f"{atlas_path}"
    )

    print(
        f"{action_name.upper()}_SIZE="
        f"{atlas_width}x{atlas_height}"
    )

    print(
        f"{action_name.upper()}_CELL="
        f"{cell_width}x{cell_height}"
    )

    print(
        f"{action_name.upper()}_ANCHOR="
        f"{bounds['anchor_x']},"
        f"{bounds['anchor_y']}"
    )

    print(
        f"{action_name.upper()}_MAX_FRAMES="
        f"{max_frames}"
    )

    return {
        "atlas": atlas_path,
        "metadata": metadata_path,
        "cell_width": cell_width,
        "cell_height": cell_height,
        "anchor_x": bounds["anchor_x"],
        "anchor_y": bounds["anchor_y"],
        "max_frames": max_frames,
        "directions": directions,
    }


def write_idle_metadata(walk_result):
    path = (
        OUTPUT_ROOT /
        "idle_from_walk.txt"
    )

    lines = [
        "source_action=walk",
        "source=Remake UnitStillFrames",
        "direction_count=8",
    ]

    for direction_index, direction_name in enumerate(
        DIRECTIONS
    ):
        frame_index = STILL_FRAMES[
            direction_index
        ]

        frames = (
            walk_result[
                "directions"
            ][direction_index]["frames"]
        )

        if (
            frame_index < 0 or
            frame_index >= len(frames)
        ):
            raise RuntimeError(
                f"Idle frame {frame_index} "
                f"outside {direction_name}"
            )

        lines.extend([
            (
                f"direction_{direction_index:02d}_"
                f"{direction_name}_frame="
                f"{frame_index}"
            ),
            (
                f"direction_{direction_index:02d}_"
                f"{direction_name}_rx="
                f"{frames[frame_index]['id']}"
            ),
        ])

    path.write_text(
        "\n".join(lines) + "\n",
        encoding="utf-8"
    )

    print(f"IDLE_METADATA={path}")


def main():
    data = UNIT_DAT.read_bytes()

    if len(data) != EXPECTED_SIZE:
        raise SystemExit(
            "ERROR: unexpected unit.dat size "
            f"{len(data)}, expected "
            f"{EXPECTED_SIZE}"
        )

    palette = load_palette(
        PALETTE_PATH
    )

    OUTPUT_ROOT.mkdir(
        parents=True,
        exist_ok=True
    )

    results = {}

    print("=== GENERATE COMPLETE ORIGINAL BAKER PACK ===")
    print(f"UNITS_RX={UNITS_RX}")
    print(f"BAKER_INDEX={BAKER_INDEX}")
    print(
        f"BAKER_RECORD_OFFSET="
        f"{BAKER_RECORD_OFFSET}"
    )
    print(
        f"BAKER_SPRITE_OFFSET="
        f"{BAKER_SPRITE_OFFSET}"
    )
    print()

    for action_name, action_index in ACTIONS.items():
        directions = gather_action(
            data,
            action_name,
            action_index,
        )

        results[action_name] = (
            build_action_atlas(
                action_name,
                action_index,
                directions,
                palette,
            )
        )

        print()

    write_idle_metadata(
        results["walk"]
    )

    manifest = (
        OUTPUT_ROOT /
        "manifest.txt"
    )

    manifest.write_text(
        "\n".join([
            "unit=Baker",
            "unit_index=6",
            "source=original unit.dat + units.rx",
            "actions=walk,work,die,eat",
            "idle=walk_with_UnitStillFrames",
            "empty_actions=spec,work1,work2,workend,walkarm,walktool,walkbooty,walktool2,walkbooty2,unknown",
            "walk_directions=8",
            "walk_frames=8",
            "work_directions=8",
            "work_frames=1",
            "die_directions=8",
            "die_frames_by_direction=16,16,14,14,15,15,18,18",
            "eat_directions=8",
            "eat_frames=30",
            "unit_render_scale=32/40",
            "synthetic_frames=none",
            "temporary_assets=none",
        ]) + "\n",
        encoding="utf-8"
    )

    print(f"MANIFEST={manifest}")
    print("BAKER_ACTIONS=walk,work,die,eat")
    print("BAKER_IDLE=original_walk_still_frames")
    print("SYNTHETIC_FRAMES=NONE")
    print("TEMPORARY_ASSETS=NONE")
    print("COMPLETE_BAKER_PACK=READY")


if __name__ == "__main__":
    main()

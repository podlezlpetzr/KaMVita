import json
import re
import shutil
import struct
import subprocess
from pathlib import Path


ROOT = Path.home() / "KaMVita"

UNIT_DAT = (
    ROOT /
    "resources/original_kam/unit.dat"
)

UNITS_RX = (
    ROOT /
    "resources/original_kam/units.rx"
)

PALETTE = (
    ROOT /
    "resources/original_kam/pal0.bbm"
)

EXTRACTOR = (
    ROOT /
    "tools/extract_house.py"
)

OUTPUT_ROOT = (
    ROOT /
    "assets/units/serf/complete"
)

TEMP_ROOT = (
    ROOT /
    "assets/units/serf/_complete_build"
)


WARE_NAMES = [
    "trunk",
    "stone",
    "wood",
    "iron_ore",
    "gold_ore",
    "coal",
    "steel",
    "gold",
    "wine",
    "corn",
    "bread",
    "flour",
    "leather",
    "sausages",
    "pig",
    "skin",
    "shield",
    "metal_shield",
    "armor",
    "metal_armor",
    "axe",
    "sword",
    "pike",
    "halberd",
    "bow",
    "arbalet",
    "horse",
    "fish",
]

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

ACTION_NAMES = [
    "walk",
    "work",
    "spec",
    "die",
    "work1",
    "work2",
    "work_end",
    "eat",
    "walk_arm",
    "walk_tool",
    "walk_booty",
    "walk_tool2",
    "walk_booty2",
    "unknown",
]

WARE_COUNT = 28
DIRECTION_COUNT = 8
ACTION_COUNT = 14

ANIM_LOOP_SIZE = 70
UNIT_DAT_SIZE = 22
UNIT_SPRITE2_SIZE = 18 * 2

SERF_CARRY_SIZE = (
    WARE_COUNT *
    DIRECTION_COUNT *
    ANIM_LOOP_SIZE
)

UNIT_SPRITE_SIZE = (
    ACTION_COUNT *
    DIRECTION_COUNT *
    ANIM_LOOP_SIZE
)

RECORD_SIZE = (
    UNIT_DAT_SIZE +
    UNIT_SPRITE_SIZE +
    UNIT_SPRITE2_SIZE
)

SERF_INDEX = 0


def run(command):
    subprocess.run(
        command,
        check=True,
    )


def read_animation(data, offset):
    steps = list(
        struct.unpack_from(
            "<30h",
            data,
            offset,
        )
    )

    count = struct.unpack_from(
        "<h",
        data,
        offset + 60,
    )[0]

    count = max(
        0,
        min(count, 30),
    )

    return [
        step
        for step in steps[:count]
        if step >= 0
    ]


def read_frame_metadata(path):
    text = path.read_text(
        encoding="utf-8"
    )

    values = {}

    # Primarni format extract_house.py:
    # width=34
    # height=44
    # pivot_x=-12
    # pivot_y=-36
    for line in text.splitlines():
        if "=" not in line:
            continue

        key, value = line.split("=", 1)

        key = key.strip().lower()
        value = value.strip()

        if key not in {
            "width",
            "height",
            "pivot_x",
            "pivot_y",
        }:
            continue

        try:
            values[key] = int(value)
        except ValueError:
            pass

    # Podpora alternativniho ceskeho formatu.
    if len(values) < 4:
        size_match = re.search(
            r"Rozmery:\s*(-?\d+)\s*x\s*(-?\d+)",
            text,
            re.IGNORECASE,
        )

        pivot_match = re.search(
            r"Pivot:\s*(-?\d+)\s*,\s*(-?\d+)",
            text,
            re.IGNORECASE,
        )

        if size_match:
            values["width"] = int(
                size_match.group(1)
            )
            values["height"] = int(
                size_match.group(2)
            )

        if pivot_match:
            values["pivot_x"] = int(
                pivot_match.group(1)
            )
            values["pivot_y"] = int(
                pivot_match.group(2)
            )

    required = {
        "width",
        "height",
        "pivot_x",
        "pivot_y",
    }

    missing = required - values.keys()

    if missing:
        raise RuntimeError(
            f"Neplatna metadata: {path}; "
            f"chybi: {sorted(missing)}; "
            f"obsah: {text!r}"
        )

    return {
        "width": values["width"],
        "height": values["height"],
        "pivot_x": values["pivot_x"],
        "pivot_y": values["pivot_y"],
    }


def extract_sprite(sprite_id, output_stem):
    bmp_path = output_stem.with_suffix(".bmp")
    txt_path = output_stem.with_suffix(".txt")

    if (
        not bmp_path.is_file() or
        not txt_path.is_file()
    ):
        run([
            "python3",
            str(EXTRACTOR),
            str(UNITS_RX),
            str(PALETTE),
            str(sprite_id),
            str(bmp_path),
        ])

    metadata = read_frame_metadata(
        txt_path
    )

    return {
        "bmp": bmp_path,
        "sprite_id": sprite_id,
        **metadata,
    }


def normalize_and_build(
    name,
    steps_by_direction,
    output_directory,
):
    direction_count = len(
        steps_by_direction
    )

    frame_count = max(
        len(steps)
        for steps in steps_by_direction
    )

    if frame_count <= 0:
        raise RuntimeError(
            f"Akce {name} nema snimky."
        )

    source_directory = (
        TEMP_ROOT /
        "source" /
        name
    )

    source_directory.mkdir(
        parents=True,
        exist_ok=True,
    )

    frames = []

    for direction_index, steps in enumerate(
        steps_by_direction
    ):
        if not steps:
            raise RuntimeError(
                f"{name}: smer "
                f"{DIRECTIONS[direction_index]} "
                "nema zadny snimek."
            )

        # Kratsi smerove animace doplnime
        # jejich poslednim snimkem.
        padded_steps = list(steps)

        while len(padded_steps) < frame_count:
            padded_steps.append(
                padded_steps[-1]
            )

        for frame_index, unit_dat_id in enumerate(
            padded_steps
        ):
            # KaM renderer pouziva unit.dat ID + 1
            # jako sprite ID v units.rx.
            sprite_id = unit_dat_id + 1

            stem = (
                source_directory /
                (
                    f"{DIRECTIONS[direction_index]}"
                    f"_{frame_index:02d}"
                )
            )

            frame = extract_sprite(
                sprite_id,
                stem,
            )

            frame["direction"] = (
                direction_index
            )

            frame["frame"] = frame_index

            frames.append(frame)

    # Vypocet jednoho spolecneho souradneho
    # systemu pro cely atlas.
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
        maximum_x - minimum_x
    )

    cell_height = (
        maximum_y - minimum_y
    )

    if (
        cell_width <= 0 or
        cell_height <= 0
    ):
        raise RuntimeError(
            f"{name}: neplatna bunka "
            f"{cell_width}x{cell_height}"
        )

    normalized_directory = (
        TEMP_ROOT /
        "normalized" /
        name
    )

    normalized_directory.mkdir(
        parents=True,
        exist_ok=True,
    )

    rows = []

    for direction_index in range(
        direction_count
    ):
        normalized_frames = []

        for frame_index in range(
            frame_count
        ):
            frame = next(
                item
                for item in frames
                if (
                    item["direction"] ==
                    direction_index and
                    item["frame"] ==
                    frame_index
                )
            )

            offset_x = (
                frame["pivot_x"] -
                minimum_x
            )

            offset_y = (
                frame["pivot_y"] -
                minimum_y
            )

            normalized_path = (
                normalized_directory /
                (
                    f"d{direction_index}"
                    f"_f{frame_index:02d}.bmp"
                )
            )

            run([
                "convert",
                "-size",
                f"{cell_width}x{cell_height}",
                "xc:#ff00ff",
                str(frame["bmp"]),
                "-geometry",
                f"+{offset_x}+{offset_y}",
                "-composite",
                "-type",
                "TrueColor",
                f"BMP3:{normalized_path}",
            ])

            normalized_frames.append(
                normalized_path
            )

        row_path = (
            normalized_directory /
            f"row_{direction_index}.bmp"
        )

        run([
            "convert",
            *[
                str(path)
                for path in normalized_frames
            ],
            "+append",
            "-type",
            "TrueColor",
            f"BMP3:{row_path}",
        ])

        rows.append(row_path)

    output_directory.mkdir(
        parents=True,
        exist_ok=True,
    )

    atlas_path = (
        output_directory /
        f"{name}_atlas.bmp"
    )

    run([
        "convert",
        *[
            str(path)
            for path in rows
        ],
        "-append",
        "-type",
        "TrueColor",
        f"BMP3:{atlas_path}",
    ])

    metadata = {
        "name": name,
        "atlas": str(
            atlas_path.relative_to(ROOT)
        ),
        "cell_width": cell_width,
        "cell_height": cell_height,
        "frame_count": frame_count,
        "direction_count": direction_count,
        "anchor_x": minimum_x,
        "anchor_y": minimum_y,
        "direction_frame_counts": [
            len(steps)
            for steps in steps_by_direction
        ],
    }

    metadata_path = (
        output_directory /
        f"{name}_atlas.txt"
    )

    metadata_path.write_text(
        "\n".join([
            f"cell_width={cell_width}",
            f"cell_height={cell_height}",
            f"frame_count={frame_count}",
            (
                "direction_count="
                f"{direction_count}"
            ),
            f"anchor_x={minimum_x}",
            f"anchor_y={minimum_y}",
            (
                "direction_frame_counts="
                + ",".join(
                    str(len(steps))
                    for steps
                    in steps_by_direction
                )
            ),
            "",
        ]),
        encoding="utf-8",
    )

    print(
        f"OK {name}: "
        f"{cell_width}x{cell_height}, "
        f"{frame_count} snimku, "
        f"{direction_count} smeru"
    )

    return metadata


def get_standard_action(
    data,
    action_index,
):
    record_offset = (
        SERF_CARRY_SIZE +
        SERF_INDEX * RECORD_SIZE
    )

    sprite_offset = (
        record_offset +
        UNIT_DAT_SIZE
    )

    result = []

    for direction_index in range(
        DIRECTION_COUNT
    ):
        loop_index = (
            action_index *
            DIRECTION_COUNT +
            direction_index
        )

        offset = (
            sprite_offset +
            loop_index *
            ANIM_LOOP_SIZE
        )

        result.append(
            read_animation(
                data,
                offset,
            )
        )

    return result


def get_carry_action(
    data,
    ware_index,
):
    result = []

    for direction_index in range(
        DIRECTION_COUNT
    ):
        loop_index = (
            ware_index *
            DIRECTION_COUNT +
            direction_index
        )

        offset = (
            loop_index *
            ANIM_LOOP_SIZE
        )

        result.append(
            read_animation(
                data,
                offset,
            )
        )

    return result


def main():
    for required in [
        UNIT_DAT,
        UNITS_RX,
        PALETTE,
        EXTRACTOR,
    ]:
        if not required.is_file():
            raise SystemExit(
                f"Chybi: {required}"
            )

    data = UNIT_DAT.read_bytes()

    expected_size = (
        SERF_CARRY_SIZE +
        41 * RECORD_SIZE
    )

    if len(data) != expected_size:
        raise SystemExit(
            "Neocekavana velikost unit.dat: "
            f"{len(data)}, "
            f"ocekavano {expected_size}"
        )

    if TEMP_ROOT.exists():
        shutil.rmtree(TEMP_ROOT)

    TEMP_ROOT.mkdir(
        parents=True,
        exist_ok=True,
    )

    OUTPUT_ROOT.mkdir(
        parents=True,
        exist_ok=True,
    )

    manifest = {
        "unit": "serf",
        "standard_actions": {},
        "carry_actions": {},
    }

    # Pouze skutecne dostupne standardni akce.
    standard_actions = {
        "walk": 0,
        "die": 3,
        "eat": 7,
        "walk_arm": 8,
    }

    action_output = (
        OUTPUT_ROOT /
        "actions"
    )

    for name, action_index in (
        standard_actions.items()
    ):
        steps = get_standard_action(
            data,
            action_index,
        )

        if not all(steps):
            raise RuntimeError(
                f"Standardni akce {name} "
                "nema vsechny smery."
            )

        manifest[
            "standard_actions"
        ][name] = normalize_and_build(
            name,
            steps,
            action_output,
        )

    carry_output = (
        OUTPUT_ROOT /
        "carry"
    )

    for ware_index, ware_name in enumerate(
        WARE_NAMES
    ):
        # Pro aktualni KaMVita ekonomiku zatim
        # generujeme tri stavebni materialy.
        if ware_name not in {
            "trunk",
            "stone",
            "wood",
        }:
            continue

        steps = get_carry_action(
            data,
            ware_index,
        )

        if not all(steps):
            raise RuntimeError(
                f"Carry akce {ware_name} "
                "nema vsechny smery."
            )

        atlas_name = (
            f"carry_{ware_name}"
        )

        manifest[
            "carry_actions"
        ][ware_name] = normalize_and_build(
            atlas_name,
            steps,
            carry_output,
        )

    manifest_path = (
        OUTPUT_ROOT /
        "serf_manifest.json"
    )

    manifest_path.write_text(
        json.dumps(
            manifest,
            indent=2,
        ),
        encoding="utf-8",
    )

    shutil.rmtree(
        TEMP_ROOT,
        ignore_errors=True,
    )

    print()
    print("SERF PACK HOTOV:")
    print(OUTPUT_ROOT)
    print("Manifest:")
    print(manifest_path)


if __name__ == "__main__":
    main()

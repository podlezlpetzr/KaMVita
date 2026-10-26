import subprocess
from pathlib import Path

ROOT = Path.home() / "KaMVita"

EXTRACTOR = ROOT / "tools/extract_house.py"
UNITS_RX = ROOT / "resources/original_kam/units.rx"
PALETTE = ROOT / "resources/original_kam/pal0.bbm"

OUTPUT_ROOT = ROOT / "assets/units/serf"

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

# Hodnoty z unit.dat + 1 pro units.rx.
WALK_IDS = [
    list(range(81, 89)),    # N
    list(range(89, 97)),    # NE
    list(range(97, 105)),   # E
    list(range(105, 113)),  # SE
    list(range(113, 121)),  # S
    list(range(121, 129)),  # SW
    list(range(65, 73)),    # W
    list(range(73, 81)),    # NW
]

# WARE INDEX 1 = wt_Stone.
# Hodnoty z unit.dat + 1 pro units.rx.
CARRY_STONE_IDS = [
    list(range(585, 593)),  # N
    list(range(593, 601)),  # NE
    list(range(609, 617)),  # E
    list(range(617, 625)),  # SE
    list(range(641, 649)),  # S
    list(range(633, 641)),  # SW
    list(range(625, 633)),  # W
    list(range(601, 609)),  # NW
]


def read_metadata(path):
    values = {}

    for line in path.read_text().splitlines():
        if "=" not in line:
            continue

        key, value = line.split("=", 1)

        try:
            values[key.strip()] = int(value.strip())
        except ValueError:
            pass

    required = {
        "width",
        "height",
        "pivot_x",
        "pivot_y",
    }

    missing = required - values.keys()

    if missing:
        raise RuntimeError(
            f"V {path} chybi metadata: "
            + ", ".join(sorted(missing))
        )

    return values


def extract_set(name, ids_by_direction):
    output = OUTPUT_ROOT / name
    output.mkdir(parents=True, exist_ok=True)

    frames = []

    for direction_index, direction in enumerate(DIRECTIONS):
        for frame_index, sprite_id in enumerate(
            ids_by_direction[direction_index]
        ):
            stem = (
                f"{direction}_{frame_index:02d}"
            )

            bmp = output / f"{stem}.bmp"
            txt = output / f"{stem}.txt"

            if not bmp.is_file() or not txt.is_file():
                subprocess.run(
                    [
                        "python3",
                        str(EXTRACTOR),
                        str(UNITS_RX),
                        str(PALETTE),
                        str(sprite_id),
                        str(bmp),
                    ],
                    check=True,
                )

            metadata = read_metadata(txt)

            frames.append({
                "direction": direction_index,
                "frame": frame_index,
                "bmp": bmp,
                **metadata,
            })

    return output, frames


def build_atlas(name, frames):
    frame_count = 8
    direction_count = 8

    minimum_x = min(
        frame["pivot_x"]
        for frame in frames
    )

    minimum_y = min(
        frame["pivot_y"]
        for frame in frames
    )

    maximum_x = max(
        frame["pivot_x"] + frame["width"]
        for frame in frames
    )

    maximum_y = max(
        frame["pivot_y"] + frame["height"]
        for frame in frames
    )

    cell_width = maximum_x - minimum_x
    cell_height = maximum_y - minimum_y

    working = OUTPUT_ROOT / f"_{name}_build"
    working.mkdir(parents=True, exist_ok=True)

    rows = []

    try:
        for direction in range(direction_count):
            normalized_frames = []

            for frame_index in range(frame_count):
                frame = next(
                    item
                    for item in frames
                    if item["direction"] == direction
                    and item["frame"] == frame_index
                )

                normalized = (
                    working
                    / f"d{direction}_f{frame_index}.bmp"
                )

                offset_x = (
                    frame["pivot_x"] - minimum_x
                )

                offset_y = (
                    frame["pivot_y"] - minimum_y
                )

                subprocess.run(
                    [
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
                        f"BMP3:{normalized}",
                    ],
                    check=True,
                )

                normalized_frames.append(normalized)

            row = working / f"row_{direction}.bmp"

            subprocess.run(
                [
                    "convert",
                    *map(str, normalized_frames),
                    "+append",
                    "-type",
                    "TrueColor",
                    f"BMP3:{row}",
                ],
                check=True,
            )

            rows.append(row)

        atlas = OUTPUT_ROOT / f"{name}_atlas.bmp"

        subprocess.run(
            [
                "convert",
                *map(str, rows),
                "-append",
                "-type",
                "TrueColor",
                f"BMP3:{atlas}",
            ],
            check=True,
        )

        metadata = atlas.with_suffix(".txt")

        metadata.write_text(
            f"cell_width={cell_width}\n"
            f"cell_height={cell_height}\n"
            f"frame_count={frame_count}\n"
            f"direction_count={direction_count}\n"
            f"anchor_x={minimum_x}\n"
            f"anchor_y={minimum_y}\n"
        )

        print()
        print("Atlas:", atlas)
        print(
            "Rozmer:",
            cell_width * frame_count,
            "x",
            cell_height * direction_count,
        )
        print(
            "Bunka:",
            cell_width,
            "x",
            cell_height,
        )
        print(
            "Anchor:",
            minimum_x,
            minimum_y,
        )

    finally:
        subprocess.run(
            ["rm", "-rf", str(working)],
            check=True,
        )


OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)

_, walk_frames = extract_set(
    "walk",
    WALK_IDS,
)

build_atlas(
    "walk",
    walk_frames,
)

_, stone_frames = extract_set(
    "carry_stone",
    CARRY_STONE_IDS,
)

build_atlas(
    "carry_stone",
    stone_frames,
)

print()
print("OK: Serf walk a carry stone atlasy vytvoreny.")

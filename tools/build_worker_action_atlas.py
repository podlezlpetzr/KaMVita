import re
import subprocess
import sys
from pathlib import Path


def read_metadata(path):
    text = path.read_text(encoding="utf-8")
    values = {}

    patterns = {
        "width": r"(?:width|sirka|rozmer_x)\s*[=:]\s*(-?\d+)",
        "height": r"(?:height|vyska|rozmer_y)\s*[=:]\s*(-?\d+)",
        "pivot_x": r"pivot_x\s*[=:]\s*(-?\d+)",
        "pivot_y": r"pivot_y\s*[=:]\s*(-?\d+)",
    }

    for key, pattern in patterns.items():
        match = re.search(
            pattern,
            text,
            flags=re.IGNORECASE
        )

        if match:
            values[key] = int(match.group(1))

    if len(values) != 4:
        # Podpora puvodniho ceskeho vystupu extraktoru.
        size_match = re.search(
            r"Rozmery:\s*(\d+)\s*x\s*(\d+)",
            text,
            flags=re.IGNORECASE
        )

        pivot_match = re.search(
            r"Pivot:\s*(-?\d+)\s*,\s*(-?\d+)",
            text,
            flags=re.IGNORECASE
        )

        if size_match:
            values["width"] = int(size_match.group(1))
            values["height"] = int(size_match.group(2))

        if pivot_match:
            values["pivot_x"] = int(pivot_match.group(1))
            values["pivot_y"] = int(pivot_match.group(2))

    missing = {
        "width",
        "height",
        "pivot_x",
        "pivot_y",
    } - set(values)

    if missing:
        raise ValueError(
            f"V {path} chybi metadata: "
            + ", ".join(sorted(missing))
        )

    return values


if len(sys.argv) != 4:
    raise SystemExit(
        "Pouziti:\n"
        "  python3 build_worker_action_atlas.py "
        "<zdrojova_slozka> <pocet_snimku> <vystup.bmp>"
    )

source_dir = Path(sys.argv[1]).expanduser()
frame_count = int(sys.argv[2])
output_path = Path(sys.argv[3]).expanduser()

if frame_count <= 0:
    raise SystemExit("Pocet snimku musi byt kladny.")

frames = []

for index in range(frame_count):
    stem = f"frame_{index:02d}"
    bmp_path = source_dir / f"{stem}.bmp"
    metadata_path = source_dir / f"{stem}.txt"

    if not bmp_path.is_file():
        raise SystemExit(f"Chybi: {bmp_path}")

    if not metadata_path.is_file():
        raise SystemExit(f"Chybi: {metadata_path}")

    metadata = read_metadata(metadata_path)

    frames.append({
        "bmp": bmp_path,
        **metadata,
    })

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

if cell_width <= 0 or cell_height <= 0:
    raise SystemExit("Neplatny rozmer atlasove bunky.")

temporary_files = []

try:
    for index, frame in enumerate(frames):
        normalized_path = (
            source_dir /
            f"_normalized_{index:02d}.bmp"
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
                f"BMP3:{normalized_path}",
            ],
            check=True,
        )

        temporary_files.append(normalized_path)

    output_path.parent.mkdir(
        parents=True,
        exist_ok=True
    )

    subprocess.run(
        [
            "convert",
            *[str(path) for path in temporary_files],
            "+append",
            "-type",
            "TrueColor",
            f"BMP3:{output_path}",
        ],
        check=True,
    )

finally:
    for temporary_file in temporary_files:
        temporary_file.unlink(missing_ok=True)

metadata_output = output_path.with_suffix(".txt")

metadata_output.write_text(
    f"cell_width={cell_width}\n"
    f"cell_height={cell_height}\n"
    f"frame_count={frame_count}\n"
    f"anchor_x={minimum_x}\n"
    f"anchor_y={minimum_y}\n",
    encoding="utf-8",
)

print(f"Atlas: {output_path}")
print(
    f"Rozmer: "
    f"{cell_width * frame_count} x {cell_height}"
)
print(
    f"Bunka: {cell_width} x {cell_height}"
)
print(
    f"Anchor: {minimum_x}, {minimum_y}"
)
print(f"Snimku: {frame_count}")
print(f"Metadata: {metadata_output}")

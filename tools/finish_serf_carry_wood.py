import re
import subprocess
from pathlib import Path

root = Path.home() / "KaMVita"

source = (
    root /
    "assets/units/serf/_complete_build/source/carry_wood"
)

output_dir = (
    root /
    "assets/units/serf/complete/carry"
)

work = Path("/tmp/kamvita_carry_wood")

directions = [
    "n", "ne", "e", "se",
    "s", "sw", "w", "nw"
]


def metadata(path):
    text = path.read_text()

    values = {}

    for line in text.splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            key = key.strip().lower()

            if key in {
                "width",
                "height",
                "pivot_x",
                "pivot_y",
            }:
                values[key] = int(value.strip())

    if len(values) < 4:
        size = re.search(
            r"Rozmery:\s*(-?\d+)\s*x\s*(-?\d+)",
            text
        )

        pivot = re.search(
            r"Pivot:\s*(-?\d+)\s*,\s*(-?\d+)",
            text
        )

        if size:
            values["width"] = int(size.group(1))
            values["height"] = int(size.group(2))

        if pivot:
            values["pivot_x"] = int(pivot.group(1))
            values["pivot_y"] = int(pivot.group(2))

    return values


frames = []

for d_index, direction in enumerate(directions):
    for frame in range(8):
        stem = f"{direction}_{frame:02d}"

        bmp = source / f"{stem}.bmp"
        txt = source / f"{stem}.txt"

        if not bmp.is_file() or not txt.is_file():
            raise SystemExit(f"Chybi {stem}")

        meta = metadata(txt)

        frames.append({
            "direction": d_index,
            "frame": frame,
            "bmp": bmp,
            **meta,
        })


min_x = min(f["pivot_x"] for f in frames)
min_y = min(f["pivot_y"] for f in frames)

max_x = max(
    f["pivot_x"] + f["width"]
    for f in frames
)

max_y = max(
    f["pivot_y"] + f["height"]
    for f in frames
)

cell_w = max_x - min_x
cell_h = max_y - min_y

work.mkdir(
    parents=True,
    exist_ok=True
)

rows = []

for d_index in range(8):
    normalized = []

    for frame_index in range(8):
        frame = next(
            f for f in frames
            if f["direction"] == d_index
            and f["frame"] == frame_index
        )

        x = frame["pivot_x"] - min_x
        y = frame["pivot_y"] - min_y

        out = (
            work /
            f"d{d_index}_f{frame_index}.bmp"
        )

        subprocess.run([
            "convert",
            "-size",
            f"{cell_w}x{cell_h}",
            "xc:#ff00ff",
            str(frame["bmp"]),
            "-geometry",
            f"+{x}+{y}",
            "-composite",
            "-type",
            "TrueColor",
            f"BMP3:{out}",
        ], check=True)

        normalized.append(out)

    row = work / f"row_{d_index}.bmp"

    subprocess.run([
        "convert",
        *map(str, normalized),
        "+append",
        "-type",
        "TrueColor",
        f"BMP3:{row}",
    ], check=True)

    rows.append(row)


output_dir.mkdir(
    parents=True,
    exist_ok=True
)

atlas = output_dir / "carry_wood_atlas.bmp"

subprocess.run([
    "convert",
    *map(str, rows),
    "-append",
    "-type",
    "TrueColor",
    f"BMP3:{atlas}",
], check=True)


meta = output_dir / "carry_wood_atlas.txt"

meta.write_text(
    f"cell_width={cell_w}\n"
    f"cell_height={cell_h}\n"
    "frame_count=8\n"
    "direction_count=8\n"
    f"anchor_x={min_x}\n"
    f"anchor_y={min_y}\n"
    "direction_frame_counts=8,8,8,8,8,8,8,8\n"
)

print("OK:", atlas)
print("cell:", cell_w, "x", cell_h)
print("anchor:", min_x, min_y)

import re
import subprocess
from pathlib import Path

root = Path.home() / "KaMVita"
source = root / "assets/units/worker/work_n"
output = root / "assets/units/worker/work_n_atlas.bmp"

frames = []

for index in range(11):
    name = f"frame_{index:02}"
    bmp = source / f"{name}.bmp"
    txt = source / f"{name}.txt"

    values = {}

    for line in txt.read_text().splitlines():
        key, value = line.split("=", 1)
        values[key] = int(value)

    frames.append({
        "bmp": bmp,
        "width": values["width"],
        "height": values["height"],
        "pivot_x": values["pivot_x"],
        "pivot_y": values["pivot_y"],
    })

min_x = min(frame["pivot_x"] for frame in frames)
min_y = min(frame["pivot_y"] for frame in frames)

max_x = max(
    frame["pivot_x"] + frame["width"]
    for frame in frames
)

max_y = max(
    frame["pivot_y"] + frame["height"]
    for frame in frames
)

cell_width = max_x - min_x
cell_height = max_y - min_y

normalized = []

for index, frame in enumerate(frames):
    target = source / f"normalized_{index:02}.bmp"

    offset_x = frame["pivot_x"] - min_x
    offset_y = frame["pivot_y"] - min_y

    subprocess.run([
        "convert",
        "-size",
        f"{cell_width}x{cell_height}",
        "xc:#ff00ff",
        str(frame["bmp"]),
        "-geometry",
        f"+{offset_x}+{offset_y}",
        "-composite",
        f"BMP3:{target}",
    ], check=True)

    normalized.append(target)

subprocess.run([
    "convert",
    *map(str, normalized),
    "+append",
    f"BMP3:{output}",
], check=True)

metadata = output.with_suffix(".txt")

metadata.write_text(
    f"cell_width={cell_width}\n"
    f"cell_height={cell_height}\n"
    f"frame_count=11\n"
    f"anchor_x={min_x}\n"
    f"anchor_y={min_y}\n"
)

print("Atlas:", output)
print("Cell:", cell_width, "x", cell_height)
print("Anchor:", min_x, min_y)

from pathlib import Path
import sys

ROOT = Path.home() / "KaMVita"

sys.path.insert(0, str(ROOT / "tools"))

from generate_construction_atlas import (
    TRANSPARENT_RGB,
    load_palette,
    load_rx_sprite,
    write_bmp,
)

RX_PATH = ROOT / "resources/original_kam/houses.rx"
PAL_PATH = ROOT / "resources/original_kam/pal0.bbm"
OUT = ROOT / "assets/buildings/mill/runtime"

# Real Mill houses.dat record 22.

IDLE_IDS = [
    1095,1095,1095,1095,1095,
    1095,1095,1095,1095,1095,
    1095,
    1096,1097,1098,1099,
    1100,1100,1100,1100,1100,1100,
    1099,1098,1097,1096,
    1095,1095,1095,1095,1095,
]

WORK2_IDS = [
    881,882,883,884,
    885,886,887,888,
]

MOVE_X = 0
MOVE_Y = -1


def generate(name, action, ids):
    palette = load_palette(PAL_PATH)

    frames = [
        load_rx_sprite(RX_PATH, sprite_id)
        for sprite_id in ids
    ]

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

    columns = len(frames)
    rows = 1

    atlas_w = cell_w * columns
    atlas_h = cell_h

    atlas = [
        TRANSPARENT_RGB
        for _ in range(atlas_w * atlas_h)
    ]

    for index, frame in enumerate(frames):
        ox = (
            index * cell_w +
            frame["pivot_x"] -
            min_x
        )

        oy = frame["pivot_y"] - min_y

        for y in range(frame["height"]):
            for x in range(frame["width"]):
                src = y * frame["width"] + x
                pal = frame["pixels"][src]

                if pal == 0:
                    continue

                dx = ox + x
                dy = oy + y

                atlas[dy * atlas_w + dx] = (
                    palette[pal]
                )

    OUT.mkdir(parents=True, exist_ok=True)

    bmp = OUT / f"{name}_atlas.bmp"
    txt = OUT / f"{name}_atlas.txt"

    write_bmp(
        bmp,
        atlas_w,
        atlas_h,
        atlas
    )

    metadata = [
        "house_index=22",
        f"house_action={action}",
        f"cell_width={cell_w}",
        f"cell_height={cell_h}",
        f"frame_count={len(frames)}",
        f"columns={columns}",
        f"rows={rows}",
        f"anchor_x={min_x}",
        f"anchor_y={min_y}",
        f"move_x={MOVE_X}",
        f"move_y={MOVE_Y}",
        "rx_sequence=" +
        ",".join(map(str, ids)),
    ]

    txt.write_text(
        "\n".join(metadata) + "\n",
        encoding="utf-8"
    )

    print(f"=== {action} ===")
    print(f"ATLAS={bmp}")
    print(f"SIZE={atlas_w}x{atlas_h}")
    print(f"CELL={cell_w}x{cell_h}")
    print(f"FRAMES={len(frames)}")
    print(f"ANCHOR={min_x},{min_y}")
    print(f"MOVE={MOVE_X},{MOVE_Y}")
    print(
        "RX_SEQUENCE=" +
        ",".join(map(str, ids))
    )
    print()


def main():
    generate(
        "idle",
        "ha_Idle",
        IDLE_IDS
    )

    generate(
        "work2",
        "ha_Work2",
        WORK2_IDS
    )

    print("MILL_RUNTIME_ATLASES_READY")


if __name__ == "__main__":
    main()

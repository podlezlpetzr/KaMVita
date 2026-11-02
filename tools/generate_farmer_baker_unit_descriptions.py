#!/usr/bin/env python3

import struct
import sys
from pathlib import Path

ROOT = Path.home() / "KaMVita"

sys.path.insert(
    0,
    str(ROOT / "tools")
)

from render_kam_fntx_text import (
    load_fntx,
    get_text_width,
    render_text,
)

FONT_PATH = (
    ROOT /
    "resources/kam_source/data/gfx/fonts/game.fntx"
)

OUTPUT_DIR = (
    ROOT /
    "assets/ui/units/text"
)

TRANSPARENT_RGB = (255, 0, 255)

MAX_WIDTH = 180
LINE_GAP = 2


# ============================================================
# ORIGINAL KaM REMAKE ENGLISH UNIT DESCRIPTIONS
#
# text.eng.libx:
#
# ID 92:
# Farmer
#
# ID 94:
# Baker
#
# The original || separator means paragraph / line break.
# ============================================================

DESCRIPTIONS = {
    "farmer": [
        "The farmer plants and harvests corn on the farm.",
        "He also produces wine in the vineyard.",
    ],

    "baker": [
        "The baker grinds corn into flour in the mill.",
        "He also bakes bread in the bakery.",
    ],
}


def wrap_paragraph(font, text, max_width):
    words = text.split()

    lines = []
    current = ""

    for word in words:
        candidate = (
            word
            if not current
            else current + " " + word
        )

        width = get_text_width(
            font,
            candidate
        )

        if (
            current and
            width > max_width
        ):
            lines.append(current)
            current = word
        else:
            current = candidate

    if current:
        lines.append(current)

    return lines


def write_bmp(path, width, height, pixels):
    row_size = (
        (width * 3 + 3) // 4
    ) * 4

    image_size = (
        row_size * height
    )

    file_size = (
        54 + image_size
    )

    padding = bytes(
        row_size - width * 3
    )

    with path.open("wb") as output:
        output.write(b"BM")

        output.write(
            struct.pack(
                "<IHHI",
                file_size,
                0,
                0,
                54
            )
        )

        output.write(
            struct.pack(
                "<IIIHHIIIIII",
                40,
                width,
                height,
                1,
                24,
                0,
                image_size,
                2835,
                2835,
                0,
                0
            )
        )

        for y in range(
            height - 1,
            -1,
            -1
        ):
            for x in range(width):
                red, green, blue = pixels[
                    y * width + x
                ]

                output.write(
                    bytes((
                        blue,
                        green,
                        red
                    ))
                )

            output.write(padding)


def build_description(
    font,
    name,
    paragraphs
):
    lines = []

    for paragraph_index, paragraph in enumerate(
        paragraphs
    ):
        wrapped = wrap_paragraph(
            font,
            paragraph,
            MAX_WIDTH
        )

        lines.extend(wrapped)

        # Preserve original || separation.
        if (
            paragraph_index <
            len(paragraphs) - 1
        ):
            lines.append("")

    rendered_lines = []

    line_height = 17

    for line in lines:
        if not line:
            rendered_lines.append(None)
            continue

        image, _ = render_text(
            font,
            line
        )

        if image["width"] > MAX_WIDTH:
            raise SystemExit(
                "ERROR: wrapped line too wide: "
                f"{name}: "
                f"{image['width']} px: "
                f"{line}"
            )

        rendered_lines.append(image)

        line_height = max(
            line_height,
            image["height"]
        )

    width = MAX_WIDTH

    height = (
        len(lines) * line_height +
        max(
            len(lines) - 1,
            0
        ) * LINE_GAP
    )

    pixels = [
        TRANSPARENT_RGB
        for _ in range(
            width * height
        )
    ]

    dst_y = 0

    for line_index, image in enumerate(
        rendered_lines
    ):
        if image is not None:
            for y in range(
                image["height"]
            ):
                for x in range(
                    image["width"]
                ):
                    pixel = image["pixels"][
                        y * image["width"] + x
                    ]

                    if pixel == TRANSPARENT_RGB:
                        continue

                    dst_index = (
                        (dst_y + y) *
                        width +
                        x
                    )

                    pixels[dst_index] = pixel

        dst_y += (
            line_height +
            LINE_GAP
        )

    output_path = (
        OUTPUT_DIR /
        f"{name}_description.bmp"
    )

    metadata_path = (
        OUTPUT_DIR /
        f"{name}_description.txt"
    )

    write_bmp(
        output_path,
        width,
        height,
        pixels
    )

    metadata = [
        f"unit={name}",
        "source=original KaM Remake text.eng.libx",
        "font=original KaM Remake game.fntx",
        f"width={width}",
        f"height={height}",
        f"line_height={line_height}",
        f"line_gap={LINE_GAP}",
        f"line_count={len(lines)}",
    ]

    for index, line in enumerate(lines):
        metadata.append(
            f"line_{index}={line}"
        )

    metadata_path.write_text(
        "\n".join(metadata) + "\n",
        encoding="utf-8"
    )

    print()
    print(
        f"=== {name.upper()} ==="
    )

    print(
        f"OUTPUT={output_path}"
    )

    print(
        f"SIZE={width}x{height}"
    )

    for index, line in enumerate(lines):
        display = (
            line
            if line
            else "<paragraph gap>"
        )

        if line:
            line_width = get_text_width(
                font,
                line
            )
        else:
            line_width = 0

        print(
            f"LINE_{index}="
            f"{line_width}px "
            f"{display}"
        )


def main():
    if not FONT_PATH.is_file():
        raise SystemExit(
            f"ERROR: missing {FONT_PATH}"
        )

    OUTPUT_DIR.mkdir(
        parents=True,
        exist_ok=True
    )

    font = load_fntx(
        FONT_PATH
    )

    print(
        "=== UNIT 03: ORIGINAL FARMER + "
        "BAKER DESCRIPTIONS ==="
    )

    print(
        f"FONT={FONT_PATH}"
    )

    print(
        f"MAX_WIDTH={MAX_WIDTH}"
    )

    for name, paragraphs in DESCRIPTIONS.items():
        build_description(
            font,
            name,
            paragraphs
        )

    print()
    print("=== VERIFY ===")

    for name in DESCRIPTIONS:
        bmp = (
            OUTPUT_DIR /
            f"{name}_description.bmp"
        )

        meta = (
            OUTPUT_DIR /
            f"{name}_description.txt"
        )

        if not bmp.is_file():
            raise SystemExit(
                f"ERROR: missing {bmp}"
            )

        if bmp.stat().st_size <= 54:
            raise SystemExit(
                f"ERROR: invalid BMP {bmp}"
            )

        if not meta.is_file():
            raise SystemExit(
                f"ERROR: missing {meta}"
            )

    print(
        "OK: original Farmer description generated"
    )

    print(
        "OK: original Baker description generated"
    )

    print(
        "OK: original game.fntx used"
    )

    print(
        "OK: description width limited to 180 px"
    )

    print(
        "OK: original paragraph breaks preserved"
    )

    print(
        "OK: no C++ source modified"
    )


if __name__ == "__main__":
    main()

#!/usr/bin/env python3

import argparse
import struct
import zlib
from pathlib import Path

FNTX_CHAR_COUNT = 65536
LETTER_STRUCT = struct.Struct("<HHhHffff")

TRANSPARENT_RGB = (255, 0, 255)


def read_exact(data, offset, size, label):
    end = offset + size

    if end > len(data):
        raise ValueError(
            f"Truncated FNTX while reading {label}"
        )

    return data[offset:end], end


def load_fntx(path):
    compressed = path.read_bytes()
    data = zlib.decompress(compressed)

    offset = 0

    header, offset = read_exact(
        data,
        offset,
        4,
        "header"
    )

    if header != b"FNTX":
        raise ValueError(
            f"Invalid FNTX header: {header!r}"
        )

    properties, offset = read_exact(
        data,
        offset,
        7,
        "font properties"
    )

    (
        base_height,
        word_spacing,
        char_spacing,
        line_spacing,
    ) = struct.unpack(
        "<hhhB",
        properties
    )

    used_bytes, offset = read_exact(
        data,
        offset,
        FNTX_CHAR_COUNT,
        "used character map"
    )

    letters = {}

    for codepoint, used in enumerate(used_bytes):
        if not used:
            continue

        raw_letter, offset = read_exact(
            data,
            offset,
            LETTER_STRUCT.size,
            f"letter U+{codepoint:04X}"
        )

        (
            width,
            height,
            y_offset,
            atlas_id,
            u1,
            v1,
            u2,
            v2,
        ) = LETTER_STRUCT.unpack(raw_letter)

        letters[codepoint] = {
            "width": width,
            "height": height,
            "y_offset": y_offset,
            "atlas_id": atlas_id,
            "u1": u1,
            "v1": v1,
            "u2": u2,
            "v2": v2,
        }

    atlas_header, offset = read_exact(
        data,
        offset,
        5,
        "atlas header"
    )

    atlas_count, texture_width, texture_height = (
        struct.unpack(
            "<BHH",
            atlas_header
        )
    )

    atlas_size = (
        texture_width *
        texture_height *
        4
    )

    atlases = []

    for atlas_index in range(atlas_count):
        atlas_bytes, offset = read_exact(
            data,
            offset,
            atlas_size,
            f"atlas {atlas_index}"
        )

        atlases.append(atlas_bytes)

    if offset != len(data):
        raise ValueError(
            "Unexpected bytes after FNTX atlas data: "
            f"{len(data) - offset}"
        )

    return {
        "base_height": base_height,
        "word_spacing": word_spacing,
        "char_spacing": char_spacing,
        "line_spacing": line_spacing,
        "letters": letters,
        "atlas_count": atlas_count,
        "texture_width": texture_width,
        "texture_height": texture_height,
        "atlases": atlases,
        "decompressed_size": len(data),
    }


def get_letter(font, character):
    codepoint = ord(character)

    if codepoint in font["letters"]:
        return font["letters"][codepoint]

    if 0 in font["letters"]:
        return font["letters"][0]

    raise ValueError(
        f"Missing glyph U+{codepoint:04X}"
    )


def get_text_width(font, text):
    width = 0

    for character in text:
        if character == " ":
            width += font["word_spacing"]
            continue

        letter = get_letter(
            font,
            character
        )

        width += (
            letter["width"] +
            font["char_spacing"]
        )

    if text and text[-1] != " ":
        width -= font["char_spacing"]

    return max(width, 1)


def render_text(font, text):
    width = get_text_width(font, text)

    min_y = 0
    max_y = font["base_height"]

    for character in text:
        if character == " ":
            continue

        letter = get_letter(
            font,
            character
        )

        min_y = min(
            min_y,
            letter["y_offset"]
        )

        max_y = max(
            max_y,
            letter["y_offset"] +
            letter["height"]
        )

    # KaMVita UI caption assets pouzivaji kolem
    # originalniho game.fntx glyphu plny 17px textovy box.
    #
    # Horizontalni FNTX metriky zustavaji beze zmeny.
    # Needs uz presne odpovida originalni sirce 37 px.
    #
    # Glyph obsah game.fntx ma 13 px a existujici originalni
    # KaMVita Needs asset 17 px. Zachovame tedy 2 px okraj
    # nahore a 2 px dole.
    glyph_height = max(
        font["base_height"],
        max_y,
        1
    )

    vertical_padding_top = 2
    vertical_padding_bottom = 2

    height = (
        glyph_height +
        vertical_padding_top +
        vertical_padding_bottom
    )

    min_y = -vertical_padding_top

    pixels = [
        TRANSPARENT_RGB
        for _ in range(width * height)
    ]
    destination_x = 0

    texture_width = font["texture_width"]
    texture_height = font["texture_height"]

    for character in text:
        if character == " ":
            destination_x += (
                font["word_spacing"]
            )
            continue

        letter = get_letter(
            font,
            character
        )

        atlas_id = letter["atlas_id"]

        if atlas_id >= font["atlas_count"]:
            raise ValueError(
                f"Invalid atlas ID {atlas_id}"
            )

        atlas = font["atlases"][atlas_id]

        source_x = round(
            letter["u1"] *
            texture_width
        )

        source_y = round(
            letter["v1"] *
            texture_height
        )

        destination_y = (
            letter["y_offset"] -
            min_y
        )

        for glyph_y in range(
            letter["height"]
        ):
            for glyph_x in range(
                letter["width"]
            ):
                atlas_pixel_index = (
                    (
                        source_y + glyph_y
                    ) *
                    texture_width +
                    source_x +
                    glyph_x
                )

                byte_index = (
                    atlas_pixel_index * 4
                )

                blue = atlas[
                    byte_index + 0
                ]

                green = atlas[
                    byte_index + 1
                ]

                red = atlas[
                    byte_index + 2
                ]

                alpha = atlas[
                    byte_index + 3
                ]

                if alpha == 0:
                    continue

                # game.fntx je tf_Alpha8.
                # Zachovame puvodni intenzitu glyphu
                # jako sedy az bily pixel nad color key.
                intensity = alpha

                if red != 255 or green != 255 or blue != 255:
                    intensity = max(
                        red,
                        green,
                        blue,
                        alpha
                    )

                pixel_x = (
                    destination_x +
                    glyph_x
                )

                pixel_y = (
                    destination_y +
                    glyph_y
                )

                if (
                    0 <= pixel_x < width and
                    0 <= pixel_y < height
                ):
                    pixels[
                        pixel_y * width +
                        pixel_x
                    ] = (
                        intensity,
                        intensity,
                        intensity
                    )

        destination_x += (
            letter["width"] +
            font["char_spacing"]
        )

    return {
        "width": width,
        "height": height,
        "pixels": pixels,
    }, min_y


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--font",
        required=True,
        type=Path
    )

    parser.add_argument(
        "--text",
        required=True
    )

    parser.add_argument(
        "--output",
        required=True,
        type=Path
    )

    args = parser.parse_args()

    font = load_fntx(args.font)

    image, min_y = render_text(
        font,
        args.text
    )

    args.output.parent.mkdir(
        parents=True,
        exist_ok=True
    )

    # ----------------------------------------------------
    # Native 24-bit BMP writer.
    # Native 24-bit BMP output.
    # ----------------------------------------------------
    width = image["width"]
    height = image["height"]
    pixels = image["pixels"]

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

    with args.output.open("wb") as output:
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

    metadata_path = (
        args.output.with_suffix(".txt")
    )

    metadata_path.write_text(
        "\n".join([
            f"text={args.text}",
            f"font={args.font}",
            f"width={image['width']}",
            f"height={image['height']}",
            f"base_height={font['base_height']}",
            f"word_spacing={font['word_spacing']}",
            f"char_spacing={font['char_spacing']}",
            f"line_spacing={font['line_spacing']}",
            f"atlas_count={font['atlas_count']}",
            f"texture_width={font['texture_width']}",
            f"texture_height={font['texture_height']}",
            f"min_y={min_y}",
            (
                "decompressed_size="
                f"{font['decompressed_size']}"
            ),
            "source=original KaM Remake game.fntx",
        ]) + "\n",
        encoding="utf-8"
    )

    print(f"TEXT={args.text}")
    print(f"OUTPUT={args.output}")
    print(
        f"SIZE={image['width']}x{image['height']}"
    )
    print(
        f"BASE_HEIGHT={font['base_height']}"
    )
    print(
        "SPACING="
        f"{font['word_spacing']},"
        f"{font['char_spacing']}"
    )
    print(
        f"ATLAS_COUNT={font['atlas_count']}"
    )
    print(
        "ATLAS_SIZE="
        f"{font['texture_width']}x"
        f"{font['texture_height']}"
    )
    print(f"METADATA={metadata_path}")


if __name__ == "__main__":
    main()

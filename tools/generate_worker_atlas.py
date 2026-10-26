import struct
from pathlib import Path


ROOT = Path.home() / "KaMVita"

UNITS_RX = (
    ROOT
    / "resources/original_kam/units.rx"
)

PALETTE_PATH = (
    ROOT
    / "resources/original_kam/pal0.bbm"
)

OUTPUT_PATH = (
    ROOT
    / "assets/units/worker/walk_atlas.bmp"
)

METADATA_PATH = (
    ROOT
    / "assets/units/worker/walk_atlas.txt"
)

PALETTE_OFFSET = 48
PALETTE_SIZE = 768
TRANSPARENT_RGB = (255, 0, 255)

DIRECTIONS = [
    "N",
    "NE",
    "E",
    "SE",
    "S",
    "SW",
    "W",
    "NW",
]

# Hodnoty z unit.dat + 1, jak používá původní renderer.
WALK_SPRITES = {
    "N": [
        1264, 1265, 1266, 1267,
        1275, 1276, 1277, 1278,
    ],
    "NE": [
        1287, 1288, 1289, 1290,
        1291, 1292, 1293, 1294,
    ],
    "E": [
        1295, 1296, 1297, 1298,
        1299, 1300, 1301, 1302,
    ],
    "SE": [
        1303, 1304, 1305, 1306,
        1307, 1308, 1309, 1310,
    ],
    "S": [
        1327, 1328, 1329, 1330,
        1331, 1332, 1333, 1334,
    ],
    "SW": [
        1319, 1320, 1321, 1322,
        1323, 1324, 1325, 1326,
    ],
    "W": [
        1311, 1312, 1313, 1314,
        1315, 1316, 1317, 1318,
    ],
    "NW": [
        1279, 1280, 1281, 1282,
        1283, 1284, 1285, 1286,
    ],
}


def read_exact(file, size):
    data = file.read(size)

    if len(data) != size:
        raise EOFError(
            f"Ocekavano {size} B, "
            f"nacteno {len(data)} B"
        )

    return data


def load_palette(path):
    raw = path.read_bytes()

    palette_data = raw[
        PALETTE_OFFSET:
        PALETTE_OFFSET + PALETTE_SIZE
    ]

    if len(palette_data) != PALETTE_SIZE:
        raise ValueError("Neplatna paleta")

    return [
        tuple(palette_data[index:index + 3])
        for index in range(0, PALETTE_SIZE, 3)
    ]


def load_rx_sprites(path, wanted_ids):
    result = {}

    with path.open("rb") as file:
        sprite_count = struct.unpack(
            "<I",
            read_exact(file, 4)
        )[0]

        used = read_exact(file, sprite_count)

        for sprite_id in range(1, sprite_count + 1):
            if used[sprite_id - 1] != 1:
                continue

            width, height = struct.unpack(
                "<hh",
                read_exact(file, 4)
            )

            pivot_x, pivot_y = struct.unpack(
                "<ii",
                read_exact(file, 8)
            )

            pixels = read_exact(
                file,
                width * height
            )

            if sprite_id in wanted_ids:
                result[sprite_id] = {
                    "id": sprite_id,
                    "width": width,
                    "height": height,
                    "pivot_x": pivot_x,
                    "pivot_y": pivot_y,
                    "pixels": pixels,
                }

    missing = wanted_ids - set(result)

    if missing:
        raise ValueError(
            "Nenalezeny sprity: "
            + ", ".join(
                str(value)
                for value in sorted(missing)
            )
        )

    return result


def write_bmp(path, width, height, pixels):
    row_size = ((width * 3 + 3) // 4) * 4
    image_size = row_size * height
    file_size = 54 + image_size

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

        padding = b"\x00" * (
            row_size - width * 3
        )

        for y in range(height - 1, -1, -1):
            for x in range(width):
                red, green, blue = pixels[
                    y * width + x
                ]

                output.write(
                    bytes((blue, green, red))
                )

            output.write(padding)


all_ids = {
    sprite_id
    for direction in DIRECTIONS
    for sprite_id in WALK_SPRITES[direction]
}

palette = load_palette(PALETTE_PATH)

sprites = load_rx_sprites(
    UNITS_RX,
    all_ids
)

# Společný prostor všech snímků vůči pivotu.
minimum_x = min(
    sprite["pivot_x"]
    for sprite in sprites.values()
)

minimum_y = min(
    sprite["pivot_y"]
    for sprite in sprites.values()
)

maximum_x = max(
    sprite["pivot_x"] + sprite["width"]
    for sprite in sprites.values()
)

maximum_y = max(
    sprite["pivot_y"] + sprite["height"]
    for sprite in sprites.values()
)

cell_width = maximum_x - minimum_x
cell_height = maximum_y - minimum_y

columns = 8
rows = 8

atlas_width = cell_width * columns
atlas_height = cell_height * rows

atlas = [
    TRANSPARENT_RGB
    for _ in range(atlas_width * atlas_height)
]

metadata_lines = [
    f"cell_width={cell_width}",
    f"cell_height={cell_height}",
    f"columns={columns}",
    f"rows={rows}",
    f"anchor_x={minimum_x}",
    f"anchor_y={minimum_y}",
    "frames_per_direction=8",
]

for direction_index, direction in enumerate(DIRECTIONS):
    for frame_index, sprite_id in enumerate(
        WALK_SPRITES[direction]
    ):
        sprite = sprites[sprite_id]

        cell_x = frame_index * cell_width
        cell_y = direction_index * cell_height

        sprite_x = (
            cell_x
            + sprite["pivot_x"]
            - minimum_x
        )

        sprite_y = (
            cell_y
            + sprite["pivot_y"]
            - minimum_y
        )

        for source_y in range(sprite["height"]):
            for source_x in range(sprite["width"]):
                source_index = (
                    source_y * sprite["width"]
                    + source_x
                )

                palette_index = sprite["pixels"][
                    source_index
                ]

                if palette_index == 0:
                    continue

                destination_x = sprite_x + source_x
                destination_y = sprite_y + source_y

                destination_index = (
                    destination_y * atlas_width
                    + destination_x
                )

                atlas[destination_index] = (
                    palette[palette_index]
                )

        metadata_lines.append(
            f"{direction}_{frame_index}="
            f"id:{sprite_id},"
            f"width:{sprite['width']},"
            f"height:{sprite['height']},"
            f"pivot_x:{sprite['pivot_x']},"
            f"pivot_y:{sprite['pivot_y']}"
        )

OUTPUT_PATH.parent.mkdir(
    parents=True,
    exist_ok=True
)

write_bmp(
    OUTPUT_PATH,
    atlas_width,
    atlas_height,
    atlas
)

METADATA_PATH.write_text(
    "\n".join(metadata_lines) + "\n",
    encoding="utf-8"
)

print(f"Atlas: {OUTPUT_PATH}")
print(
    f"Rozmer atlasu: "
    f"{atlas_width} x {atlas_height}"
)
print(
    f"Velikost bunky: "
    f"{cell_width} x {cell_height}"
)
print(
    f"Spolecny anchor: "
    f"{minimum_x}, {minimum_y}"
)
print(f"Metadata: {METADATA_PATH}")

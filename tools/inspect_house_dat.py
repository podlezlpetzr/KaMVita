import struct
from pathlib import Path

HOUSE_DAT = (
    Path.home()
    / "KaMVita/resources/original_kam/houses.dat"
)

BEAST_ANIM_SIZE = 2100
RECORD_SIZE = 1688

HOUSES = {
    7: "Bakery",
    8: "Farm",
    11: "Store",
    13: "School",
}


def read_i16(data, offset):
    return struct.unpack_from("<h", data, offset)[0]


def read_u16(data, offset):
    return struct.unpack_from("<H", data, offset)[0]


def read_u8(data, offset):
    return data[offset]


def read_i8(data, offset):
    return struct.unpack_from(
        "<b",
        data,
        offset
    )[0]


data = HOUSE_DAT.read_bytes()
expected_size = BEAST_ANIM_SIZE + 29 * RECORD_SIZE

print(f"Soubor: {HOUSE_DAT}")
print(f"Velikost: {len(data)} B")
print(f"Ocekavana velikost: {expected_size} B")
print()

if len(data) != expected_size:
    raise SystemExit(
        "Velikost houses.dat neodpovida "
        "2100 + 29 * 1688."
    )

for house_index, name in HOUSES.items():
    record_offset = (
        BEAST_ANIM_SIZE
        + house_index * RECORD_SIZE
    )

    record = data[
        record_offset:
        record_offset + RECORD_SIZE
    ]

    stone_pic = read_i16(record, 0)
    wood_pic = read_i16(record, 2)
    wood_pal = read_i16(record, 4)
    stone_pal = read_i16(record, 6)

    wood_pic_steps = read_u16(record, 1418)
    stone_pic_steps = read_u16(record, 1420)

    entrance_offset_x = read_i8(
        record, 1424
    )

    entrance_offset_y = read_i8(
        record, 1425
    )

    entrance_offset_x_px = read_i8(
        record, 1426
    )

    entrance_offset_y_px = read_i8(
        record, 1427
    )

    wood_cost = read_u8(record, 1528)
    stone_cost = read_u8(record, 1529)

    print(f"{name}:")
    print(f"  House index: {house_index}")
    print(f"  Record offset: {record_offset}")

    print(f"  StonePic DAT: {stone_pic}")
    print(f"  StonePic RX:  {stone_pic + 1}")

    print(f"  WoodPic DAT:  {wood_pic}")
    print(f"  WoodPic RX:   {wood_pic + 1}")

    print(f"  WoodPal DAT:  {wood_pal}")
    print(f"  WoodPal RX:   {wood_pal + 1}")

    print(f"  StonePal DAT: {stone_pal}")
    print(f"  StonePal RX:  {stone_pal + 1}")

    print(f"  WoodPicSteps: {wood_pic_steps}")
    print(f"  StonePicSteps: {stone_pic_steps}")

    print(
        f"  EntranceOffset: "
        f"({entrance_offset_x}, "
        f"{entrance_offset_y})"
    )

    print(
        f"  EntranceOffsetPx: "
        f"({entrance_offset_x_px}, "
        f"{entrance_offset_y_px})"
    )

    print(f"  WoodCost: {wood_cost}")
    print(f"  StoneCost: {stone_cost}")
    print()

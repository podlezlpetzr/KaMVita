import struct
from pathlib import Path

UNIT_DAT = (
    Path.home()
    / "KaMVita/resources/original_kam/unit.dat"
)

WARE_COUNT = 28
DIRECTION_COUNT = 8
ANIM_LOOP_SIZE = 70

SERF_CARRY_SIZE = (
    WARE_COUNT
    * DIRECTION_COUNT
    * ANIM_LOOP_SIZE
)

UNIT_DAT_SIZE = 22
ACTION_COUNT = 14

UNIT_SPRITE_SIZE = (
    ACTION_COUNT
    * DIRECTION_COUNT
    * ANIM_LOOP_SIZE
)

UNIT_SPRITE2_SIZE = 18 * 2

RECORD_SIZE = (
    UNIT_DAT_SIZE
    + UNIT_SPRITE_SIZE
    + UNIT_SPRITE2_SIZE
)

SERF_INDEX = 0

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


def read_anim_loop(data, offset):
    steps = list(
        struct.unpack_from("<30h", data, offset)
    )

    count = struct.unpack_from(
        "<h",
        data,
        offset + 60
    )[0]

    move_x = struct.unpack_from(
        "<i",
        data,
        offset + 62
    )[0]

    move_y = struct.unpack_from(
        "<i",
        data,
        offset + 66
    )[0]

    count = max(0, min(count, 30))

    return {
        "steps": steps[:count],
        "count": count,
        "move_x": move_x,
        "move_y": move_y,
    }


data = UNIT_DAT.read_bytes()

print("Soubor:", UNIT_DAT)
print("Velikost:", len(data))
print("Serf carry blok:", SERF_CARRY_SIZE)
print("Unit record:", RECORD_SIZE)
print()

print("=" * 70)
print("SERF: bezna ua_Walk animace")
print("=" * 70)

serf_record_offset = (
    SERF_CARRY_SIZE
    + SERF_INDEX * RECORD_SIZE
)

serf_sprite_offset = (
    serf_record_offset
    + UNIT_DAT_SIZE
)

# ua_Walk je action index 0.
for direction_index, direction in enumerate(
    DIRECTIONS
):
    loop_offset = (
        serf_sprite_offset
        + direction_index * ANIM_LOOP_SIZE
    )

    animation = read_anim_loop(
        data,
        loop_offset
    )

    print(
        f"{direction:2} "
        f"count={animation['count']:2} "
        f"move=({animation['move_x']},"
        f"{animation['move_y']}) "
        f"steps={animation['steps']}"
    )

print()
print("=" * 70)
print("SERF: carry animace podle ware indexu")
print("=" * 70)

for ware_index in range(WARE_COUNT):
    print()
    print(f"WARE INDEX {ware_index}")

    for direction_index, direction in enumerate(
        DIRECTIONS
    ):
        loop_index = (
            ware_index * DIRECTION_COUNT
            + direction_index
        )

        loop_offset = (
            loop_index * ANIM_LOOP_SIZE
        )

        animation = read_anim_loop(
            data,
            loop_offset
        )

        print(
            f"  {direction:2} "
            f"count={animation['count']:2} "
            f"steps={animation['steps']}"
        )

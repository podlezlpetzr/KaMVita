import struct
from pathlib import Path

UNIT_DAT = (
    Path.home()
    / "KaMVita/resources/original_kam/unit.dat"
)

SERF_CARRY_SIZE = 28 * 8 * 70
UNIT_DAT_SIZE = 22
ANIM_LOOP_SIZE = 70
ACTION_COUNT = 14
DIRECTION_COUNT = 8
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

WORKER_INDEX = 9

ACTIONS = [
    "ua_Walk",
    "ua_Work",
    "ua_Spec",
    "ua_Die",
    "ua_Work1",
    "ua_Work2",
    "ua_WorkEnd",
    "ua_Eat",
    "ua_WalkArm",
    "ua_WalkTool",
    "ua_WalkBooty",
    "ua_WalkTool2",
    "ua_WalkBooty2",
    "ua_Unknown",
]

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

    return {
        "steps": steps,
        "count": count,
        "move_x": move_x,
        "move_y": move_y,
    }


data = UNIT_DAT.read_bytes()

expected_size = (
    SERF_CARRY_SIZE
    + 41 * RECORD_SIZE
)

print(f"Soubor: {UNIT_DAT}")
print(f"Velikost: {len(data)} B")
print(f"Ocekavano: {expected_size} B")
print(f"SerfCarry: {SERF_CARRY_SIZE} B")
print(f"Unit record: {RECORD_SIZE} B")

if len(data) != expected_size:
    raise SystemExit(
        "Velikost unit.dat neodpovida vypoctu."
    )

worker_offset = (
    SERF_CARRY_SIZE
    + WORKER_INDEX * RECORD_SIZE
)

sprite_offset = (
    worker_offset
    + UNIT_DAT_SIZE
)

print(f"Worker index: {WORKER_INDEX}")
print(f"Worker offset: {worker_offset}")
print(f"Worker sprite offset: {sprite_offset}")
print()

wanted_actions = {
    0,  # ua_Walk
    1,  # ua_Work
    4,  # ua_Work1
    5,  # ua_Work2
    6,  # ua_WorkEnd
    9,  # ua_WalkTool
}

for action_index in sorted(wanted_actions):
    print("=" * 60)
    print(
        f"Akce {action_index}: "
        f"{ACTIONS[action_index]}"
    )

    for direction_index, direction in enumerate(
        DIRECTIONS
    ):
        loop_index = (
            action_index * DIRECTION_COUNT
            + direction_index
        )

        loop_offset = (
            sprite_offset
            + loop_index * ANIM_LOOP_SIZE
        )

        animation = read_anim_loop(
            data,
            loop_offset
        )

        count = max(
            0,
            min(animation["count"], 30)
        )

        valid_steps = animation["steps"][:count]

        print(
            f"  {direction:2} "
            f"count={count:2} "
            f"move=({animation['move_x']},"
            f"{animation['move_y']}) "
            f"steps={valid_steps}"
        )

import struct
from pathlib import Path

root = Path.home() / "KaMVita"
unit_dat = root / "resources/original_kam/unit.dat"

ware_count = 28
direction_count = 8
action_count = 14
anim_loop_size = 70
unit_dat_size = 22
unit_sprite2_size = 18 * 2

serf_carry_size = (
    ware_count *
    direction_count *
    anim_loop_size
)

unit_sprite_size = (
    action_count *
    direction_count *
    anim_loop_size
)

record_size = (
    unit_dat_size +
    unit_sprite_size +
    unit_sprite2_size
)

serf_index = 0

actions = [
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

directions = [
    "N",
    "NE",
    "E",
    "SE",
    "S",
    "SW",
    "W",
    "NW",
]


def read_animation(data, offset):
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
        "count": count,
        "steps": steps[:count],
        "move_x": move_x,
        "move_y": move_y,
    }


data = unit_dat.read_bytes()

expected_size = (
    serf_carry_size +
    41 * record_size
)

if len(data) != expected_size:
    raise SystemExit(
        f"Neocekavana velikost unit.dat: "
        f"{len(data)}, ocekavano {expected_size}"
    )

record_offset = (
    serf_carry_size +
    serf_index * record_size
)

sprite_offset = (
    record_offset +
    unit_dat_size
)

print("SERF STANDARD ACTIONS")
print("unit.dat:", unit_dat)
print("record offset:", record_offset)
print("sprite offset:", sprite_offset)
print()

for action_index, action_name in enumerate(actions):
    available = False
    action_rows = []

    for direction_index, direction in enumerate(
        directions
    ):
        loop_index = (
            action_index * direction_count +
            direction_index
        )

        offset = (
            sprite_offset +
            loop_index * anim_loop_size
        )

        animation = read_animation(
            data,
            offset
        )

        valid_steps = [
            step
            for step in animation["steps"]
            if step >= 0
        ]

        if valid_steps:
            available = True

        action_rows.append(
            (
                direction,
                animation,
                valid_steps,
            )
        )

    status = (
        "AVAILABLE"
        if available
        else "EMPTY"
    )

    print("=" * 72)
    print(
        f"ACTION {action_index}: "
        f"{action_name} [{status}]"
    )

    for direction, animation, valid_steps in action_rows:
        print(
            f"  {direction:2} "
            f"count={animation['count']:2} "
            f"move=("
            f"{animation['move_x']},"
            f"{animation['move_y']}) "
            f"steps={valid_steps}"
        )

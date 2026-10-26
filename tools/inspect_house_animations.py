import struct
from pathlib import Path

ROOT = Path.home() / "KaMVita"

HOUSE_DAT = (
    ROOT /
    "resources/original_kam/houses.dat"
)

BEAST_ANIM_SIZE = 2100
RECORD_SIZE = 1688

ANIM_OFFSET = 88
ANIM_LOOP_SIZE = 70

HOUSE_ACTIONS = [
    "ha_Work1",
    "ha_Work2",
    "ha_Work3",
    "ha_Work4",
    "ha_Work5",
    "ha_Smoke",
    "ha_Flagpole",
    "ha_Idle",
    "ha_Flag1",
    "ha_Flag2",
    "ha_Flag3",
    "ha_Fire1",
    "ha_Fire2",
    "ha_Fire3",
    "ha_Fire4",
    "ha_Fire5",
    "ha_Fire6",
    "ha_Fire7",
    "ha_Fire8",
]

HOUSES = {
    7: "Bakery",
    8: "Farm",
    11: "Store",
    13: "School",
}


def read_animation(record, action_index):
    offset = (
        ANIM_OFFSET +
        action_index * ANIM_LOOP_SIZE
    )

    steps = list(
        struct.unpack_from(
            "<30h",
            record,
            offset,
        )
    )

    count = struct.unpack_from(
        "<h",
        record,
        offset + 60,
    )[0]

    move_x = struct.unpack_from(
        "<i",
        record,
        offset + 62,
    )[0]

    move_y = struct.unpack_from(
        "<i",
        record,
        offset + 66,
    )[0]

    count = max(
        0,
        min(count, 30),
    )

    valid_steps = [
        step
        for step in steps[:count]
        if step >= 0
    ]

    return {
        "count": count,
        "steps": valid_steps,
        "move_x": move_x,
        "move_y": move_y,
    }


data = HOUSE_DAT.read_bytes()

expected_size = (
    BEAST_ANIM_SIZE +
    29 * RECORD_SIZE
)

if len(data) != expected_size:
    raise SystemExit(
        f"Neocekavana velikost houses.dat: "
        f"{len(data)}, "
        f"ocekavano {expected_size}"
    )

for house_index, house_name in HOUSES.items():
    record_offset = (
        BEAST_ANIM_SIZE +
        house_index * RECORD_SIZE
    )

    record = data[
        record_offset:
        record_offset + RECORD_SIZE
    ]

    print("=" * 78)
    print(
        f"{house_name.upper()} "
        f"[house index {house_index}]"
    )
    print("=" * 78)

    available_count = 0

    for action_index, action_name in enumerate(
        HOUSE_ACTIONS
    ):
        animation = read_animation(
            record,
            action_index,
        )

        available = bool(
            animation["steps"]
        )

        if available:
            available_count += 1

        status = (
            "AVAILABLE"
            if available
            else "EMPTY"
        )

        rx_steps = [
            step + 1
            for step in animation["steps"]
        ]

        print(
            f"{action_index:02d} "
            f"{action_name:11} "
            f"[{status:9}] "
            f"count={animation['count']:2} "
            f"move=("
            f"{animation['move_x']},"
            f"{animation['move_y']})"
        )

        if available:
            print(
                "   DAT:",
                animation["steps"],
            )

            print(
                "   RX: ",
                rx_steps,
            )

    print(
        f"Available actions: {available_count}"
    )
    print()

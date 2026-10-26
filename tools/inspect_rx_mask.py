import struct
import sys
from collections import Counter
from pathlib import Path


def read_exact(file, size):
    data = file.read(size)

    if len(data) != size:
        raise EOFError(
            f"Ocekavano {size} B, nacteno {len(data)} B"
        )

    return data


def inspect(rx_path, sprite_ids):
    rx_path = Path(rx_path)

    with rx_path.open("rb") as file:
        sprite_count = struct.unpack(
            "<I",
            read_exact(file, 4)
        )[0]

        used = read_exact(file, sprite_count)

        for current_id in range(1, sprite_count + 1):
            if used[current_id - 1] != 1:
                continue

            width, height = struct.unpack(
                "<hh",
                read_exact(file, 4)
            )

            pivot_x, pivot_y = struct.unpack(
                "<ii",
                read_exact(file, 8)
            )

            pixels = read_exact(file, width * height)

            if current_id not in sprite_ids:
                continue

            counts = Counter(pixels)
            nonzero = [value for value in pixels if value != 0]

            print()
            print(f"Sprite ID: {current_id}")
            print(f"Rozmery: {width} x {height}")
            print(f"Pivot: {pivot_x}, {pivot_y}")
            print(f"Nulovych pixelu: {counts[0]}")

            if nonzero:
                print(f"Minimum mimo nulu: {min(nonzero)}")
                print(f"Maximum: {max(nonzero)}")
                print(
                    "Pouzitych hodnot:",
                    len(set(nonzero))
                )

            print("Nejcastejsi hodnoty:")

            for value, count in counts.most_common(20):
                print(
                    f"  {value:3}: {count:6}"
                )


if __name__ == "__main__":
    if len(sys.argv) < 3:
        raise SystemExit(
            "Pouziti: inspect_rx_mask.py "
            "houses.rx id [id ...]"
        )

    inspect(
        sys.argv[1],
        {int(value) for value in sys.argv[2:]}
    )

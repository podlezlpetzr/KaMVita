import struct
import sys
from pathlib import Path

TARGET_ID = 138

def read_exact(file, size):
    data = file.read(size)
    if len(data) != size:
        raise EOFError(
            f"Neocekavany konec souboru: "
            f"ocekavano {size} B, nacteno {len(data)} B"
        )
    return data

def inspect(path):
    path = Path(path)

    with path.open("rb") as file:
        house_qty = struct.unpack("<I", read_exact(file, 4))[0]
        used_flags = read_exact(file, house_qty)

        print(f"\nSoubor: {path}")
        print(f"Velikost: {path.stat().st_size} B")
        print(f"HouseQty: {house_qty}")
        print(f"Pouzitych zaznamu: {sum(flag == 1 for flag in used_flags)}")

        found = False

        # Pascal pole v editoru pouziva indexy 1..HouseQty
        for sprite_id in range(1, house_qty + 1):
            if used_flags[sprite_id - 1] != 1:
                continue

            width, height = struct.unpack(
                "<hh",
                read_exact(file, 4)
            )

            pivot_x, pivot_y = struct.unpack(
                "<ii",
                read_exact(file, 8)
            )

            pixel_count = width * height

            if width < 0 or height < 0 or pixel_count > 100_000_000:
                raise ValueError(
                    f"Neplatny zaznam {sprite_id}: "
                    f"{width}x{height}"
                )

            pixels = read_exact(file, pixel_count)

            if sprite_id == TARGET_ID:
                nonzero = sum(pixel != 0 for pixel in pixels)

                print(f"Sprite ID: {sprite_id}")
                print(f"Rozmery: {width} x {height}")
                print(f"Pivot: {pivot_x}, {pivot_y}")
                print(f"Pixelu: {pixel_count}")
                print(f"Nepruhlednych pixelu: {nonzero}")
                print(f"Pozice konce zaznamu: {file.tell()} B")

                found = True

        if not found:
            print(f"Sprite {TARGET_ID} v souboru nebyl nalezen.")

        print(f"Pozice po parsovani: {file.tell()} B")
        print(f"Zbyva neprecteno: {path.stat().st_size - file.tell()} B")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit(
            "Pouziti: python3 inspect_houses_rx.py houses.rx [houses.rx ...]"
        )

    for filename in sys.argv[1:]:
        try:
            inspect(filename)
        except Exception as error:
            print(f"\nCHYBA u {filename}: {error}")

import importlib.util
import json
import shutil
from pathlib import Path


ROOT = Path.home() / "KaMVita"

SHARED_BUILDER = (
    ROOT /
    "tools/build_complete_serf_pack.py"
)

OUTPUT_ROOT = (
    ROOT /
    "assets/units/worker/complete"
)

TEMP_ROOT = (
    ROOT /
    "assets/units/worker/_complete_build"
)

WORKER_INDEX = 9


def load_shared_builder():
    specification = (
        importlib.util.spec_from_file_location(
            "kamvita_unit_builder",
            SHARED_BUILDER,
        )
    )

    if (
        specification is None or
        specification.loader is None
    ):
        raise RuntimeError(
            "Nelze nacist sdileny asset builder."
        )

    module = (
        importlib.util.module_from_spec(
            specification
        )
    )

    specification.loader.exec_module(
        module
    )

    return module


def main():
    if not SHARED_BUILDER.is_file():
        raise SystemExit(
            f"Chybi: {SHARED_BUILDER}"
        )

    builder = load_shared_builder()

    # Serf i Worker jsou v unit.dat za
    # specialnim Serf carry blokem.
    builder.SERF_INDEX = WORKER_INDEX

    builder.OUTPUT_ROOT = OUTPUT_ROOT
    builder.TEMP_ROOT = TEMP_ROOT

    if TEMP_ROOT.exists():
        shutil.rmtree(TEMP_ROOT)

    if OUTPUT_ROOT.exists():
        shutil.rmtree(OUTPUT_ROOT)

    TEMP_ROOT.mkdir(
        parents=True,
        exist_ok=True,
    )

    OUTPUT_ROOT.mkdir(
        parents=True,
        exist_ok=True,
    )

    data = builder.UNIT_DAT.read_bytes()

    expected_size = (
        builder.SERF_CARRY_SIZE +
        41 * builder.RECORD_SIZE
    )

    if len(data) != expected_size:
        raise SystemExit(
            "Neocekavana velikost unit.dat: "
            f"{len(data)}, "
            f"ocekavano {expected_size}"
        )

    # Inventura z worker_all_actions.txt:
    # 0 = ua_Walk
    # 1 = ua_Work
    # 3 = ua_Die
    # 4 = ua_Work1
    # 5 = ua_Work2
    # 7 = ua_Eat
    actions = {
        "walk": 0,
        "work": 1,
        "die": 3,
        "work1": 4,
        "work2": 5,
        "eat": 7,
    }

    action_output = (
        OUTPUT_ROOT /
        "actions"
    )

    manifest = {
        "unit": "worker",
        "unit_index": WORKER_INDEX,
        "standard_actions": {},
    }

    for name, action_index in actions.items():
        steps = builder.get_standard_action(
            data,
            action_index,
        )

        if not all(steps):
            missing_directions = [
                builder.DIRECTIONS[index]
                for index, direction_steps
                in enumerate(steps)
                if not direction_steps
            ]

            raise RuntimeError(
                f"Worker akce {name} nema smery: "
                + ", ".join(missing_directions)
            )

        manifest[
            "standard_actions"
        ][name] = builder.normalize_and_build(
            name,
            steps,
            action_output,
        )

    manifest_path = (
        OUTPUT_ROOT /
        "worker_manifest.json"
    )

    manifest_path.write_text(
        json.dumps(
            manifest,
            indent=2,
        ),
        encoding="utf-8",
    )

    shutil.rmtree(
        TEMP_ROOT,
        ignore_errors=True,
    )

    print()
    print("WORKER PACK HOTOV:")
    print(OUTPUT_ROOT)
    print("Manifest:")
    print(manifest_path)


if __name__ == "__main__":
    main()

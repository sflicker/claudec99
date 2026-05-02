#!/usr/bin/env python3

import os
import re
import sys


def derive_stage_label(path: str) -> str:
    base = os.path.basename(path)
    stem = re.sub(r"\.[^.]+$", "", base)

    parts = stem.split("-")

    try:
        stage_index = parts.index("stage")
    except ValueError:
        raise ValueError("filename does not contain '-stage-'")

    numbers = []
    for part in parts[stage_index + 1:]:
        if part.isdigit():
            numbers.append(part)
        else:
            break

    if not numbers:
        raise ValueError("no numeric stage components found after '-stage-'")

    return "stage-" + "-".join(numbers)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: stage-label.py <stage-spec-path>", file=sys.stderr)
        return 2

    try:
        print(derive_stage_label(sys.argv[1]))
        return 0
    except ValueError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())

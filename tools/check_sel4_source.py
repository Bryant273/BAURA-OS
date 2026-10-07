#!/usr/bin/env python3
"""Reject seL4 source trees whose tracked symlinks were copied as text files."""

import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {Path(sys.argv[0]).name} SEL4_SOURCE_DIR", file=sys.stderr)
        return 2

    source = Path(sys.argv[1]).resolve()
    links = (
        Path("libsel4/tools/condition.py"),
        Path("libsel4/tools/syscall_header_gen.py"),
        Path("libsel4/tools/invocation_header_gen.py"),
        Path("libsel4/tools/changed.sh"),
        Path("libsel4/tools/bitfield_gen.py"),
        Path("libsel4/tools/xmllint.sh"),
    )
    invalid = [
        link
        for link in links
        if not (source / link).is_symlink()
        or not (source / link).resolve().is_file()
    ]
    if invalid:
        print(
            "seL4 checkout is incomplete: expected Git symlinks are missing or "
            "were copied as text files:",
            file=sys.stderr,
        )
        for link in invalid:
            print(f"  {link.as_posix()}", file=sys.stderr)
        print(
            "Initialize KERNEL/L4/sel4 as a Git submodule in a Linux filesystem "
            "(for example, clone with --recurse-submodules inside WSL). Do not "
            "copy the submodule from Windows/NTFS into WSL.",
            file=sys.stderr,
        )
        return 1

    print("seL4 source symlinks: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

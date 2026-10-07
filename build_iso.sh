#!/usr/bin/env bash
set -euo pipefail

make clean
make legacy32-iso

printf 'ISO legacy x86 32 bits créée : baura_os.iso\n'

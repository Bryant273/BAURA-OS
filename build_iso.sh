#!/usr/bin/env bash
set -euo pipefail

make clean
make iso

printf 'ISO créée : baura_os.iso\n'

#!/usr/bin/env bash

set -euo pipefail

if [[ ! -f helper.hpp ]]; then
    printf '%s\n' 'helper.hpp is missing. Run: unswbc update algo_bot' >&2
    exit 1
fi

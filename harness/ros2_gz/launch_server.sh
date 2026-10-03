#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Headless gz-sim server for a fixed iteration count.
# The world has no Latch plugin and does not elect a mode.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
WORLD="${ROOT}/worlds/empty.sdf"

if ! command -v gz >/dev/null 2>&1; then
  echo "gz-sim binary absent; launch not run"
  exit 2
fi

# -v 4 is the verbosity this gz build uses for the server shutdown line
# printed when the requested iteration count has been executed.
exec gz sim -s -r -v 4 --iterations 5 "${WORLD}"

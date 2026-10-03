#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Scan published text against a local word list.
# The list is gitignored and untracked. When that file is absent, skip.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LIST="${ROOT}/scripts/public_boundary_denylist.local"
if [[ ! -f "${LIST}" ]]; then
  echo "public-boundary lint skipped (local word list absent)"
  exit 0
fi
python3 - "$ROOT" "$LIST" <<'PY'
import pathlib, re, subprocess, sys
root = pathlib.Path(sys.argv[1])
list_path = pathlib.Path(sys.argv[2]).resolve()
patterns = []
for raw in list_path.read_text(encoding="utf-8").splitlines():
    line = raw.strip()
    if not line or line.startswith("#"):
        continue
    patterns.append(re.compile(line))
if not patterns:
    print("public-boundary lint skipped (local word list empty)")
    sys.exit(0)
listed = subprocess.check_output(
    ["git", "ls-files", "--cached", "--others", "--exclude-standard", "-z"],
    cwd=root,
)
hits = []
for rel_text in listed.decode("utf-8").split("\0"):
    if not rel_text:
        continue
    path = (root / rel_text).resolve()
    if path == list_path or not path.is_file():
        continue
    try:
        text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        continue
    lines = text.splitlines()
    rel = path.relative_to(root)
    for i, line in enumerate(lines, 1):
        blob = line if i == len(lines) else line + " " + lines[i]
        for pat in patterns:
            if pat.search(blob):
                hits.append(f"{rel}:{i}: banned")
                break
if hits:
    print("public-boundary lint failed:")
    print("\n".join(hits))
    sys.exit(1)
print("public-boundary lint OK")
PY

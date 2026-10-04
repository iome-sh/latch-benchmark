#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Classifier and claim-line checks. Does not write a fixture into this repo.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck disable=SC1091
source "${ROOT}/scripts/public_fixture_wire.sh"

fail() {
  echo "FAIL $*" >&2
  exit 1
}

assert_eq() {
  local got="$1"
  local want="$2"
  local label="$3"
  if [[ "${got}" != "${want}" ]]; then
    printf 'FAIL %s\n got: %s\n want: %s\n' "${label}" "${got}" "${want}" >&2
    exit 1
  fi
}

out="$(bm01_after_run "noise")"
assert_eq "${out}" "BM-01 60/60 not claimed" "missing match prints not claimed"

out="$(bm01_after_run $'prefix\nBM-01 Latch bit-match 60/60\nsuffix')"
assert_eq "${out}" "" "exact 60/60 line suppresses not claimed"

out="$(bm01_after_run "BM-01 Latch bit-match 61/61")"
assert_eq "${out}" "BM-01 60/60 not claimed" "61 rows stays not claimed"

out="$(bm01_after_run "BM-01 mock-oracle 10 rows (not Latch 60/60 bit-match)")"
assert_eq "${out}" "BM-01 60/60 not claimed" "mock disclaimer stays not claimed"

rows="$(jsonl_data_rows "${ROOT}/${MOCK_REL}")"
assert_eq "${rows}" "10" "mock oracle row count"

classify_tracked "${ROOT}"
assert_eq "${#golden_files[@]}" "0" "this tree has no 60-row export"
assert_eq "${#session_files[@]}" "0" "this tree has no session export"

tmp="$(mktemp -d)"
cleanup() { rm -rf "${tmp}"; }
trap cleanup EXIT

git -C "${tmp}" init -q
git -C "${tmp}" config user.email "fixture-wire@example.com"
git -C "${tmp}" config user.name "fixture-wire"

mkdir -p "${tmp}/fixtures"
cp "${ROOT}/${MOCK_REL}" "${tmp}/fixtures/mock-oracle.jsonl.example"
python3 - "${tmp}/fixtures/short.jsonl" <<'PY'
import sys
path = sys.argv[1]
with open(path, "w", encoding="utf-8") as fh:
    for i in range(9):
        fh.write('{"id":"r%d","evidence":"not-a-hash"}\n' % i)
PY
git -C "${tmp}" add fixtures
git -C "${tmp}" commit -q -m "short rows"
classify_tracked "${tmp}"
assert_eq "${#golden_files[@]}" "0" "short and example files are not a 60-row golden"
assert_eq "${#session_files[@]}" "0" "short file is not a session"

python3 - "${tmp}/fixtures/rows60.jsonl" <<'PY'
import sys
path = sys.argv[1]
with open(path, "w", encoding="utf-8") as fh:
    for i in range(60):
        fh.write('{"id":"r%d","evidence":"not-a-hash"}\n' % i)
PY
git -C "${tmp}" add fixtures/rows60.jsonl
git -C "${tmp}" commit -q -m "sixty rows"
classify_tracked "${tmp}"
assert_eq "${#golden_files[@]}" "1" "tracked 60-row file is a golden candidate"
assert_eq "${#session_files[@]}" "0" "60-row file without session keys is not a session"

python3 - "${tmp}/fixtures/untracked60.jsonl" <<'PY'
import sys
path = sys.argv[1]
with open(path, "w", encoding="utf-8") as fh:
    for i in range(60):
        fh.write('{"id":"u%d","evidence":"not-a-hash"}\n' % i)
PY
classify_tracked "${tmp}"
assert_eq "${#golden_files[@]}" "1" "untracked 60-row file is not a candidate"

python3 - "${tmp}/fixtures/session.jsonl" <<'PY'
import sys
path = sys.argv[1]
with open(path, "w", encoding="utf-8") as fh:
    fh.write('{"session":"one","snapshot_id":"fixture-snap","evidence":"not-a-hash","id":"s"}\n')
PY
git -C "${tmp}" add fixtures/session.jsonl
git -C "${tmp}" commit -q -m "session row"
classify_tracked "${tmp}"
assert_eq "${#session_files[@]}" "1" "tracked session file is a session candidate"
snap="$(snapshot_id_of "${tmp}/fixtures/session.jsonl")"
assert_eq "${snap}" "fixture-snap" "snapshot id is read from the file"

python3 - "${tmp}/fixtures/mixed.jsonl" <<'PY'
import sys
path = sys.argv[1]
with open(path, "w", encoding="utf-8") as fh:
    fh.write('{"session":"one","snapshot_id":"a","evidence":"not-a-hash"}\n')
    fh.write('{"session":"one","snapshot_id":"b","evidence":"not-a-hash"}\n')
PY
if snapshot_id_of "${tmp}/fixtures/mixed.jsonl" >/dev/null 2>&1; then
  fail "mixed snapshot ids must not resolve"
fi

unset LATCH_LIB_DIR LATCH_LIBRARY LATCH_GOLDENS LATCH_SESSION LATCH_SNAPSHOT_ID
wire="$(bash "${ROOT}/scripts/public_fixture_wire.sh")"
printf '%s\n' "${wire}" | grep -qx 'BM-01 60/60 not claimed' || fail "public run did not print not claimed"
printf '%s\n' "${wire}" | grep -q 'BM-01 60-row golden: Gap' || fail "public run did not leave BM-01 Gap"
printf '%s\n' "${wire}" | grep -qx 'BM-09 evidence replay: Gap (no tracked exported session fixture the verify path can recompute)' || fail "BM-09 not Gap"
printf '%s\n' "${wire}" | grep -qx 'BM-10 two-process soak: Gap (no tracked exported session fixture the verify path can recompute)' || fail "BM-10 not Gap"
if printf '%s\n' "${wire}" | grep -qx 'BM-01 Latch bit-match 60/60'; then
  fail "public run claimed 60/60"
fi

echo "public fixture wire tests passed"

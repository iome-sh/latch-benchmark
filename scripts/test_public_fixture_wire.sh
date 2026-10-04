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

rows="$(jsonl_data_rows "${ROOT}/fixtures/latch-robot-1.jsonl")"
assert_eq "${rows}" "60" "tracked golden row count"

classify_tracked "${ROOT}"
assert_eq "${#golden_files[@]}" "1" "tracked 60-row golden"
assert_eq "${#session_files[@]}" "0" "golden file is not a session fixture"
snap="$(snapshot_id_of "${ROOT}/fixtures/latch-robot-1.jsonl")"
assert_eq "${snap}" "latch-robot-1" "snapshot key"

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

unset LATCH_LIB_DIR LATCH_LIBRARY
out="$(report_linked_golden 'BM-09 mock-oracle 60 rows evidence match (not a Latch evidence-v2 certificate)')"
printf '%s\n' "${out}" | grep -qx 'BM-09: Gap' || fail "mock recompute marked BM-09 Exists"
printf '%s\n' "${out}" | grep -qx 'L2: Gap' || fail "mock recompute marked L2 Exists"
if printf '%s\n' "${out}" | grep -q ': Exists'; then
  fail "unset library marked a row Exists"
fi
LATCH_LIBRARY=/tmp/not-a-committed-library.so
out="$(report_linked_golden $'BM-09 mock-oracle 60 rows evidence match (not a Latch evidence-v2 certificate)\nBM-10 mock-oracle 60 rows two-process match (not a Latch evidence-v2 certificate)')"
if printf '%s\n' "${out}" | grep -q ': Exists'; then
  fail "mock recompute marked Exists while a library variable was set"
fi
out="$(report_linked_golden $'BM-09 Latch library evidence match 10 rows\nBM-10 Latch library two-process match 10 rows')"
if printf '%s\n' "${out}" | grep -q ': Exists'; then
  fail "short provided-library match marked Exists"
fi
out="$(report_linked_golden 'BM-09 Latch library evidence match 60 rows')"
if printf '%s\n' "${out}" | grep -q ': Exists'; then
  fail "verify without soak marked Exists"
fi
out="$(report_linked_golden $'BM-09 Latch library evidence match 60 rows\nBM-10 Latch library two-process match 60 rows')"
printf '%s\n' "${out}" | grep -qx 'BM-09: Exists' || fail "60-row provided match left BM-09 Gap"
printf '%s\n' "${out}" | grep -qx 'BM-10: Exists' || fail "60-row provided match left BM-10 Gap"
printf '%s\n' "${out}" | grep -qx 'L2: Exists' || fail "60-row provided match left L2 Gap"
unset LATCH_LIBRARY
out="$(report_linked_golden $'BM-09 Latch library evidence match 60 rows\nBM-10 Latch library two-process match 60 rows')"
if printf '%s\n' "${out}" | grep -q ': Exists'; then
  fail "60-row text without a library marked Exists"
fi

unset LATCH_LIB_DIR LATCH_LIBRARY LATCH_GOLDENS LATCH_SESSION LATCH_SNAPSHOT_ID
wire="$(bash "${ROOT}/scripts/public_fixture_wire.sh")"
if printf '%s\n' "${wire}" | grep -qx 'BM-01 Latch bit-match 60/60'; then
  fail "public mock printed Latch bit-match"
fi
printf '%s\n' "${wire}" | grep -qx 'BM-01 mock-oracle 60 rows (not Latch 60/60 bit-match)' || fail "mock 60 recompute missing"
printf '%s\n' "${wire}" | grep -qx 'BM-01 mock-oracle recompute: Exists' || fail "mock recompute not Exists"
printf '%s\n' "${wire}" | grep -qx 'BM-01 60/60 not claimed' || fail "public run did not print not claimed"
printf '%s\n' "${wire}" | grep -qx 'BM-01 Latch bit-match: Gap' || fail "Latch bit-match not Gap"
printf '%s\n' "${wire}" | grep -qx 'BM-09: Gap' || fail "BM-09 not Gap"
printf '%s\n' "${wire}" | grep -qx 'BM-10: Gap' || fail "BM-10 not Gap"
printf '%s\n' "${wire}" | grep -qx 'L2: Gap' || fail "L2 not Gap"
if printf '%s\n' "${wire}" | grep -qx 'BM-09: Exists'; then
  fail "BM-09 marked Exists"
fi
if printf '%s\n' "${wire}" | grep -qx 'BM-10: Exists'; then
  fail "BM-10 marked Exists"
fi
if printf '%s\n' "${wire}" | grep -qx 'L2: Exists'; then
  fail "L2 marked Exists"
fi

mujoco_prefix="${ROOT}/build/mujoco/_deps/mujoco-3.14.0"
if [[ ! -f "${mujoco_prefix}/include/mujoco/mujoco.h" ]]; then
  fail "mujoco prefix missing after the public configure"
fi

probe="${tmp}/provided-lib"
marker="${probe}/marker"
mkdir -p "${probe}/mismatch" "${probe}/match"
cat > "${probe}/mismatch.c" <<'EOF'
#include "latch_abi.h"

#include <stdio.h>
#include <string.h>

static void mark(void) {
  FILE *fp = fopen(PROBE_MARKER, "a");
  if (!fp) return;
  fputs("called\n", fp);
  fclose(fp);
}

int latch_bind(LatchState *state, const LatchBlob *blob) {
  mark();
  if (!state || !blob || blob->schema != LATCH_BLOB_SCHEMA) return 1;
  memset(state, 0, sizeof *state);
  state->blob = blob;
  return 0;
}

int latch_consider(const LatchSense *in, LatchState *state, int budget_hit, LatchMode *out) {
  (void)in;
  (void)budget_hit;
  mark();
  if (!state || !out) return 1;
  memset(out, 0, sizeof *out);
  out->name = LATCH_HOLD;
  out->plane = LATCH_PLANE_L2;
  return 0;
}

uint32_t latch_legal_mask(const LatchSense *in) {
  (void)in;
  mark();
  return 0xffffffffu;
}

void latch_evidence(const LatchState *state, const LatchSense *in, const LatchMode *mode, char out[17]) {
  (void)state;
  (void)in;
  (void)mode;
  mark();
  memcpy(out, "0123456789abcdef", 17);
}

int latch_apply_reject(const LatchSense *decision_sense, const LatchSense *apply_sense, const LatchMode *decision,
                       LatchMode *out) {
  (void)decision_sense;
  (void)apply_sense;
  mark();
  if (!decision || !out) return -1;
  *out = *decision;
  return 0;
}
EOF

gcc -shared -fPIC -I "${ROOT}/harness/mujoco/include" \
  -DPROBE_MARKER="\"${marker}\"" \
  -Wl,-soname,liblatch.so \
  -o "${probe}/mismatch/liblatch.so" \
  "${probe}/mismatch.c"
gcc -shared -fPIC -I "${ROOT}/harness/mujoco/include" \
  -Wl,-soname,liblatch.so \
  -o "${probe}/match/liblatch.so" \
  "${ROOT}/harness/mujoco/src/latch_mock.c"

configure_provided() {
  local dest="$1"
  local lib="$2"
  cmake -S "${ROOT}/harness/mujoco" -B "${dest}" \
    -DMUJOCO_DIR="${mujoco_prefix}" \
    -DLATCH_LIBRARY="${lib}" -ULATCH_LIB_DIR
}

assert_calls_library() {
  local bin="$1"
  local lib="$2"
  local lib_dir=""
  lib_dir="$(dirname "${lib}")"
  nm -D "${bin}" | awk '$1 == "U" && $2 == "latch_consider" { found = 1 } END { exit !found }' \
    || fail "latch_consider is not a dynamic reference in ${bin}"
  env -u LD_LIBRARY_PATH ldd "${bin}" | grep -F "${lib}" >/dev/null \
    || fail "ldd did not resolve ${bin} to ${lib}"
  readelf -d "${bin}" | grep -E 'RPATH|RUNPATH' | grep -F "${lib_dir}" >/dev/null \
    || fail "runtime search path for ${bin} omits ${lib_dir}"
}

echo "building harness against a mismatched provided library"
configure_provided "${ROOT}/build/mujoco-provided-mismatch" "${probe}/mismatch/liblatch.so"
set +e
mismatch_build="$(env -u LD_LIBRARY_PATH -u LATCH_LIB_DIR -u LATCH_LIBRARY \
  LATCH_GOLDENS="${ROOT}/fixtures/latch-robot-1.jsonl" \
  LATCH_SNAPSHOT_ID=latch-robot-1 \
  cmake --build "${ROOT}/build/mujoco-provided-mismatch" 2>&1)"
mismatch_rc=$?
set -e
if [[ "${mismatch_rc}" -eq 0 ]]; then
  fail "mismatched provided library was treated as a passing run"
fi
if printf '%s\n' "${mismatch_build}" | grep -qx 'BM-01 Latch bit-match 60/60'; then
  fail "mismatched provided library printed Latch bit-match"
fi
if [[ ! -s "${marker}" ]]; then
  fail "mismatched provided library was not called"
fi
assert_calls_library "${ROOT}/build/mujoco-provided-mismatch/lbs_bm" "${probe}/mismatch/liblatch.so"

echo "building harness against a matching provided library"
configure_provided "${ROOT}/build/mujoco-provided-match" "${probe}/match/liblatch.so"
match_build="$(env -u LD_LIBRARY_PATH -u LATCH_LIB_DIR -u LATCH_LIBRARY \
  LATCH_GOLDENS="${ROOT}/fixtures/latch-robot-1.jsonl" \
  LATCH_SNAPSHOT_ID=latch-robot-1 \
  cmake --build "${ROOT}/build/mujoco-provided-match" 2>&1)"
printf '%s\n' "${match_build}"
printf '%s\n' "${match_build}" | grep -qx 'BM-01 Latch bit-match 60/60' \
  || fail "matching provided library did not print Latch bit-match"
printf '%s\n' "${match_build}" | grep -qx 'oracle=latch' || fail "matching run was not the provided library"
printf '%s\n' "${match_build}" | grep -qx 'BM-09 Latch library evidence match 60 rows' \
  || fail "matching provided library did not compare evidence"
printf '%s\n' "${match_build}" | grep -qx 'BM-10 Latch library two-process match 60 rows' \
  || fail "matching provided library did not compare the two-process soak"
assert_calls_library "${ROOT}/build/mujoco-provided-match/lbs_bm" "${probe}/match/liblatch.so"
direct="$(env -u LD_LIBRARY_PATH LATCH_GOLDENS="${ROOT}/fixtures/latch-robot-1.jsonl" \
  "${ROOT}/build/mujoco-provided-match/lbs_bm")"
printf '%s\n' "${direct}" | grep -qx 'BM-01 Latch bit-match 60/60' \
  || fail "direct run did not keep the Latch bit-match"
set +e
short_verify="$(env -u LD_LIBRARY_PATH \
  LATCH_SESSION="${ROOT}/harness/mujoco/fixtures/mock-oracle.jsonl.example" \
  LATCH_SNAPSHOT_ID=lbs-mock-oracle \
  "${ROOT}/build/mujoco-provided-match/lbs_verify" 2>&1)"
short_verify_rc=$?
short_soak="$(env -u LD_LIBRARY_PATH \
  LATCH_SESSION="${ROOT}/harness/mujoco/fixtures/mock-oracle.jsonl.example" \
  LATCH_SNAPSHOT_ID=lbs-mock-oracle \
  "${ROOT}/build/mujoco-provided-match/lbs_soak" 2>&1)"
short_soak_rc=$?
set -e
if [[ "${short_verify_rc}" -eq 0 || "${short_soak_rc}" -eq 0 ]]; then
  fail "10-row provided-library replay was treated as the 60-row golden"
fi
if printf '%s\n' "${short_verify}" | grep -qx 'BM-09 Latch library evidence match 60 rows'; then
  fail "10-row verify printed a 60-row evidence match"
fi
if printf '%s\n' "${short_soak}" | grep -qx 'BM-10 Latch library two-process match 60 rows'; then
  fail "10-row soak printed a 60-row two-process match"
fi

restore_mock_build() {
  if [[ ! -d "${ROOT}/build/mujoco" ]]; then
    return 0
  fi
  env -u LATCH_LIB_DIR -u LATCH_LIBRARY cmake -S "${ROOT}/harness/mujoco" -B "${ROOT}/build/mujoco" \
    -DMUJOCO_DIR="${mujoco_prefix}" -ULATCH_LIB_DIR -ULATCH_LIBRARY
  cmake --build "${ROOT}/build/mujoco" --target lbs_bm --target lbs_verify --target lbs_soak --target lbs_noalloc --target lbs_hand --target lbs_contact
}
trap 'restore_mock_build; cleanup' EXIT

echo "wire script with a matching provided library"
match_wire="$(env -u LD_LIBRARY_PATH -u LATCH_LIB_DIR LATCH_LIBRARY="${probe}/match/liblatch.so" \
  bash "${ROOT}/scripts/public_fixture_wire.sh")"
printf '%s\n' "${match_wire}"
printf '%s\n' "${match_wire}" | grep -qx 'BM-01 Latch bit-match 60/60' \
  || fail "wire script hid a matching provided-library bit-match"
if printf '%s\n' "${match_wire}" | grep -qx 'BM-01 Latch bit-match: Gap'; then
  fail "wire script left Latch bit-match Gap after a matching provided library"
fi
printf '%s\n' "${match_wire}" | grep -qx 'BM-09: Exists' || fail "matching library left BM-09 Gap"
printf '%s\n' "${match_wire}" | grep -qx 'BM-10: Exists' || fail "matching library left BM-10 Gap"
printf '%s\n' "${match_wire}" | grep -qx 'L2: Exists' || fail "matching library left L2 Gap"
if printf '%s\n' "${match_wire}" | grep -qx 'BM-01 mock-oracle 60 rows (not Latch 60/60 bit-match)'; then
  fail "matching provided library was labeled mock-oracle"
fi

echo "wire script with a mismatched provided library"
set +e
mismatch_wire="$(env -u LD_LIBRARY_PATH -u LATCH_LIB_DIR LATCH_LIBRARY="${probe}/mismatch/liblatch.so" \
  bash "${ROOT}/scripts/public_fixture_wire.sh" 2>&1)"
mismatch_wire_rc=$?
set -e
printf '%s\n' "${mismatch_wire}" | grep -E '^(BM-01|BM-09|BM-10|L2|oracle=)' || true
if [[ "${mismatch_wire_rc}" -eq 0 ]]; then
  fail "mismatched provided library did not fail the wire script"
fi
if printf '%s\n' "${mismatch_wire}" | grep -qx 'BM-01 Latch bit-match 60/60'; then
  fail "wire script printed Latch bit-match for a mismatched library"
fi
printf '%s\n' "${mismatch_wire}" | grep -qx 'BM-01 Latch bit-match: Gap' \
  || fail "mismatched library was not left Gap"
printf '%s\n' "${mismatch_wire}" | grep -qx 'BM-09: Gap' || fail "mismatched BM-09 was not Gap"
printf '%s\n' "${mismatch_wire}" | grep -qx 'BM-10: Gap' || fail "mismatched BM-10 was not Gap"
printf '%s\n' "${mismatch_wire}" | grep -qx 'L2: Gap' || fail "mismatched L2 was not Gap"

restore_mock_build
restored="$(env -u LD_LIBRARY_PATH -u LATCH_LIBRARY -u LATCH_LIB_DIR \
  LATCH_GOLDENS="${ROOT}/fixtures/latch-robot-1.jsonl" \
  "${ROOT}/build/mujoco/lbs_bm")"
printf '%s\n' "${restored}" | grep -qx 'BM-01 mock-oracle 60 rows (not Latch 60/60 bit-match)' \
  || fail "restored public build lost the mock recompute"
if printf '%s\n' "${restored}" | grep -qx 'BM-01 Latch bit-match 60/60'; then
  fail "restored public build printed Latch bit-match"
fi
printf '%s\n' "${restored}" | grep -qx 'oracle=mock' || fail "restored public build is not the mock oracle"

echo "public fixture wire tests passed"

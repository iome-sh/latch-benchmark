#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Public CI fixture wiring.
# Tracked *.jsonl files are the only export candidates.
# A 60-row file with an evidence field is a BM-01 golden candidate.
# The mock path may recompute the tracked 60-row file. It does not print a Latch bit-match.
# A provided library (LATCH_LIBRARY or LATCH_LIB_DIR) is linked and called.
# BM-01 Latch bit-match 60/60 is reported only when that run matches all 60 rows.
# With the library linked, lbs_verify and lbs_soak compare LATCH_GOLDENS and exit 0 only when both match all 60 rows.
# The in-tree fixture replay does not exit 0.
# BM-09, BM-10, and L2 are Exists only for that 60-row match.
# A mock recompute does not mark them Exists. No library leaves them Gap.
# This script does not write rows, hashes, or evidence.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MOCK_REL="harness/mujoco/fixtures/mock-oracle.jsonl.example"
golden_files=()
session_files=()

jsonl_data_rows() {
  awk 'NF && $0 !~ /^[[:space:]]*#/ { n++ } END { print n+0 }' "$1"
}

tracked_jsonl() {
  local root="$1"
  git -C "$root" ls-files -z -- '*.jsonl'
}

file_has_key() {
  local file="$1"
  local key="$2"
  grep -q "\"${key}\"" "$file"
}

is_golden_60() {
  local file="$1"
  local rows=0
  file_has_key "$file" "evidence" || return 1
  rows="$(jsonl_data_rows "$file")"
  [[ "$rows" -eq 60 ]]
}

is_session_fixture() {
  local file="$1"
  file_has_key "$file" "evidence" || return 1
  file_has_key "$file" "session" || return 1
  file_has_key "$file" "snapshot_id" || return 1
  return 0
}

# Prints one snapshot id. Exit 2 if missing. Exit 3 if the file mixes ids.
snapshot_id_of() {
  python3 - "$1" <<'PY'
import re, sys
text = open(sys.argv[1], encoding="utf-8").read()
ids = re.findall(r'"(?:snapshot_id|snapshot)"\s*:\s*"([^"]*)"', text)
ids = [item for item in ids if item]
if not ids:
    sys.exit(2)
if any(item != ids[0] for item in ids):
    sys.exit(3)
print(ids[0])
PY
}

# Echoes "BM-01 60/60 not claimed" unless the harness already settled the claim.
bm01_after_run() {
  local out="$1"
  if printf '%s\n' "$out" | grep -qx 'BM-01 Latch bit-match 60/60'; then
    return 0
  fi
  if printf '%s\n' "$out" | grep -qx 'BM-01 60/60 not claimed'; then
    return 0
  fi
  echo "BM-01 60/60 not claimed"
}

mock_row_count() {
  local path="${ROOT}/${MOCK_REL}"
  if [[ -f "$path" ]]; then
    jsonl_data_rows "$path"
    return 0
  fi
  echo 0
}

classify_tracked() {
  local root="$1"
  local rel=""
  local path=""
  golden_files=()
  session_files=()
  while IFS= read -r -d '' rel; do
    [[ -n "$rel" ]] || continue
    path="${root}/${rel}"
    [[ -f "$path" ]] || continue
    if is_golden_60 "$path"; then
      golden_files+=("$path")
    fi
    if is_session_fixture "$path"; then
      session_files+=("$path")
    fi
  done < <(tracked_jsonl "$root")
}

resolve_bin() {
  local build="$1"
  local name="$2"
  local p=""
  if [[ -x "${build}/${name}" ]]; then
    printf '%s\n' "${build}/${name}"
    return 0
  fi
  shopt -s nullglob
  for p in "${build}"/*/"${name}" "${build}"/*/"${name}.exe"; do
    if [[ -x "$p" ]]; then
      printf '%s\n' "$p"
      shopt -u nullglob
      return 0
    fi
  done
  shopt -u nullglob
  return 1
}

configure_harness() {
  local harness="${ROOT}/harness/mujoco"
  local build="${ROOT}/build/mujoco"
  local lib_dir="${LATCH_LIB_DIR:-}"
  local library="${LATCH_LIBRARY:-}"
  if [[ -n "${library}" && -f "${library}" ]]; then
    env -u LATCH_LIB_DIR cmake -S "${harness}" -B "${build}" \
      -DLATCH_LIBRARY="${library}" -ULATCH_LIB_DIR
  elif [[ -n "${lib_dir}" || -n "${library}" ]]; then
    local use_dir="${lib_dir}"
    if [[ -z "${use_dir}" ]]; then
      use_dir="${library}"
    fi
    env -u LATCH_LIBRARY cmake -S "${harness}" -B "${build}" \
      -DLATCH_LIB_DIR="${use_dir}" -ULATCH_LIBRARY
  else
    env -u LATCH_LIB_DIR -u LATCH_LIBRARY cmake -S "${harness}" -B "${build}" \
      -ULATCH_LIB_DIR -ULATCH_LIBRARY
  fi
}

run_bm_binary() {
  local goldens="$1"
  local build="${ROOT}/build/mujoco"
  local bin=""
  configure_harness
  cmake --build "${build}" --target lbs_bm
  bin="$(resolve_bin "${build}" lbs_bm)"
  LATCH_GOLDENS="${goldens}" "${bin}"
}

run_bm01() {
  local goldens="$1"
  local lib_dir="${LATCH_LIB_DIR:-}"
  local library="${LATCH_LIBRARY:-}"
  local out=""
  local rc=0
  if [[ -n "${lib_dir}" || -n "${library}" ]]; then
    set +e
    out="$(LATCH_GOLDENS="${goldens}" bash "${ROOT}/scripts/w2_optional_artifact.sh" 2>&1)"
    rc=$?
    set -e
  else
    set +e
    out="$(run_bm_binary "${goldens}" 2>&1)"
    rc=$?
    set -e
  fi
  printf '%s\n' "${out}"
  bm01_after_run "${out}"
  if printf '%s\n' "${out}" | grep -qx 'BM-01 Latch bit-match 60/60'; then
    set +e
    return "${rc}"
  fi
  if printf '%s\n' "${out}" | grep -qx 'BM-01 mock-oracle 60 rows (not Latch 60/60 bit-match)'; then
    echo "BM-01 mock-oracle recompute: Exists"
  else
    rc=1
  fi
  echo "BM-01 Latch bit-match: Gap"
  # set -e is global. A non-zero return would exit the caller before it records Gap.
  set +e
  return "${rc}"
}

run_session_binaries() {
  local session="$1"
  local snap="$2"
  local build="${ROOT}/build/mujoco"
  local verify=""
  local soak=""
  configure_harness
  cmake --build "${build}" --target lbs_verify --target lbs_soak
  local vout=""
  local sout=""
  local vrc=0
  local src=0
  verify="$(resolve_bin "${build}" lbs_verify)"
  soak="$(resolve_bin "${build}" lbs_soak)"
  set +e
  vout="$(env -u LATCH_GOLDENS LATCH_SESSION="${session}" LATCH_SNAPSHOT_ID="${snap}" "${verify}" 2>&1)"
  vrc=$?
  sout="$(env -u LATCH_GOLDENS LATCH_SESSION="${session}" LATCH_SNAPSHOT_ID="${snap}" "${soak}" 2>&1)"
  src=$?
  set -e
  printf '%s\n' "${vout}"
  printf '%s\n' "${sout}"
  [[ "${vrc}" -eq 0 && "${src}" -eq 0 ]]
}

run_session() {
  local session="$1"
  local snap=""
  local rc=0
  local out=""
  set +e
  snap="$(snapshot_id_of "${session}")"
  rc=$?
  set -e
  if [[ "${rc}" -ne 0 || -z "${snap}" ]]; then
    echo "BM-09 evidence replay: Gap (session fixture has no single snapshot_id; not inventing one)"
    echo "BM-10 two-process soak: Gap (same fixture, no single snapshot_id)"
    return 0
  fi
  set +e
  out="$(run_session_binaries "${session}" "${snap}" 2>&1)"
  rc=$?
  set -e
  printf '%s\n' "${out}"
  if [[ "${rc}" -ne 0 ]]; then
    echo "BM-09 evidence replay: Gap (verify path did not recompute)"
    echo "BM-10 two-process soak: Gap (second process did not match)"
    return 0
  fi
  echo "BM-09 replay ran: ${session}"
  echo "BM-10 two-process soak ran: ${session}"
  if [[ "${out}" == *"not a Latch"* || "${out}" == *"not claimed"* || "${out}" == *"not certified"* ]]; then
    echo "BM-09 evidence replay: Gap"
    echo "BM-10 two-process soak: Gap"
  fi
}

# Exists only for a provided library whose verify and soak both matched 60 rows.
# Mock text, a shorter match, or an unset library stays Gap.
report_linked_golden() {
  local out="$1"
  local linked=0
  if [[ -n "${LATCH_LIB_DIR:-}" || -n "${LATCH_LIBRARY:-}" ]]; then
    linked=1
  fi
  if [[ "${linked}" -eq 1 ]] \
    && printf '%s\n' "${out}" | grep -qx 'BM-09 Latch library evidence match 60 rows' \
    && printf '%s\n' "${out}" | grep -qx 'BM-10 Latch library two-process match 60 rows' \
    && [[ "${out}" != *"not a Latch"* && "${out}" != *"not claimed"* && "${out}" != *"not certified"* ]]; then
    echo "BM-09: Exists"
    echo "BM-10: Exists"
    echo "L2: Exists"
    return 0
  fi
  echo "BM-09: Gap"
  echo "BM-10: Gap"
  echo "L2: Gap"
}

try_recompute_golden() {
  local golden="$1"
  local snap=""
  local rc=0
  local out=""
  set +e
  snap="$(snapshot_id_of "${golden}")"
  rc=$?
  set -e
  if [[ "${rc}" -ne 0 || -z "${snap}" ]]; then
    echo "BM-09 evidence replay: Gap (tracked golden has no single snapshot; not inventing one)"
    echo "BM-10 two-process soak: Gap (same file, no single snapshot)"
    return 0
  fi
  set +e
  out="$(run_session_binaries "${golden}" "${snap}" 2>&1)"
  rc=$?
  set -e
  if [[ "${rc}" -ne 0 ]]; then
    echo "BM-09: Gap"
    echo "BM-10: Gap"
    echo "L2: Gap"
    set +e
    return 1
  fi
  printf '%s\n' "${out}"
  report_linked_golden "${out}"
}

gap_bm01_missing() {
  local rows=0
  rows="$(mock_row_count)"
  echo "BM-01 60-row golden: Gap (no tracked exported 60-row fixture; ${MOCK_REL} has ${rows} mock-oracle rows)"
}

main() {
  local out=""
  local rc=0
  local status=0
  classify_tracked "${ROOT}"
  if [[ "${#golden_files[@]}" -eq 0 ]]; then
    set +e
    out="$(bash "${ROOT}/scripts/w2_optional_artifact.sh" 2>&1)"
    rc=$?
    set -e
    printf '%s\n' "${out}"
    if ! printf '%s\n' "${out}" | grep -qx 'BM-01 Latch bit-match 60/60'; then
      if ! printf '%s\n' "${out}" | grep -qx 'BM-01 60/60 not claimed'; then
        echo "BM-01 60/60 not claimed"
      fi
      gap_bm01_missing
    fi
    if [[ "${rc}" -ne 0 ]]; then
      status="${rc}"
    fi
  elif [[ "${#golden_files[@]}" -gt 1 ]]; then
    echo "BM-01 60/60 not claimed"
    echo "BM-01 60-row golden: Gap (more than one tracked 60-row fixture)"
    status=1
  else
    set +e
    run_bm01 "${golden_files[0]}"
    rc=$?
    set -e
    if [[ "${rc}" -ne 0 ]]; then
      status="${rc}"
    fi
  fi

  if [[ "${#session_files[@]}" -eq 0 ]]; then
    if [[ "${#golden_files[@]}" -eq 1 ]]; then
      set +e
      try_recompute_golden "${golden_files[0]}"
      rc=$?
      set -e
      if [[ "${rc}" -ne 0 && "${status}" -eq 0 ]]; then
        status="${rc}"
      fi
    else
      echo "BM-09 evidence replay: Gap (no tracked exported session fixture the verify path can recompute)"
      echo "BM-10 two-process soak: Gap (no tracked exported session fixture the verify path can recompute)"
    fi
  elif [[ "${#session_files[@]}" -gt 1 ]]; then
    echo "BM-09 evidence replay: Gap (more than one tracked session fixture)"
    echo "BM-10 two-process soak: Gap (more than one tracked session fixture)"
    status=1
  else
    run_session "${session_files[0]}"
  fi
  return "${status}"
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi

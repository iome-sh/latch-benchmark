#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Optional BM-01 hook.
# Both library variables empty, or goldens empty: print and exit. Do not configure.
# A library path and LATCH_GOLDENS: configure harness/mujoco, build lbs_bm, and run it.
# Fail unless the binary prints the bit-match line. Do not fetch a repository.
# Do not claim 60/60 before that line.
set -euo pipefail

lib_dir="${LATCH_LIB_DIR:-}"
library="${LATCH_LIBRARY:-}"
goldens="${LATCH_GOLDENS:-}"

if [[ -z "${lib_dir}" && -z "${library}" ]]; then
  echo "BM-01 60/60 not claimed"
  exit 0
fi

if [[ -z "${goldens}" ]]; then
  echo "BM-01 private library set but goldens unset; 60/60 not claimed"
  exit 0
fi

script_dir="$(cd "$(dirname "$0")" && pwd)"
harness="$(cd "${script_dir}/../harness/mujoco" && pwd)"
build="${script_dir}/../build/mujoco"

# A stale cache entry must not keep the other variable.
if [[ -n "${library}" && -f "${library}" ]]; then
  env -u LATCH_LIB_DIR cmake -S "${harness}" -B "${build}" \
    -DLATCH_LIBRARY="${library}" -ULATCH_LIB_DIR
else
  use_dir="${lib_dir}"
  if [[ -z "${use_dir}" ]]; then
    use_dir="${library}"
  fi
  env -u LATCH_LIBRARY cmake -S "${harness}" -B "${build}" \
    -DLATCH_LIB_DIR="${use_dir}" -ULATCH_LIBRARY
fi

cmake --build "${build}" --target lbs_bm

bin=""
if [[ -x "${build}/lbs_bm" ]]; then
  bin="${build}/lbs_bm"
else
  shopt -s nullglob
  for p in "${build}"/*/"lbs_bm" "${build}"/*/"lbs_bm.exe"; do
    if [[ -x "${p}" ]]; then
      bin="${p}"
      break
    fi
  done
  shopt -u nullglob
fi
if [[ -z "${bin}" ]]; then
  echo "lbs_bm was not produced under build/mujoco" >&2
  exit 1
fi

export LATCH_GOLDENS="${goldens}"
set +e
out="$("${bin}" 2>&1)"
rc=$?
set -e
printf '%s\n' "${out}"
if [[ "${out}" != *"BM-01 Latch bit-match"* ]]; then
  echo "lbs_bm did not print BM-01 Latch bit-match" >&2
  exit 1
fi
if [[ "${rc}" -ne 0 ]]; then
  exit "${rc}"
fi

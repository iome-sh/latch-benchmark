#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Run the FA checklist.
# No arguments: MuJoCo harness binaries. That path does not start gz-sim.
# `partner`: the ROS 2 / gz one-joint launch. Mock-oracle.
# build/ros2_gz/lbs_node_wrap, when present, is mock-oracle and not a gz-sim launch.
set -euo pipefail
shopt -s nullglob

ROOT="$(cd "$(dirname "$0")/.." && pwd)"

if [[ "${1:-}" == "partner" ]]; then
  exec bash "${ROOT}/harness/ros2_gz/launch_partner.sh"
fi

BUILD="${ROOT}/build/mujoco"

if [[ ! -d "${BUILD}" ]]; then
  echo "refusing to run: build/mujoco is missing" >&2
  echo "cmake -S harness/mujoco -B build/mujoco"
  exit 1
fi

resolve_bin() {
  local name="$1"
  local p
  if [[ -f "${BUILD}/${name}" && -x "${BUILD}/${name}" ]]; then
    printf '%s\n' "${BUILD}/${name}"
    return 0
  fi
  for p in "${BUILD}"/*/"${name}"; do
    if [[ -f "${p}" && -x "${p}" ]]; then
      printf '%s\n' "${p}"
      return 0
    fi
  done
  return 1
}

status=0
bm_log=""
verify_log=""
soak_log=""
for name in lbs_bm lbs_verify lbs_soak lbs_noalloc; do
  if ! bin="$(resolve_bin "${name}")"; then
    echo "missing binary: ${name} under build/mujoco" >&2
    status=1
    continue
  fi
  echo "running ${name}: ${bin}"
  set +e
  log="$("${bin}" 2>&1)"
  rc=$?
  set -e
  printf '%s\n' "${log}"
  if [[ "${rc}" -ne 0 ]]; then
    echo "binary exited non-zero: ${name}" >&2
    status=1
  fi
  case "${name}" in
    lbs_bm) bm_log="${log}" ;;
    lbs_verify) verify_log="${log}" ;;
    lbs_soak) soak_log="${log}" ;;
  esac
done

wrap="${ROOT}/build/ros2_gz/lbs_node_wrap"
if [[ -f "${wrap}" ]]; then
  echo "running lbs_node_wrap: ${wrap}"
  if ! "${wrap}"; then
    echo "binary exited non-zero: lbs_node_wrap" >&2
    status=1
  fi
  echo "checklist lbs_node_wrap: mock-oracle, not a gz-sim launch"
else
  echo "node wrap binary was not built"
fi

if printf '%s\n' "${bm_log}" | grep -qx 'oracle=latch'; then
  if printf '%s\n' "${bm_log}" | grep -qx 'BM-01 Latch bit-match 60/60'; then
    echo "checklist lbs_bm: provided library printed BM-01 Latch bit-match 60/60"
  else
    echo "checklist lbs_bm: provided library linked; Latch bit-match not claimed"
  fi
  if printf '%s\n' "${verify_log}" | grep -q 'BM-09 Latch library evidence match'; then
    echo "checklist lbs_verify: provided library evidence match"
  else
    echo "checklist lbs_verify: provided library linked; evidence match not claimed"
  fi
  if printf '%s\n' "${soak_log}" | grep -q 'BM-10 Latch library two-process match'; then
    echo "checklist lbs_soak: provided library two-process match"
  else
    echo "checklist lbs_soak: provided library linked; two-process match not claimed"
  fi
fi
echo "checklist: MuJoCo path only (mock-oracle). This invocation does not start gz-sim."

exit "${status}"

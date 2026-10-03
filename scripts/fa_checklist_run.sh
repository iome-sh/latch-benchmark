#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Run the FA checklist.
# No arguments: MuJoCo harness binaries. That path does not start gz-sim.
# `partner`: the ROS 2 / gz one-joint launch. Mock-oracle, not a Latch certificate.
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
for name in lbs_bm lbs_verify lbs_soak lbs_noalloc; do
  if ! bin="$(resolve_bin "${name}")"; then
    echo "missing binary: ${name} under build/mujoco" >&2
    status=1
    continue
  fi
  echo "running ${name}: ${bin}"
  if ! "${bin}"; then
    echo "binary exited non-zero: ${name}" >&2
    status=1
  fi
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

echo "checklist lbs_bm: mock-oracle, not a Latch 60/60 certificate"
echo "checklist lbs_verify: mock-oracle, not a Latch 60/60 certificate"
echo "checklist lbs_soak: mock-oracle, not a Latch 60/60 certificate"
echo "checklist lbs_noalloc: mock-oracle, not a Latch 60/60 certificate"
echo "checklist: MuJoCo path only (mock-oracle). This invocation does not start gz-sim."

exit "${status}"

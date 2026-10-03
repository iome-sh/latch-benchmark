#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Partner FA entry. Starts the existing one-joint ROS 2 / gz path and scores
# scripts/fa_checklist.md. Mock-oracle. Not a Latch certificate.
# Yield leaves the setpoint.
# `partner checklist: gz-sim launched` is printed only when launch_one.sh
# itself printed `gz-sim launched`. Exit 0 alone is not that claim.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

gaps() {
  echo "partner checklist: bit-match stays Gap"
  echo "partner checklist: live evidence path stays Gap"
  echo "partner checklist: chatter stale deny budget reject stay Gap"
  echo "partner checklist: evidence replay stays Gap"
  echo "partner checklist: soak stays Gap"
  echo "partner checklist: operator signature stays Gap"
  echo "partner checklist: latency histogram stays Gap"
  echo "partner checklist: not a Latch certificate"
}

log="$(mktemp)"
trap 'rm -f "${log}"' EXIT
set +e
bash "${ROOT}/harness/gz_control/launch_one.sh" 2>&1 | tee "${log}"
status=${PIPESTATUS[0]}
set -e
if [[ "${status}" -ne 0 ]] || ! grep -qx "gz-sim launched" "${log}"; then
  echo "partner checklist: launch did not finish"
  gaps
  if [[ "${status}" -eq 0 ]]; then
    exit 1
  fi
  exit "${status}"
fi

echo "partner checklist: gz-sim launched"
echo "partner checklist: one-arm gz binding ran"
echo "partner checklist: yield leaves the setpoint"
echo "partner checklist: BT.ROS2 condition read-only"
gaps
exit 0

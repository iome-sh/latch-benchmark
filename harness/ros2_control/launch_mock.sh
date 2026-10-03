#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Start Controller Manager and a stock position controller on mock hardware.
# A mapper turns a mode name into a position command. This is not the partner
# Mock hardware only. This script does not start gz-sim.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
HERE="$(cd "$(dirname "$0")" && pwd)"
LOG="${TMPDIR:-/tmp}/latch-bench-ros2c-$$"
mkdir -p "${LOG}"

if [[ ! -f /opt/ros/jazzy/setup.bash ]]; then
  echo "ROS 2 Jazzy is not installed" >&2
  exit 1
fi
# shellcheck disable=SC1091
set +u
source /opt/ros/jazzy/setup.bash
set -u

export ROS_LOCALHOST_ONLY=1
export ROS_DOMAIN_ID="${ROS_DOMAIN_ID:-47}"
export PYTHONUNBUFFERED=1

cleanup() {
  local pid
  for pid in "${CM_PID:-}" "${RSP_PID:-}" "${MAP_PID:-}"; do
    if [[ -n "${pid}" ]] && kill -0 "${pid}" 2>/dev/null; then
      kill "${pid}" 2>/dev/null || true
      wait "${pid}" 2>/dev/null || true
    fi
  done
}
trap cleanup EXIT

python3 - "${HERE}/one_joint.urdf" "${LOG}/rsp.yaml" <<'PY'
import pathlib, sys
urdf = pathlib.Path(sys.argv[1]).read_text()
body = "\n".join(("      " + line) if line else "" for line in urdf.splitlines())
pathlib.Path(sys.argv[2]).write_text(
    "robot_state_publisher:\n  ros__parameters:\n    robot_description: |\n" + body + "\n"
)
PY

ros2 run controller_manager ros2_control_node --ros-args -p update_rate:=20 \
  >"${LOG}/cm.log" 2>&1 &
CM_PID=$!
ros2 run robot_state_publisher robot_state_publisher --ros-args \
  --params-file "${LOG}/rsp.yaml" >"${LOG}/rsp.log" 2>&1 &
RSP_PID=$!

sleep 2
if ! kill -0 "${CM_PID}" 2>/dev/null; then
  echo "controller_manager exited early" >&2
  cat "${LOG}/cm.log" >&2 || true
  exit 1
fi

timeout 40 ros2 run controller_manager spawner joint_state_broadcaster \
  --param-file "${HERE}/controllers.yaml" >"${LOG}/spawn_jsb.log" 2>&1
timeout 40 ros2 run controller_manager spawner forward_position_controller \
  --param-file "${HERE}/controllers.yaml" >"${LOG}/spawn_fwd.log" 2>&1

if ! ros2 control list_controllers | tee "${LOG}/controllers.txt" | grep -q "forward_position_controller.*active"; then
  echo "forward position controller is not active" >&2
  cat "${LOG}/controllers.txt" >&2 || true
  cat "${LOG}/cm.log" >&2 || true
  cat "${LOG}/spawn_fwd.log" >&2 || true
  exit 1
fi
echo "ros2_control forward position controller active"

python3 "${HERE}/mode_mapper.py" >"${LOG}/mapper.log" 2>&1 &
MAP_PID=$!
sleep 2

set +e
python3 "${HERE}/assert_motion.py" >"${LOG}/assert.log" 2>&1
status=$?
set -e
cat "${LOG}/assert.log"
if [[ "${status}" -ne 0 ]]; then
  echo "motion assert failed" >&2
  cat "${LOG}/mapper.log" >&2 || true
  cat "${LOG}/cm.log" >&2 || true
  exit "${status}"
fi
if ! grep -q "series stop tripped without reading the mode" "${LOG}/mapper.log"; then
  echo "series-stop line missing from mapper log" >&2
  cat "${LOG}/mapper.log" >&2 || true
  exit 1
fi
grep -q "yield leaves the command" "${LOG}/assert.log"
grep -q "this job does not start gz-sim" "${LOG}/assert.log"
echo "series stop tripped without reading the mode"

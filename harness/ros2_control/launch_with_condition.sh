#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Mock position controller and the read-only condition in one process group.
# gz-sim is not started. Latch is not linked.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
HERE="$(cd "$(dirname "$0")" && pwd)"
LOG="${TMPDIR:-/tmp}/latch-benchmark-together-$$"
mkdir -p "${LOG}"

if [[ ! -f /opt/ros/jazzy/setup.bash ]]; then
  echo "ROS 2 Jazzy is not installed" >&2
  exit 1
fi
# shellcheck disable=SC1091
set +u
source /opt/ros/jazzy/setup.bash
if [[ -n "${BT_SETUP:-}" ]]; then
  # shellcheck disable=SC1090
  source "${BT_SETUP}"
fi
if [[ -n "${OVERLAY_SETUP:-}" ]]; then
  # shellcheck disable=SC1090
  source "${OVERLAY_SETUP}"
fi
set -u

export ROS_LOCALHOST_ONLY=1
export ROS_DOMAIN_ID="${ROS_DOMAIN_ID:-51}"
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
  exit 1
fi

python3 "${HERE}/mode_mapper.py" >"${LOG}/mapper.log" 2>&1 &
MAP_PID=$!
sleep 2

ros2 run latch_bench_bt latch_bt_condition >"${LOG}/condition.log" 2>&1
cat "${LOG}/condition.log"
grep -q "BT.ROS2 condition read-only" "${LOG}/condition.log"

python3 - <<'PY'
import sys, time
import rclpy
from sensor_msgs.msg import JointState
from std_msgs.msg import UInt8

rclpy.init()
node = rclpy.create_node("together_probe")
node.position = None

def on_joint(msg):
    if msg.name and "joint1" in msg.name:
        node.position = float(msg.position[msg.name.index("joint1")])
    elif msg.position:
        node.position = float(msg.position[0])

node.create_subscription(JointState, "/joint_states", on_joint, 10)
pub = node.create_publisher(UInt8, "/latch/mode_name", 10)
approach = UInt8()
approach.data = 0
deadline = time.monotonic() + 5.0
while time.monotonic() < deadline:
    pub.publish(approach)
    rclpy.spin_once(node, timeout_sec=0.05)
    if node.position is not None and node.position >= 0.007:
        break
if node.position is None or node.position < 0.007:
    print(f"together: approach did not advance ({node.position})", file=sys.stderr)
    sys.exit(1)

msg = UInt8()
msg.data = 4
end = time.monotonic() + 0.8
while time.monotonic() < end:
    pub.publish(msg)
    rclpy.spin_once(node, timeout_sec=0.05)
held = node.position
end = time.monotonic() + 0.8
while time.monotonic() < end:
    pub.publish(msg)
    rclpy.spin_once(node, timeout_sec=0.05)
if held is None or node.position is None or abs(node.position - held) > 0.004:
    print(f"together: yield moved the joint ({held} -> {node.position})", file=sys.stderr)
    sys.exit(1)
print("yield leaves the command", flush=True)
node.destroy_node()
rclpy.shutdown()
PY

echo "controller and condition in one job"
echo "gz-sim not started"
echo "Latch is not linked"
echo "this job does not start gz-sim"

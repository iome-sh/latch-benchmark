#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Headless gz-sim, gz_ros2_control, the stock position controller, and the
# read-only condition in one job. The partner checklist calls this through
# harness/ros2_gz/launch_partner.sh. Latch is not linked. Yield leaves the
# setpoint. Not a Latch certificate.
set -euo pipefail
set -m

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
HERE="$(cd "$(dirname "$0")" && pwd)"
LOG="${TMPDIR:-/tmp}/latch-bench-gzctl-$$"
mkdir -p "${LOG}"

if [[ ! -f /opt/ros/jazzy/setup.bash ]]; then
  echo "gz-sim did not start"
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
export ROS_DOMAIN_ID="${ROS_DOMAIN_ID:-53}"
export PYTHONUNBUFFERED=1

cleanup() {
  local pid
  for pid in "${GZ_PID:-}" "${BRIDGE_PID:-}" "${RSP_PID:-}" "${MAP_PID:-}" "${DRIVE_PID:-}"; do
    if [[ -n "${pid}" ]] && kill -0 "${pid}" 2>/dev/null; then
      kill -- -"${pid}" 2>/dev/null || kill "${pid}" 2>/dev/null || true
    fi
  done
  sleep 0.5
  for pid in "${GZ_PID:-}" "${BRIDGE_PID:-}" "${RSP_PID:-}" "${MAP_PID:-}" "${DRIVE_PID:-}"; do
    if [[ -n "${pid}" ]] && kill -0 "${pid}" 2>/dev/null; then
      kill -9 -- -"${pid}" 2>/dev/null || kill -9 "${pid}" 2>/dev/null || true
    fi
  done
}
trap cleanup EXIT

PARAMS="${HERE}/controllers.yaml"
python3 - "${HERE}/one_joint.urdf.in" "${LOG}/one_joint.urdf" "${PARAMS}" "${LOG}/rsp.yaml" <<'PY'
import pathlib, sys
src, dst, params, rsp = sys.argv[1:]
urdf = pathlib.Path(src).read_text().replace("@PARAMS@", str(pathlib.Path(params).resolve()))
pathlib.Path(dst).write_text(urdf)
body = "\n".join(("      " + line) if line else "" for line in urdf.splitlines())
pathlib.Path(rsp).write_text(
    "robot_state_publisher:\n  ros__parameters:\n    use_sim_time: false\n    robot_description: |\n"
    + body
    + "\n"
)
PY

echo "ros2 launch ${ROOT}/harness/ros2_gz/launch/partner_fa.launch.py"
ros2 launch "${ROOT}/harness/ros2_gz/launch/partner_fa.launch.py" \
  gz_args:="-s -r -v 1 empty.sdf" \
  rsp_params:="${LOG}/rsp.yaml" \
  >"${LOG}/gz.log" 2>&1 &
GZ_PID=$!
sleep 3
if ! kill -0 "${GZ_PID}" 2>/dev/null; then
  echo "gz-sim did not start"
  echo "gz sim exited early" >&2
  cat "${LOG}/gz.log" >&2 || true
  exit 1
fi
echo "gz-sim launched"
sleep 2

spawned=0
for _ in 1 2 3 4 5 6; do
  if timeout 20 ros2 run ros_gz_sim create -topic robot_description -name latch_bench_one -allow_renaming true \
      >"${LOG}/create.log" 2>&1; then
    spawned=1
    break
  fi
  sleep 2
done
if [[ "${spawned}" -ne 1 ]]; then
  echo "model create failed" >&2
  cat "${LOG}/create.log" >&2 || true
  cat "${LOG}/gz.log" >&2 || true
  exit 1
fi

if ! timeout 45 ros2 run controller_manager spawner joint_state_broadcaster \
    --param-file "${PARAMS}" >"${LOG}/spawn_jsb.log" 2>&1; then
  echo "joint_state_broadcaster spawner failed" >&2
  cat "${LOG}/spawn_jsb.log" >&2 || true
  exit 1
fi
if ! timeout 45 ros2 run controller_manager spawner forward_position_controller \
    --param-file "${PARAMS}" >"${LOG}/spawn_fwd.log" 2>&1; then
  echo "forward_position_controller spawner failed" >&2
  cat "${LOG}/spawn_fwd.log" >&2 || true
  exit 1
fi
if ! ros2 control list_controllers | tee "${LOG}/controllers.txt" | grep -q "forward_position_controller.*active"; then
  echo "forward position controller is not active" >&2
  cat "${LOG}/controllers.txt" >&2 || true
  exit 1
fi
echo "gz_ros2_control one joint active"

export LATCH_BENCH_STEP=0.25
python3 "${ROOT}/harness/ros2_control/mode_mapper.py" >"${LOG}/mapper.log" 2>&1 &
MAP_PID=$!
sleep 2

ros2 run latch_bench_bt latch_bt_condition >"${LOG}/condition.log" 2>&1
cat "${LOG}/condition.log"
grep -q "BT.ROS2 condition read-only" "${LOG}/condition.log"

BASELINE="$(python3 - <<'PY'
import time
import rclpy
from sensor_msgs.msg import JointState
rclpy.init()
node = rclpy.create_node("gz_baseline")
node.position = None
def on_joint(msg):
    if msg.name and "joint1" in msg.name:
        node.position = float(msg.position[msg.name.index("joint1")])
    elif msg.position:
        node.position = float(msg.position[0])
node.create_subscription(JointState, "/joint_states", on_joint, 10)
end = time.monotonic() + 3.0
while node.position is None and time.monotonic() < end:
    rclpy.spin_once(node, timeout_sec=0.1)
print("0" if node.position is None else node.position)
node.destroy_node()
rclpy.shutdown()
PY
)"

ros2 run latch_bench_mock_elect latch_mock_elect --drive-sim >"${LOG}/drive.log" 2>&1 &
DRIVE_PID=$!

set +e
python3 - "${BASELINE}" <<'PY'
import sys, time
import rclpy
from sensor_msgs.msg import JointState
baseline = float(sys.argv[1])
rclpy.init()
node = rclpy.create_node("gz_probe")
node.position = None
def on_joint(msg):
    if msg.name and "joint1" in msg.name:
        node.position = float(msg.position[msg.name.index("joint1")])
    elif msg.position:
        node.position = float(msg.position[0])
node.create_subscription(JointState, "/joint_states", on_joint, 10)
deadline = time.monotonic() + 8.0
while time.monotonic() < deadline:
    rclpy.spin_once(node, timeout_sec=0.05)
    if node.position is not None and node.position >= baseline + 0.05:
        break
if node.position is None or node.position < baseline + 0.05:
    print(f"gz launch: elected approach did not advance ({baseline} -> {node.position})", file=sys.stderr)
    sys.exit(1)
print("approach mode moves the sim joint", flush=True)
node.destroy_node()
rclpy.shutdown()
PY
move_status=$?
set -e
if [[ "${move_status}" -ne 0 ]]; then
  echo "----- drive log -----" >&2
  cat "${LOG}/drive.log" >&2 || true
fi
# Approach has already moved the joint. Publish the series stop while the
# elector is still sending approach, and require the joint to stay put.
stop_status=0
if [[ "${move_status}" -eq 0 ]]; then
  set +e
  python3 - <<'PY'
import sys, time
import rclpy
from sensor_msgs.msg import JointState
from std_msgs.msg import Bool
rclpy.init()
node = rclpy.create_node("gz_series_stop")
node.position = None
def on_joint(msg):
    if msg.name and "joint1" in msg.name:
        node.position = float(msg.position[msg.name.index("joint1")])
    elif msg.position:
        node.position = float(msg.position[0])
node.create_subscription(JointState, "/joint_states", on_joint, 10)
pub = node.create_publisher(Bool, "/emergency_stop", 10)
# Latch the stop before sampling. One in-flight approach step is 0.25 and
# may still land; the sample below is taken after that window.
end = time.monotonic() + 0.8
while time.monotonic() < end:
    flag = Bool()
    flag.data = True
    pub.publish(flag)
    rclpy.spin_once(node, timeout_sec=0.05)
    time.sleep(0.05)
end = time.monotonic() + 2.0
while node.position is None and time.monotonic() < end:
    flag = Bool()
    flag.data = True
    pub.publish(flag)
    rclpy.spin_once(node, timeout_sec=0.1)
if node.position is None:
    print("gz launch: series stop did not hold the joint", file=sys.stderr)
    sys.exit(1)
held = node.position
end = time.monotonic() + 1.5
while time.monotonic() < end:
    flag = Bool()
    flag.data = True
    pub.publish(flag)
    rclpy.spin_once(node, timeout_sec=0.05)
    time.sleep(0.1)
    if node.position is not None and abs(node.position - held) > 0.05:
        print(
            f"gz launch: series stop did not hold the joint ({held} -> {node.position})",
            file=sys.stderr,
        )
        sys.exit(1)
print("series stop holds the sim joint", flush=True)
node.destroy_node()
rclpy.shutdown()
PY
  stop_status=$?
  set -e
fi
drive_status=0
if ! wait "${DRIVE_PID}"; then
  echo "drive-sim elector failed" >&2
  cat "${LOG}/drive.log" >&2 || true
  drive_status=1
fi
DRIVE_PID=""
grep -q "contact dwell keeps approach" "${LOG}/drive.log"
grep -q "dwell expired elects grasp" "${LOG}/drive.log"
grep -q "drive-sim approach, dwell, grasp, then yield" "${LOG}/drive.log"
if [[ "${move_status}" -eq 0 && "${stop_status}" -eq 0 ]]; then
  grep -q "series stop tripped without reading the mode" "${LOG}/mapper.log"
fi
set +e
python3 - <<'PY'
import sys, time
import rclpy
from sensor_msgs.msg import JointState
rclpy.init()
node = rclpy.create_node("gz_hold")
node.position = None
def on_joint(msg):
    if msg.name and "joint1" in msg.name:
        node.position = float(msg.position[msg.name.index("joint1")])
    elif msg.position:
        node.position = float(msg.position[0])
node.create_subscription(JointState, "/joint_states", on_joint, 10)
end = time.monotonic() + 5.0
while node.position is None and time.monotonic() < end:
    rclpy.spin_once(node, timeout_sec=0.1)
if node.position is None:
    print("gz launch: no joint sample after yield", file=sys.stderr)
    sys.exit(1)
held = node.position
end = time.monotonic() + 0.8
while time.monotonic() < end:
    rclpy.spin_once(node, timeout_sec=0.05)
if node.position is None or abs(node.position - held) > 0.02:
    print(f"gz launch: yield moved the joint ({held} -> {node.position})", file=sys.stderr)
    sys.exit(1)
print("yield leaves the command", flush=True)
node.destroy_node()
rclpy.shutdown()
PY
hold_status=$?
set -e
status=0
if [[ "${move_status}" -ne 0 || "${stop_status}" -ne 0 || "${drive_status}" -ne 0 || "${hold_status}" -ne 0 ]]; then
  status=1
fi
if [[ "${status}" -ne 0 ]]; then
  echo "----- topics -----" >&2
  ros2 topic list >&2 || true
  echo "----- controllers -----" >&2
  ros2 control list_controllers >&2 || true
  echo "----- gz log -----" >&2
  tail -n 60 "${LOG}/gz.log" >&2 || true
  echo "----- create log -----" >&2
  tail -n 40 "${LOG}/create.log" >&2 || true
  exit "${status}"
fi

echo "read-only condition ran in the same job"
echo "Latch is not linked"
echo "not a Latch certificate"

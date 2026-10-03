# ROS 2 + gz FA path (L3 Must — primary partner FA)

**Status:** `lbs_node_wrap` Passed in CI as mock-oracle. A gz-sim server launch ran headless for a fixed iteration count (`launch_server.sh`, `worlds/empty.sdf`). The world has no Latch plugin and does not elect a mode. A separate CI workflow starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command without reading the mode. A separate CI workflow clones BehaviorTree.ROS2 at 72a3bf51dad332c67b99fc3373ccd5242f94f680 and ticks a read-only condition on /latch/mode_name. The condition does not elect and does not publish. Another workflow starts that mock controller and that condition in one job. A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs that condition in the same job. Approach on /latch/mode_name moves the sim joint through the position controller. Yield leaves that command. The one-joint gz job runs that mock-oracle process with --drive-sim. Approach moves the sim joint. E-stop elects yield and leaves the command. It is not Latch. Latch is not linked. The empty world does not elect a mode. The partner checklist runs that one-joint path. `scripts/fa_checklist_run.sh partner` starts `harness/ros2_gz/launch_partner.sh`, which starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The log prints `gz-sim launched` or `gz-sim did not start`. Yield leaves the setpoint. The partner launch **Exists** as mock-oracle. It is not a Latch certificate. Bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram stay **Gap** on that run. No Latch `.so` is committed.  
**Plan:** [`../../docs/INTEGRATION.md`](../../docs/INTEGRATION.md) · checklist [`../../scripts/fa_checklist.md`](../../scripts/fa_checklist.md)

## Server launch

`launch_server.sh` runs `gz sim -s -r -v 4 --iterations 5` on `worlds/empty.sdf`.
That world is physics-plugin only. It has no robot and no Latch plugin, and
it does not elect a mode. If the `gz` binary is missing the script exits 2
and prints `gz-sim binary absent; launch not run`. This server script does not
start ros2_control or the BT condition. The partner run is `launch_partner.sh`.

## Partner launch

`launch_partner.sh` is the partner command. `scripts/fa_checklist_run.sh partner` calls it.
The script starts `launch/partner_fa.launch.py` (headless gz-sim, the clock bridge, and `robot_state_publisher`) and then the existing one-joint sequence in `harness/gz_control/launch_one.sh`: spawn, forward position controller, mode mapper, read-only condition, and `latch_mock_elect --drive-sim`.
Approach moves the sim joint. Yield leaves the setpoint. The log prints `gz-sim launched` or `gz-sim did not start`.
This is mock-oracle. It is not a Latch certificate. Latch is not linked.
Bit-match, the live evidence path, chatter, stale, deny, budget, reject, evidence replay, soak, the operator signature, and the latency histogram stay **Gap** on that run.

## Stack

ROS 2 + gz-sim + gz_ros2_control + BT.ROS2 condition.

A **node** wraps Latch. A gz physics plugin does not own election. CI truth
for bit-match, chatter, latency, and no-alloc stays the MuJoCo harness
(`harness/mujoco`, L1 Must). Isaac is Stretch and is not this path.

## Wrap

- Subscribe joint state, contact/wrench, and sense age.
- `latch_consider` at ~20 Hz (inside the 10–50 Hz band).
- Publish `/latch/mode_record` (name + quality fields only).
- `latch_apply_reject` before the controller consumes the record.
- Existing `ros2_control` position / impedance controller maps the six names
  to position setpoints. Controller Manager keeps lifecycle. Latch does not
  activate or deactivate controllers.
- Yield / hold = hold last command (zero Δ).
- Independent `/emergency_stop` (or a gz plug) zeros effort **outside** the
  Latch plane (BM-07).
- BT.ROS2 **Condition** reads the mode name and optional plane. Read-only
  (BM-11).

`lbs_node_wrap` shows apply-reject turning a gone target into hold before the plant step, and that step does not move the joint. It is the mock oracle and is not the partner launch. The empty world still does not elect. `lbs_node_wrap` also shows grasp contact lost becoming hold before the plant step.

## Coverage

FA launch runs BM-02..08 and BM-11 live, plus the BT reader (checklist item
6). BM-01 / BM-08 CI numbers and the ASAN no-alloc assert stay on MuJoCo.
BM-09/10 go through the L2 verify CLI and soak (this launch may feed them).

## package.xml

Stub present for colcon discovery later. No Latch `.so` in-tree. Do not vendor
`liblatch*` or Latch source; link later via an optional external library,
never vendored in this tree.

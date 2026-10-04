# ROS 2 and gz partner path

`lbs_node_wrap` passed in CI as the mock oracle. The server launch and the one-joint partner launch are described below. Latch is not linked. The partner launch Exists as the mock oracle. It is not a Latch certificate. The checklist rows that launch does not run are Gap. No Latch `.so` is committed.  
How the harnesses fit together is in [`../../docs/INTEGRATION.md`](../../docs/INTEGRATION.md). The checklist is [`../../scripts/fa_checklist.md`](../../scripts/fa_checklist.md).

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
Bit-match, the live evidence path, chatter, stale, deny, budget, reject, evidence replay, soak, the operator signature, and the latency histogram stay Gap on that run.

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
- Yield and hold leave the last command in place. They do not add motion. Yield means the mode leaves the setpoint. It is not a protective stop and not a rated stop.
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

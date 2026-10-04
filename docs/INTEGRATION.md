# Which simulator each metric uses

Status as of 2026-10-03. L0 Exists. The L1 mock oracle Exists. The mock recompute of `fixtures/latch-robot-1.jsonl` Exists. The Latch bit-match is Gap. BM-09, BM-10, and L2 are Gap. `lbs_node_wrap` passed.

A gz-sim server launch has run headless for a fixed number of iterations. That world has no Latch plugin. A separate CI job starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command and does not read the mode. Another job clones BehaviorTree.ROS2 at `72a3bf51dad332c67b99fc3373ccd5242f94f680` and ticks a read-only condition on `/latch/mode_name`. The condition does not elect a mode and does not publish. A later job runs that controller and that condition together.

The partner launch Exists. It starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. Approach moves the sim joint. Yield leaves the setpoint. Latch is not linked. The evidence of earlier one-joint Actions runs is not in this repository. This launch is not a Latch certificate. On that run, bit-match, live evidence, chatter, stale sense, deny, budget, reject, evidence replay, soak, the operator signature, and the latency histogram stay Gap. No Latch `.so` is committed. The dated table is in [`ROADMAP.md`](ROADMAP.md).

These notes describe the validation path. They are not a claim that the product ships inside Gazebo, Isaac, or MuJoCo.

This page locks which simulators LBS may use, how they wrap the Latch C ABI,
and which benchmark each harness is allowed to speak for. Metric pass rules
stay in [`SPEC.md`](SPEC.md). Phase order stays in [`ROADMAP.md`](ROADMAP.md).
Banned KPIs stay in [`NONGOALS.md`](NONGOALS.md).

---

## The three simulator paths

| Rank | Stack | Role | Phase | Gate |
|------|-------|------|-------|------|
| 1 | **MuJoCo C harness** (`harness/mujoco`) | Primary CI truth: bit-match, chatter, latency, no-alloc | **L1 Must** | Yes |
| 2 | **ROS 2 + gz-sim + gz_ros2_control + BT.ROS2 condition** (`harness/ros2_gz`) | Primary partner FA | **L3 Must** | Yes |
| 3 | **Isaac** | Optional internal only | **Stretch** (BM-S8) | No |

MuJoCo is the merge-adjacent number. The ROS 2 / gz path is the partner FA
run. Isaac does not block L1, L3, or first partner open.

---

## Wrap pattern

Every path wraps the same C ABI. The plant never elects.

1. Sense stubs call `latch_consider` at **10–50 Hz**.
2. The mode record carries the evidence hex (evidence v2).
3. If a precondition dies after consider, call `latch_apply_reject` **before**
   the controller consumes the name (hold, plane `reject`).
4. The elected **name** maps onto the existing controller’s behavior table
   (approach / grasp / release / retract / yield / hold) as **position
   setpoints only**. Latch does not emit torque or trajectory.
5. The plant loop (~1 kHz) never calls Latch mid-tick.
6. The PLC / e-stop series stub stays independent of the Latch plane.

### MuJoCo C harness (L1 Must)

Single process, two rates. `mj_step` runs at ~1 kHz. Every N steps the harness
calls `latch_consider`, stores the mode, and the control callback applies the
name selects a position setpoint (`qpos` or `ctrl` targets, with no torque path). Sense comes
from `mjData` contacts and forces. `latch_apply_reject` runs before that
setpoint is consumed.

Apache-2.0 MuJoCo is the CI choice: headless, no DDS jitter, ASAN-friendly.
mjx is research-only and is not a V1 gate. Sources:
[`../harness/mujoco/README.md`](../harness/mujoco/README.md).

### ROS 2 + gz-sim + gz_ros2_control + BT.ROS2 (L3 Must)

A **node** wraps Latch. A physics plugin does not own election.

- Subscribe joint state, contact/wrench, and sense age.
- `latch_consider` at ~20 Hz; publish `/latch/mode_record`.
- `latch_apply_reject` before the controller consumes the record.
- The existing `ros2_control` position / impedance controller stays active.
  Controller Manager keeps lifecycle. Latch does not activate or deactivate
  controllers. MoveIt stays the planner; Latch may gate the execution phase
  by name only.
- Yield / hold means hold last command (zero Δ), not a stop category.
- An independent `/emergency_stop` (or a gz plug) zeros effort **outside**
  the Latch plane.
- A BT.ROS2 **Condition** reads the mode name (and optional plane) from the
  blackboard. The tree does not elect.

Partner FA is this launch plus [`../scripts/fa_checklist.md`](../scripts/fa_checklist.md).
A gz-sim server launch has run headless for a fixed number of iterations (`launch_server.sh` on `worlds/empty.sdf`). That world has no Latch plugin and does not elect a mode. A separate CI job starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command and does not read the mode. Another job clones BehaviorTree.ROS2 at `72a3bf51dad332c67b99fc3373ccd5242f94f680` and ticks a read-only condition on `/latch/mode_name`. The condition does not elect a mode and does not publish. A later job runs that controller and that condition together. A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs the condition in the same job. Approach on `/latch/mode_name` moves the sim joint. Yield leaves that command. The partner launch Exists. It starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The evidence of earlier one-joint Actions runs is not in this repository. Latch is not linked. This launch is not a Latch certificate. On that run, bit-match, live evidence, chatter, stale sense, deny, budget, reject, evidence replay, soak, the operator signature, and the latency histogram stay Gap. The mock recompute of `fixtures/latch-robot-1.jsonl` Exists. The Latch bit-match is Gap. BM-09, BM-10, and L2 are Gap. `lbs_node_wrap` passed as the mock oracle and is not that partner launch. Notes: [`../harness/ros2_gz/README.md`](../harness/ros2_gz/README.md).

### Isaac

Internal clip only: a camera clip, or a sensor-rich clip, of yield leaving the setpoint, when a partner
is already on Omniverse. Do not put Latch inside a GPU policy. Do not
block any Must exit on Isaac. Internal R&D use is the bound; packaging a
Kit-bundled turnkey needs a separate NVIDIA license check. BM-S8 in
[`SPEC.md`](SPEC.md).

---

## Which harness covers each metric

| ID | MuJoCo C (L1 CI truth) | ROS 2 + gz + BT (L3 partner FA) |
|----|------------------------|----------------------------------|
| **BM-01** | Headless goldens runner. The mock recompute of `fixtures/latch-robot-1.jsonl` Exists. The Latch bit-match is Gap. A provided library is called when `LATCH_LIBRARY` or `LATCH_LIB_DIR` is set. The bit-match line is printed only when that run matches. | Not the bit-match gate |
| **BM-02** | Chatter fixture on contact/wrench noise | FA chatter demo |
| **BM-03** | Stale fixture. When `sense_age` is at least 4, the mode is yield on plane `l2`. | Partner demo of stale sense |
| **BM-04** | Deny fixture (e-stop / collision / joint limit) | FA deny demo |
| **BM-05** | Budget overrun; plant tick not stalled | FA budget demo |
| **BM-06** | `latch_apply_reject` after consider | Same call before controller consume |
| **BM-07** | Yield leaves `qpos` target; independent stop inject stub | Yield leaves setpoint; `/emergency_stop` still trips |
| **BM-08** | p50/p99/max `latch_consider` histogram (the CI number) | Second measurement; DDS jitter is outside consider |
| **BM-09** | Gap on public CI. With a library linked, `lbs_verify` compares `LATCH_GOLDENS` and exits 0 only on a 60-row match. The in-tree fixture replay does not exit 0. It is Exists only for that match. A mock recompute does not mark it Exists. | Gap |
| **BM-10** | Gap on public CI. With a library linked, `lbs_soak` compares `LATCH_GOLDENS` and exits 0 only on a 60-row match. The in-tree fixture replay does not exit 0. It is Exists only for that match. A mock recompute does not mark it Exists. | Gap |
| **BM-11** | Adapter asserts mode-record fields only | BT.ROS2 Condition is read-only; no activate/deactivate API |
| **BM-12** | Illegal emission = 0 under soak; ASAN no-alloc on the tick path | Held if the FA soak runs |
| **BM-S8** | — | — Isaac internal clip only |

BM-09 is Gap. BM-10 is Gap. L2 is Gap. Pass rules are in [`SPEC.md`](SPEC.md).

---

## What this suite does not claim

Say these in a partner review. They are checks, not extra product.

- This is a validation path. The product is the Latch election ABI, not a simulator.
- Yield means the mode leaves the setpoint. It is not a protective stop and not a rated stop. The separate stop still has to trip after yield (BM-07). A monitored standstill in ISO 10218 is a different function. Latch is not that function.
- There is no torque and no trajectory. Adapter tests check mode-record fields only (BM-11).
- This is not a behavior tree. The BehaviorTree.ROS2 kit is a read-only condition.
- This is not Controller Manager. Latch does not activate or deactivate controllers.
- Consider latency is a measured histogram (BM-08). It is not a certified worst-case execution time.
- Isaac is an internal stretch clip. It is not a merge gate, and it is not a claim that the product ships there.
- Grasp success, pick success, ManiSkill-class scores, and SIL, PLd, or stop distance are not Latch scores. See [`NONGOALS.md`](NONGOALS.md).

---

## No Latch `.so` in-tree

Latch core stays All Rights Reserved in its own tree (ABI headers, native
oracle, 60-row goldens). This repo **consumes** that ABI.

- Do not commit Latch source, `liblatch*`, or a Latch `.so`.
- Link only through an optional external library, never vendored in this tree.
- Do not publish Latch onto a public Actions cache.
- MuJoCo and gz-sim are used under their own Apache-2.0 licenses. They are
  not vendored in this tree today.
- Harness code that links Latch stays in this repository (or a partner
  share under NDA). ABI and goldens stay in the Latch tree.

---

## Not V1 gates

| Stack | Disposition |
|-------|-------------|
| mjx | Research only; GPU path is a poor private-`.so` CI gate |
| PyBullet | Smoke only if MuJoCo is blocked; position-setpoint adapter required |
| CoppeliaSim | Deprioritized (commercial license friction for CI) |
| Webots | Spare OSS path if gz is unavailable; not the partner FA |
| ManiSkill / robosuite / MetaWorld / RLBench | Wrong KPI class — do not host validation here |
| libfranka / UR RTDE / Doosan | Hardware-in-loop after sim FA is green; not this fold |

# INTEGRATION — Top sim paths for LBS

**Status (2026-10-03, tip `beef75f8ced96163c44aaca18fd9038352a96da1`):** L0 **Exists**. L1 mock-oracle **Exists**. L2 verify and soak stay **Gap**. BM-09 and BM-10 stay **Gap**. The 60-row golden on public CI stays **Gap**. The optional BM-01 step does not claim the 60-row golden and skips configure. `lbs_node_wrap` Passed. A gz-sim server launch ran headless for a fixed iteration count. The world has no Latch plugin. A separate CI workflow starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command without reading the mode. A separate CI workflow clones BehaviorTree.ROS2 at 72a3bf51dad332c67b99fc3373ccd5242f94f680 and ticks a read-only condition on /latch/mode_name. The condition does not elect and does not publish. Another workflow starts that mock controller and that condition in one job. A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs that condition in the same job. Approach on /latch/mode_name moves the sim joint through the position controller. Yield leaves that command. The partner launch **Exists** on this tip. It starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The pull-request one-joint job 111175592122 (Actions run 37113438572) on head `2117880edc7f69367083ff607267c1bb777df9bb` and the post-merge one-joint job 111195532468 (Actions run 37120511712) on `2fba664eb8dccd1e735ddd6f7fd9d49386d34b52` both succeeded and printed `gz-sim launched` via `harness/ros2_gz/launch/partner_fa.launch.py`. Approach moves the sim joint. Yield leaves the setpoint. Latch is not linked. This is not a Latch certificate. Bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram stay **Gap** on that run. The 60-row golden on public CI stays **Gap**. BM-09 and BM-10 stay **Gap**. No Latch `.so` is committed. **Gap:** the 60-row golden through the bench harness (external library and goldens are not present on public CI). Remaining partner-checklist rows stay **Gap**. Dated table: [`ROADMAP.md`](ROADMAP.md).  
**Speech:** validation path only — never “we ship in Gazebo / Isaac / MuJoCo.”

This page locks which simulators LBS may use, how they wrap the Latch C ABI,
and which benchmark each harness is allowed to speak for. Metric pass rules
stay in [`SPEC.md`](SPEC.md). Phase order stays in [`ROADMAP.md`](ROADMAP.md).
Banned KPIs stay in [`NONGOALS.md`](NONGOALS.md).

---

## Locked top matches

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
name→setpoint table (`qpos` / `ctrl` targets — no torque path). Sense comes
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
A gz-sim server launch ran headless for a fixed iteration count (`launch_server.sh` on `worlds/empty.sdf`). The world has no Latch plugin and does not elect a mode. A separate CI workflow starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command without reading the mode. A separate CI workflow clones BehaviorTree.ROS2 at 72a3bf51dad332c67b99fc3373ccd5242f94f680 and ticks a read-only condition on /latch/mode_name. The condition does not elect and does not publish. Another workflow starts that mock controller and that condition in one job. A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs that condition in the same job. Approach on /latch/mode_name moves the sim joint through the position controller. Yield leaves that command. The partner launch **Exists** on this tip. It starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The pull-request one-joint job 111175592122 (Actions run 37113438572) on head `2117880edc7f69367083ff607267c1bb777df9bb` and the post-merge one-joint job 111195532468 (Actions run 37120511712) on `2fba664eb8dccd1e735ddd6f7fd9d49386d34b52` both succeeded and printed `gz-sim launched` via `harness/ros2_gz/launch/partner_fa.launch.py`. Approach moves the sim joint. Yield leaves the setpoint. Latch is not linked. This is not a Latch certificate. Bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram stay **Gap** on that run. The 60-row golden on public CI stays **Gap**. BM-09 and BM-10 stay **Gap**. `lbs_node_wrap` Passed as mock-oracle and is not that partner launch. Notes: [`../harness/ros2_gz/README.md`](../harness/ros2_gz/README.md).

### Isaac (Stretch — not a gate)

Internal clip only: cinematic or sensor-rich yield≠stop video when a partner
is already on Omniverse. Do not put Latch inside a GPU policy. Do not
block any Must exit on Isaac. Internal R&D use is the bound; packaging a
Kit-bundled turnkey needs a separate NVIDIA license check. BM-S8 in
[`SPEC.md`](SPEC.md).

---

## BM → harness map

| ID | MuJoCo C (L1 CI truth) | ROS 2 + gz + BT (L3 partner FA) |
|----|------------------------|----------------------------------|
| **BM-01** | Headless goldens runner vs exported oracle. The 60-row golden on public CI stays **Gap**. | Not the bit-match gate |
| **BM-02** | Chatter fixture on contact/wrench noise | FA chatter demo |
| **BM-03** | Stale fixture (`sense_age ≥ 4` → yield, plane `l2`) | FA stale demo |
| **BM-04** | Deny fixture (e-stop / collision / joint limit) | FA deny demo |
| **BM-05** | Budget overrun; plant tick not stalled | FA budget demo |
| **BM-06** | `latch_apply_reject` after consider | Same call before controller consume |
| **BM-07** | Yield leaves `qpos` target; independent stop inject stub | Yield leaves setpoint; `/emergency_stop` still trips |
| **BM-08** | p50/p99/max `latch_consider` histogram (the CI number) | Second measurement; DDS jitter is outside consider |
| **BM-09** | **Gap** | **Gap** |
| **BM-10** | **Gap** | **Gap** |
| **BM-11** | Adapter asserts mode-record fields only | BT.ROS2 Condition is read-only; no activate/deactivate API |
| **BM-12** | Illegal emission = 0 under soak; ASAN no-alloc on the tick path | Held if the FA soak runs |
| **BM-S8** | — | — Isaac internal clip only |

BM-09 and BM-10 stay **Gap** on public CI. This is not a Latch certificate. Pass rules: [`SPEC.md`](SPEC.md).

---

## Honesty non-claims

Copy these into FA speech. They are pass/fail honesty, not extra product.

- Validation path only. The product is the Latch election ABI, not a simulator.
- Yield is leave-setpoint. The PLC / e-stop path stays in series and must
  still trip after yield (BM-07). Glossary contrast with ISO 10218 monitored
  standstill is naming only — Latch is not that function.
- No torque and no trajectory. Adapter tests assert mode-record fields only
  (BM-11).
- Not a BehaviorTree. The BT.ROS2 kit is a read-only Condition.
- Not Controller Manager. Latch does not activate or deactivate controllers.
- Consider latency is a measured histogram (BM-08), not a certified WCET.
- Isaac is an internal Stretch clip, not a merge gate and not a ship claim.
- Miss KPIs stay banned: grasp / pick success, ManiSkill-class scores, SIL /
  PLd / stop-distance as Latch. See [`NONGOALS.md`](NONGOALS.md).

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

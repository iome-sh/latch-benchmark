# Roadmap

Status as of 2026-10-03.

The L0 spec Exists. The L1 mock oracle Exists. The public-boundary lint Exists. The mock oracle recomputes `fixtures/latch-robot-1.jsonl`. That recompute Exists. The Latch bit-match is Gap. BM-09, BM-10, and L2 are Gap. `lbs_node_wrap` passed. It links the in-tree mock, and it is not a gz-sim launch.

No Latch `.so` is committed. `latch_mock.c` is the in-tree stand-in. It is not Latch. An external shared library may be supplied with `LATCH_LIB_DIR` or `LATCH_LIBRARY`. When it is supplied, the harness links that file and calls it. The line `BM-01 Latch bit-match 60/60` is printed only when that run matches all 60 rows. With the library linked, `lbs_verify` and `lbs_soak` compare `LATCH_GOLDENS` and exit 0 only when both match all 60 rows. The in-tree fixture replay does not exit 0. BM-09, BM-10, and L2 may be Exists only for that match. A mock recompute does not mark them Exists. Public CI leaves both variables unset and runs `fixtures/latch-robot-1.jsonl`.

A gz-sim server launch has run headless for a fixed number of iterations. That world has no Latch plugin. A separate CI job starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command and does not read the mode. Another job clones BehaviorTree.ROS2 at `72a3bf51dad332c67b99fc3373ccd5242f94f680` and ticks a read-only condition on `/latch/mode_name`. The condition does not elect a mode and does not publish. A later job runs that controller and that condition together.

The partner launch Exists. It starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs the condition in the same job. Approach on `/latch/mode_name` moves the sim joint. Yield leaves the setpoint. Latch is not linked. The evidence of earlier one-joint Actions runs is not in this repository. On that run, bit-match, live evidence, chatter, stale sense, deny, budget, reject, evidence replay, soak, the operator signature, and the latency histogram stay Gap.

These notes describe the validation path. They are not a claim that the product ships inside Gazebo, Isaac, or MuJoCo. The harness map is in [`INTEGRATION.md`](INTEGRATION.md).

## What exists and what is still a gap

| Item | Status | Note |
|------|--------|------|
| L0 spec, schemas, and the open-source files | Exists | Docs lint is green on main. |
| Public-boundary lint | Exists | `scripts/lint_public_boundary.sh` is on main. The docs CI job runs it. |
| L1 MuJoCo C sources | Exists | `harness/mujoco` steps near 1 kHz, calls `latch_consider` at 20 Hz, and calls `latch_apply_reject` before the position setpoint. This build is the mock oracle. |
| L1 public CI when sources are present | Exists | `.github/workflows/ci.yml` takes the cmake branch when sources are present. The mock recompute Exists. The Latch bit-match is Gap. BM-09, BM-10, and L2 are Gap. |
| Mock recompute of `fixtures/latch-robot-1.jsonl` | Exists | The public mock recomputes its own 60-row fixture. The Latch bit-match is Gap. |
| BM-01 through BM-08, and BM-12 | Exists on the mock oracle | These are the L1 targets. A public pass line is not a Latch oracle result. Raw flip counts keep the previous name when a row sets reset, so an alternating raw sequence is not counted as zero. |
| BM-09 and BM-10 | Gap | Public CI does not link a Latch library. With a library linked, verify and soak compare `LATCH_GOLDENS` and exit 0 only when both match all 60 rows. The in-tree fixture replay does not exit 0. They are Exists only for that match. A mock recompute does not mark them Exists. |
| L2 evidence verify and soak | Gap | Public CI does not link a Latch library. L2 is Exists only when that same `LATCH_GOLDENS` verify and soak match all 60 rows. A mock recompute does not mark L2 Exists. |
| `lbs_node_wrap` | Exists as the mock oracle | CI reported that it passed. It links the in-tree mock. It is not a gz-sim launch. |
| L3 ROS 2 and gz partner launch | Exists | The launch described above. The other checklist rows on that run are Gap. |
| `scripts/fa_checklist.md` | Exists | `scripts/fa_checklist_run.sh` names `lbs_bm`, `lbs_verify`, `lbs_soak`, and `lbs_noalloc`. The mock recompute Exists. The Latch bit-match is Gap. BM-09, BM-10, and L2 are Gap. If `build/ros2_gz/lbs_node_wrap` exists, the script runs it and prints that it is the mock oracle, not a gz-sim launch. If that binary is absent, the script says so and the MuJoCo checklist continues. The MuJoCo invocation does not start gz-sim. `scripts/fa_checklist_run.sh partner` runs `harness/ros2_gz/launch_partner.sh` and scores the list against that launch. |

The tracked 60-row file Exists. No Latch `.so` is committed with it. The partner checklist rows that this launch does not run are Gap. Those rows are bit-match, live evidence, chatter, stale sense, deny, budget, reject, evidence replay, soak, the operator signature, and the latency histogram. `lbs_node_wrap` is not that launch. A node wraps Latch. A physics plugin does not elect the mode.

## Two paths

| Role | Stack | Phase | Gate |
|------|-------|-------|------|
| Primary CI | MuJoCo C harness (`harness/mujoco`): bit-match, chatter, latency, and the allocation check | L1, required | Yes |
| Partner run | ROS 2, gz-sim, gz_ros2_control, and a BehaviorTree.ROS2 condition (`harness/ros2_gz`) | L3, required | Yes |
| Optional internal clip | Isaac | Stretch (L5, BM-S8) | No |

L1 and L3 are required. Isaac is stretch work and is never a gate.

## Phases

### L0, spec freeze (Exists)

The spec, the banned measures, the nearby-work notes, and the schema stubs are in the tree. The community files are here: the Apache-2.0 license, NOTICE, the code of conduct, SECURITY, and CONTRIBUTING. The original harness directories were stubs. The MuJoCo stub is now the L1 source. `harness/ros2_gz` has the mock node wrap, and `lbs_node_wrap` passed. The partner launch is the one described in the status section.

The metric IDs are frozen, and the pass rules can be copied into a partner checklist. That exit is met.

### L1, MuJoCo harness on CI (required)

This is the CI path. It is a headless C harness. DDS is not part of this gate.

The sources Exist. `fixtures/latch-robot-1.jsonl` is tracked. The mock recompute Exists. The Latch bit-match is Gap. BM-01 through BM-08 and BM-12 are the L1 targets. BM-09, BM-10, and L2 are Gap.

The plant steps near 1 kHz. `latch_consider` runs at 10 to 50 Hz. `latch_apply_reject` runs before the setpoint is used. The mode name selects a position setpoint. There is no torque output. The Latch library, when a run needs it, comes from outside this tree and is not placed on a public cache.

The exit for this phase is a green CI run on the reference machine with that library.

### L2, evidence verify and the two-process soak (Gap)

Public CI does not link a Latch library. BM-09 is Gap. BM-10 is Gap. L2 is Gap.

The verify tool is BM-09, and it is Gap. The soak runs at least two processes. That is BM-10, and it is Gap. The optional sense-jitter soak (BM-S6) is not in this tree.

The exit stays Gap until a linked library's verify and soak both match all 60 rows.

### L3, ROS 2 and gz partner launch (required; the launch Exists; the other rows are Gap)

This is the partner path: ROS 2, gz-sim, gz_ros2_control, and a BehaviorTree.ROS2 condition. `lbs_node_wrap` passed in CI and links the mock oracle. A physics plugin does not elect the mode. The launch details are in the status section above. Latch is not linked.

`latch_consider` runs at about 20 Hz. `latch_apply_reject` runs before the controller uses the record. The existing ros2_control mapping turns the mode name into a position behavior, and Controller Manager keeps the lifecycle. An independent `/emergency_stop`, or a gz plug, is the BM-07 check on topics. The BehaviorTree.ROS2 condition is read-only, which is the BM-11 check. The checklist is `scripts/fa_checklist.md`.

The partner path can be started. The partner launch Exists. The other checklist rows stay Gap on that run. The mock recompute Exists. The Latch bit-match is Gap. BM-09, BM-10, and L2 are Gap.

### L4, partner acceptance pack

A runbook and an artifact bundle go on the share path used for that partner. There is a place for an acceptance signature. The exit is that pack. It is a validation path.

### L5, stretch work after the required phases are green

MCAP export is BM-S4. An informative audit-trail field map is BM-S5. Override notes are BM-S1 and BM-S2. The Isaac clip (BM-S8) shows yield leaving the setpoint. It is stretch work only. It can be a camera clip or a sensor-rich clip, and it does not block L1, L3, or the first partner open. Packaging a Kit-bundled turnkey needs a separate NVIDIA license check.

## Latch tree boundary

The Latch product keeps its ABI headers, the native core, and the oracle goldens. This repository calls that ABI through an optional external library. The library is not stored in this tree.

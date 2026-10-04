# Latch Benchmark Suite

This repository is the open benchmark for an in-process robot mode election. The license is Apache-2.0. The organization is [`iome-sh`](https://github.com/iome-sh).

Latch itself is a separate product and is not in this tree. What is here is the harness, the fixtures, the schemas, and the scripts a partner can run.

The suite checks whether the elected mode is repeatable, whether the name chatters when the sense is noisy, how long `latch_consider` takes, and whether a recorded session can be replayed. Yield means the mode leaves the setpoint. It is not a protective stop and not a rated stop.

The suite does not score grasp success. It is not a SIL or ISO pack. It is not a behavior tree.

## What each metric checks

| ID | Check |
|----|--------|
| BM-01 | The run matches the frozen golden, including the evidence hex. |
| BM-02 | Noise does not keep changing the mode name. |
| BM-03 | Stale sense elects yield on the `l2` plane. That is not a deny. |
| BM-04 | An e-stop, a collision, or a joint limit elects yield on the deny plane. |
| BM-05 | If the budget is missed, the last legal mode or hold is kept. The plant tick is not stalled. |
| BM-06 | If a precondition fails after consider, the mode becomes hold before the controller uses the name. |
| BM-07 | Yield leaves the setpoint. A separate stop input still trips. Yield is not a protective stop and not a rated stop. |
| BM-08 | Consider latency, reported as p50, p99, and max. |
| BM-09 | The verify tool recomputes the evidence hashes from a session file. |
| BM-10 | Two processes given the same inputs produce the same modes and the same evidence. |
| BM-11 | The binding publishes a mode record. It does not command torque, it does not run a behavior tree, and it does not activate controllers. |
| BM-12 | No illegal mode is emitted. |

The pass rules are in [`docs/SPEC.md`](docs/SPEC.md). Measures this suite does not use are in [`docs/NONGOALS.md`](docs/NONGOALS.md).

## Status

The spec and the MuJoCo mock harness exist. Public CI does not link a Latch library.

The mock oracle recomputes `fixtures/latch-robot-1.jsonl`. That recompute Exists. The Latch bit-match on those 60 rows is Gap, because the library is not linked. BM-09, BM-10, and L2 are Gap on that public run.

`harness/mujoco` steps the plant and calls Latch at a slower rate. MuJoCo is Apache-2.0. The build fetches it, and it is not committed.

When `LATCH_LIBRARY` or `LATCH_LIB_DIR` is set, the harness links that shared library and calls it. The line `BM-01 Latch bit-match 60/60` is printed only when that linked run matches all 60 rows. `lbs_verify` and `lbs_soak` compare the file named by `LATCH_GOLDENS`. They exit 0 only when both match all 60 rows. Replaying the in-tree fixture does not exit 0. BM-09, BM-10, and L2 may be Exists only for that match. A mock recompute does not mark them Exists. Public CI leaves the library unset, so those three stay Gap.

A gz-sim server launch has run headless for a fixed number of iterations. That world has no Latch plugin. A separate CI job starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command and does not read the mode.

Another CI job clones BehaviorTree.ROS2 at `72a3bf51dad332c67b99fc3373ccd5242f94f680` and ticks a read-only condition on `/latch/mode_name`. The condition does not elect a mode and does not publish. A later job runs that controller and that condition together.

The one-joint partner launch Exists. A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs the condition in the same job. Approach on `/latch/mode_name` moves the sim joint. Yield leaves that command. The job runs the mock-oracle process with `--drive-sim`. An e-stop elects yield and leaves the command. Latch is not linked. The empty world does not elect a mode.

`scripts/fa_checklist_run.sh partner` starts `harness/ros2_gz/launch_partner.sh`, which starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The log prints `gz-sim launched` or `gz-sim did not start`. Yield leaves the setpoint. That launch is the mock oracle. It is not a Latch certificate.

On that partner run, bit-match, live evidence, chatter, stale sense, deny, budget, reject, evidence replay, soak, the operator signature, and the latency histogram are Gap. The 10-row mock fixture is a separate file. No Latch `.so` is committed. `lbs_node_wrap` passed in CI. It links the in-tree mock, and it is not a gz-sim launch.

## How the two harnesses are used

The MuJoCo harness is the CI path. It runs BM-01 through BM-08 and BM-12. The mock recompute of `fixtures/latch-robot-1.jsonl` Exists. The Latch bit-match is Gap. BM-09, BM-10, and L2 are Gap unless a provided library's verify and soak both match all 60 golden rows.

The ROS 2 and gz partner launch runs the one-joint mock path. It does not run bit-match, chatter, latency, or soak. Those stay on the MuJoCo harness.

| Role | Path | What it is for |
|------|------|----------------|
| Primary CI | MuJoCo C harness, `harness/mujoco`. Required for L1. | Bit-match, chatter, latency, and the allocation check. Headless. DDS is not part of this gate. |
| Partner run | ROS 2, gz-sim, gz_ros2_control, and a BehaviorTree.ROS2 condition, in `harness/ros2_gz`. Required for L3. | The partner launch. A node wraps Latch. The behavior-tree condition reads the mode name. The stop topic is separate from the mode. |
| Optional | Isaac, stretch item BM-S8. | An internal clip only. It is not a gate. |

The plant runs near 1 kHz and does not call Latch in the middle of a tick. Sense code calls `latch_consider` at 10 to 50 Hz. If a precondition fails late, `latch_apply_reject` runs before the controller uses the name. The mode name selects a position setpoint on the controller that is already there. The harness does not emit torque. No Latch `.so` is stored in this tree.

Which harness may speak for each metric is in [`docs/INTEGRATION.md`](docs/INTEGRATION.md). The partner checklist is [`scripts/fa_checklist.md`](scripts/fa_checklist.md).

## Linking Latch

The Latch core stays under its own license. When a run needs it, point the build at a library outside this tree. Do not commit the library.

Do not publish that library on a public Actions cache. See [`fixtures/README.md`](fixtures/README.md).

## Layout

`docs/` holds the spec, the roadmap, the harness notes, the banned measures, and notes on nearby work. `schemas/` describes evidence v2, the mode record, and the golden file. `fixtures/` explains the golden file. There is no Latch binary there. `harness/mujoco` is the L1 source. `harness/ros2_gz` is the partner launch. That launch Exists as the mock oracle. The other checklist rows are Gap. `scripts/` holds the partner checklist. `.github/` holds CI and the issue and pull request templates.

## License

Apache License 2.0. See [`LICENSE`](LICENSE) and [`NOTICE`](NOTICE).

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) and [`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md). Security reports go to [`SECURITY.md`](SECURITY.md).

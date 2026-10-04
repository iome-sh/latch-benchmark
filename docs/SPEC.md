# Metric definitions (BM-01 through BM-12)

The L0 spec Exists. The MuJoCo L1 harness Exists as the mock oracle.

The mock oracle recomputes `fixtures/latch-robot-1.jsonl`. That recompute Exists. The Latch bit-match is Gap. BM-09, BM-10, and L2 are Gap on public CI. `lbs_node_wrap` passed. When `LATCH_LIBRARY` or `LATCH_LIB_DIR` is set, the harness links that shared library and calls it. The line `BM-01 Latch bit-match 60/60` is printed only when that run matches all 60 rows. With the library linked, `lbs_verify` and `lbs_soak` compare `LATCH_GOLDENS` and exit 0 only when both match all 60 rows. The in-tree fixture replay does not exit 0. BM-09, BM-10, and L2 may be Exists only for that match. A mock recompute does not mark them Exists. Public CI leaves the library unset, so they stay Gap.

A gz-sim server launch ran headless for a fixed iteration count. The world has no Latch plugin. A separate CI workflow starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command without reading the mode.

A separate CI workflow clones BehaviorTree.ROS2 at 72a3bf51dad332c67b99fc3373ccd5242f94f680 and ticks a read-only condition on /latch/mode_name. The condition does not elect and does not publish. Another workflow starts that mock controller and that condition in one job.

**One-joint partner launch.** A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs that condition in the same job. Approach on /latch/mode_name moves the sim joint through the position controller. Yield leaves that command.

The one-joint gz job runs that mock-oracle process with --drive-sim. Approach moves the sim joint. E-stop elects yield and leaves the command. It is not Latch. Latch is not linked. The empty world does not elect a mode.

The partner checklist runs that one-joint path. `scripts/fa_checklist_run.sh partner` starts `harness/ros2_gz/launch_partner.sh`, which starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The log prints `gz-sim launched` or `gz-sim did not start`. Yield leaves the setpoint. The partner launch Exists as the mock oracle. It is not a Latch certificate.

The other partner-launch rows are Gap. The mock recompute of `fixtures/latch-robot-1.jsonl` Exists. The Latch bit-match is Gap. BM-09, BM-10, and L2 are Gap. The 10-row mock fixture is a separate file. No Latch `.so` is committed.

This suite elects one of six modes for a controller that already exists. The result is a signed mode record. The tick does not emit torque or a trajectory, does not host a model, and does not allocate. The plant stop stays on its own path. Yield means the mode leaves the setpoint. It is not a protective stop and not a rated stop.

These are the metrics the suite measures. Nearby work is listed in [`ADJACENCY.md`](ADJACENCY.md). Measures the suite does not use are in [`NONGOALS.md`](NONGOALS.md).

BM-01 through BM-12 stay at these numbers. Do not renumber them without maintainer approval. The pass rules below are the targets. Harness code lands in L1 through L4 ([`ROADMAP.md`](ROADMAP.md)). Which harness may speak for each ID is in [`INTEGRATION.md`](INTEGRATION.md) and in the table below.

---

## Must metrics

| ID | Metric | Pass rule | Phase | Note |
|----|--------|-----------|-------|------|
| **BM-01** | Golden bit-match | All 60 rows match on name, score, margin, dwell, plane, and evidence hex. | L1 | The golden file is the reference. |
| **BM-02** | Chatter | On the chatter fixture, Latch mode flips are at most 3 because of dwell. Raw argmax flips are at least 11, or the documented N. Evidence stays stable for the run. | L1 | Noise does not keep changing the mode name. |
| **BM-03** | Stale sense | When `sense_age` is at least 4, the mode is yield on plane `l2`. That is not a deny. | L1 | Stale sense elects yield. It is not an emergency stop. |
| **BM-04** | Deny | E-stop, collision, or a joint limit elects yield on plane `deny`, and dwell is cleared. | L1 | Those denials are named on the record. |
| **BM-05** | Budget | Over budget, the mode is the last legal mode or hold, on plane `budget`. The plant tick is not stalled. See BM-08. | L1 | A missed budget does not freeze the servo. |
| **BM-06** | Apply-time reject | After consider, if a precondition fails, the mode is hold on plane `reject`, through `latch_apply_reject`. | L1 | A late failure becomes hold. The previous name is not left in place. |
| **BM-07** | Yield and the separate stop | Yield leaves the setpoint in the plant stub. A separate stop input still trips the plant. | L1 through L3 | Yield means the mode leaves the setpoint. It is not a protective stop and not a rated stop. |
| **BM-08** | Consider latency | Report p50, p99, and max `latch_consider` time in microseconds under soak. On the reference machine the max stays under 1000 microseconds, tuned for partner hardware. The budget path does not block the plant tick. | L1 | The consider call is timed. |
| **BM-09** | Evidence replay | A session JSONL file and the verify tool recompute every hash. | L2 | The recorded session can be checked later. |
| **BM-10** | Two-process soak | At least two processes, the same blob, and the same sense stream produce the same modes and the same evidence. | L2 | The two processes agree. |
| **BM-11** | Record only | The binding publishes mode-record fields only. The behavior-tree kit is a read-only condition. Latch does not activate or deactivate controllers. | L3 | The partner checklist includes this check. |
| **BM-12** | No illegal mode | No mode is emitted outside the legal mask. The soak holds that property. | L1 | The mask is part of the contract. |

BM-01 bit-match uses the tracked fixture `fixtures/latch-robot-1.jsonl`.
This repo never ships Latch source.

---

## Harness cross-link

L1 MuJoCo (`harness/mujoco`) and L3 ROS 2 + gz (`harness/ros2_gz`) are
**Must**. Isaac is **Stretch** and is not a gate. Shared wrap is
`latch_consider` at 10–50 Hz and `latch_apply_reject` before the controller
consumes the name. Detail: [`INTEGRATION.md`](INTEGRATION.md).

| ID | Speaks for this metric | Also |
|----|------------------------|------|
| **BM-01** | MuJoCo goldens runner ([harness notes](../harness/mujoco/README.md)) | Oracle stays in the Latch tree |
| **BM-02** | MuJoCo chatter fixture | ROS 2/gz FA demo |
| **BM-03** | MuJoCo stale fixture | ROS 2/gz FA demo |
| **BM-04** | MuJoCo deny fixture | ROS 2/gz FA demo |
| **BM-05** | MuJoCo budget fixture | ROS 2/gz FA demo |
| **BM-06** | MuJoCo `latch_apply_reject` | ROS 2/gz, before controller consume |
| **BM-07** | Both Must harnesses | MuJoCo: yield leaves `qpos` target. ROS 2/gz: independent `/emergency_stop` |
| **BM-08** | MuJoCo histogram (CI number) | ROS 2/gz second; DDS jitter is outside consider |
| **BM-09** | L2 verify CLI | Either Must harness may emit session JSONL |
| **BM-10** | L2 soak (≥2 processes) | MuJoCo two processes, or two ROS nodes |
| **BM-11** | ROS 2/gz BT.ROS2 read-only Condition ([harness notes](../harness/ros2_gz/README.md)) | MuJoCo adapter: mode-record fields only |
| **BM-12** | MuJoCo soak + ASAN no-alloc | — |
| **BM-S8** | Isaac internal clip | Stretch only. Not a gate. |

---

## Stretch metrics

These are not required before the first partner run.

| ID | Metric | Phase | Note |
|----|--------|-------|------|
| **BM-S1** | Override rate rail | L5 | `rejected_elections / elections`; caption: *not contact-success* |
| **BM-S2** | Cancel-log completeness | L5 | Row: mode, score, margin, snapshot, plane, evidence, override flag |
| **BM-S3** | Seventh-mode-via-snapshot | L5 | New bit + rank + behavior without tick rewrite |
| **BM-S4** | MCAP evidence export | L5 | Packaging adjacency to Foxglove; not sealed forensics |
| **BM-S5** | Informative AAT field map | L5 | Docs table only; **no** conformance badge |
| **BM-S6** | Dual-process soak under sense jitter | L5 | Hardens BM-10 |
| **BM-S7** | Asymmetric enter/exit margins | L5 | Only if fixture demands |
| **BM-S8** | Isaac internal clip of yield leaving the setpoint | Stretch | Internal video only. Not a CI gate. See [`INTEGRATION.md`](INTEGRATION.md). |

BM-S1, BM-S2, and BM-S3 enter FA only after a cancel-log schema exists. Do not fake them early.

---

## Comparing a release with the previous baseline

A release that claims the suite got better should report the change from the previous baseline:

| Signal | Better direction | Gate |
|--------|------------------|------|
| Change in flip ratio (raw divided by Latch) on the chatter fixture | A higher ratio, or fewer Latch flips | BM-02 |
| Change in p99 consider microseconds | Lower, and max stays under 1000 microseconds on the reference machine | BM-08 |
| Illegal emissions | Stay at zero | BM-12 |
| Soak process count | At least 2, and higher over time | BM-10 |
| Verify match | Stay at 100 percent | BM-09 |
| Yield and the separate stop | Pass or fail. Do not weaken the check. | BM-07 |

Do not put the banned measures on this comparison.

---

## Schemas

See [`../schemas/`](../schemas/) for evidence-v2, mode-record, and golden-jsonl
field notes (stubs until eng).

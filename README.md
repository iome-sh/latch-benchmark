# Latch Benchmark Suite

**Apache-2.0** · OSS-ready scaffold · org [`iome-sh`](https://github.com/iome-sh)

Benchmark and validation suite for **in-process robot mode-election planes**.
Measures signed mode-election quality — determinism, chatter damp, consider
latency, evidence verify, and yield≠stop honesty.

> **Not** grasp success. **Not** SIL / ISO certification. **Not** a BehaviorTree.
> Latch core is a separate proprietary product; this repo is the openable
> harness, fixtures, schemas, and FA scripts.

## What this suite measures

| ID | Metric (short) |
|----|----------------|
| BM-01 | Golden bit-match (oracle frozen) |
| BM-02 | Chatter damp under noise |
| BM-03 | Stale → soft yield |
| BM-04 | Deny planes |
| BM-05 | Budget keep (never freezes the servo) |
| BM-06 | Apply-time reject |
| BM-07 | Yield ≠ stop series honesty |
| BM-08 | Consider latency (p50/p99/max) |
| BM-09 | Evidence replay verify |
| BM-10 | Multi-process soak determinism |
| BM-11 | No-torque / not-BT / not-CM asserts |
| BM-12 | Illegal emission = 0 |

Full definitions: [`docs/SPEC.md`](docs/SPEC.md). Banned Miss KPIs: [`docs/NONGOALS.md`](docs/NONGOALS.md).

## Status

**Exists.** L0 **Exists**. L1 MuJoCo mock-oracle **Exists**. Spec / roadmap / honesty docs stay the contract. `harness/mujoco` is a two-rate C harness (MuJoCo fetched at configure, Apache-2.0, not committed).

Public CI success ran mock-oracle `lbs_verify` and `lbs_soak`. The log says this is not a Latch certificate and still printed `not Latch 60/60 bit-match`. The optional BM-01 step printed `BM-01 60/60 not claimed` and skipped configure. `lbs_node_wrap` Passed. A real `liblatch` is optional via `LATCH_LIB_DIR`.

A gz-sim server launch ran headless for a fixed iteration count. The world has no Latch plugin. A separate CI workflow starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command without reading the mode.

A separate CI workflow clones BehaviorTree.ROS2 at 72a3bf51dad332c67b99fc3373ccd5242f94f680 and ticks a read-only condition on /latch/mode_name. The condition does not elect and does not publish. Another workflow starts that mock controller and that condition in one job.

**One-joint partner launch.** A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs that condition in the same job. Approach on /latch/mode_name moves the sim joint through the position controller. Yield leaves that command.

The one-joint gz job runs that mock-oracle process with --drive-sim. Approach moves the sim joint. E-stop elects yield and leaves the command. It is not Latch. Latch is not linked. The empty world does not elect a mode.

The partner checklist runs that one-joint path. `scripts/fa_checklist_run.sh partner` starts `harness/ros2_gz/launch_partner.sh`, which starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The log prints `gz-sim launched` or `gz-sim did not start`. Yield leaves the setpoint. The partner launch **Exists** as mock-oracle. It is not a Latch certificate.

**Gap.** L2 verify and soak stay Gap. BM-09 and BM-10 stay Gap. Bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram stay **Gap** on that run. **Gap:** the 60-row golden through the bench harness (external library and goldens are not present on public CI). Remaining partner-checklist rows stay **Gap**. No Latch source or proprietary `.so` is committed.

## How we validate

Two Must paths, one Stretch. The MuJoCo harness runs BM-01..08, BM-12, and mock-oracle BM-09/10.
BM-01 on public CI is the mock-oracle fixture, not the Latch 60-row oracle.
BM-09/10 on public CI are that same fixture, not a Latch evidence certificate.
The ROS 2 / gz partner launch runs the one-joint mock-oracle path. It does not run bit-match, chatter, latency, or soak. Those stay on the MuJoCo harness.

| Role | Path | What it is for |
|------|------|----------------|
| Primary CI truth | **MuJoCo C harness** — `harness/mujoco` (**L1 Must**) | Bit-match, chatter, latency, no-alloc. Headless. No DDS in the gate. |
| Primary partner FA | **ROS 2 + gz-sim + gz_ros2_control + BT.ROS2 condition** — `harness/ros2_gz` (**L3 Must**) | Partner launch. Node wraps Latch. BT condition reads the mode name. Independent stop topic. |
| Optional internal | **Isaac** (**Stretch** / BM-S8) | Internal clip only. Not a gate. |

Wrap: the plant (~1 kHz) does not call Latch mid-tick. Sense stubs call
`latch_consider` at 10–50 Hz. A late precondition failure calls
`latch_apply_reject` before the controller consumes the name. The name maps
to the existing controller’s position setpoints (no torque). No Latch `.so`
is vendored in this tree. Map and non-claims: [`docs/INTEGRATION.md`](docs/INTEGRATION.md).
Checklist spine: [`scripts/fa_checklist.md`](scripts/fa_checklist.md).

## Latch linking (optional)

The proprietary Latch core remains All Rights Reserved. When needed, link via
an optional external library, never vendored in this tree.

Never publish Latch onto a public Actions cache. See [`fixtures/README.md`](fixtures/README.md).

## Layout

```
docs/          SPEC, ROADMAP, INTEGRATION, NONGOALS, ADJACENCY
schemas/       evidence-v2, mode-record, golden-jsonl
fixtures/      golden how-to (no Latch binary)
harness/       mujoco (L1 sources) · ros2_gz (partner gz launch Exists as mock-oracle; other checklist rows Gap)
scripts/       FA checklist spine
.github/       CI stub, issue/PR templates
```

## License

Apache License 2.0 — see [`LICENSE`](LICENSE) and [`NOTICE`](NOTICE).

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) and [`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md).
Security reports: [`SECURITY.md`](SECURITY.md).

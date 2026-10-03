# Contributing to Latch Benchmark Suite

Thanks for helping improve LBS. This repo is the **benchmark / validation
harness** — not Latch core.

## Ground rules

1. **No Latch proprietary source or `.so`** in PRs. Link Latch only via an
   optional external library that is never vendored in this tree.
2. **Metric honesty.** Do not add grasp-success, SIL/ISO-as-Latch, BT coverage,
   or other Miss KPIs (see `docs/NONGOALS.md`). New metrics need SPEC updates
   and must not renumber BM-01..12 without maintainer approval.
3. **Apache-2.0.** Contributions are under the Apache License 2.0 unless
   explicitly stated otherwise.
4. Follow the [Code of Conduct](CODE_OF_CONDUCT.md).

## How to contribute

1. Open an issue describing the change (bug, fixture, harness stub, docs).
2. Fork / branch from `main`.
3. Keep PRs focused; include checklist from `.github/PULL_REQUEST_TEMPLATE.md`.
4. Docs and schema changes are welcome early; harness code lands with L1+.

## Development notes

- L1 MuJoCo harness: `harness/mujoco/` (sources landed; mock-oracle unless `LATCH_LIB_DIR` is set).
- L3 ROS 2 / gz path: `harness/ros2_gz/` (`lbs_node_wrap` Passed as mock-oracle). A gz-sim server launch ran headless for a fixed iteration count. The world has no Latch plugin. A separate CI workflow starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command without reading the mode. A separate CI workflow clones BehaviorTree.ROS2 at 72a3bf51dad332c67b99fc3373ccd5242f94f680 and ticks a read-only condition on /latch/mode_name. The condition does not elect and does not publish. Another workflow starts that mock controller and that condition in one job. A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs that condition in the same job. Approach on /latch/mode_name moves the sim joint through the position controller. Yield leaves that command. The one-joint gz job runs that mock-oracle process with --drive-sim. Approach moves the sim joint. E-stop elects yield and leaves the command. It is not Latch. Latch is not linked. The empty world does not elect a mode. The partner checklist runs that one-joint path. `scripts/fa_checklist_run.sh partner` starts `harness/ros2_gz/launch_partner.sh`, which starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The log prints `gz-sim launched` or `gz-sim did not start`. Yield leaves the setpoint. The partner launch **Exists** as mock-oracle. It is not a Latch certificate. Bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram stay **Gap** on that run.
- CI: `.github/workflows/ci.yml` lints docs, builds the MuJoCo harness when `*.c` / `*.cpp` sources are present, and runs `lbs_node_wrap`. Public CI does not claim the 60-row golden.

## Questions

Use GitHub Issues. For security, see [SECURITY.md](SECURITY.md).

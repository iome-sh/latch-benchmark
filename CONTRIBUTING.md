# Contributing to Latch Benchmark Suite

Thanks for helping improve LBS. This repo is the **benchmark / validation
harness** — not Latch core.

## Ground rules

1. **No Latch proprietary source or `.so`** in PRs. Link Latch only via an
   optional external library that is never vendored in this tree.
2. **Stay with the metrics in the spec.** Do not add grasp-success, SIL or ISO treated as Latch, behavior-tree coverage,
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

The MuJoCo harness is `harness/mujoco/`. It uses the mock oracle unless `LATCH_LIB_DIR` or `LATCH_LIBRARY` is set. A provided library is linked and called. The line `BM-01 Latch bit-match 60/60` is printed only when that run matches all 60 rows. With the library linked, `lbs_verify` and `lbs_soak` compare `LATCH_GOLDENS` and exit 0 only when both match all 60 rows. The in-tree fixture replay does not exit 0. BM-09, BM-10, and L2 may be Exists only for that match. A mock recompute does not mark them Exists.

The ROS 2 and gz path is `harness/ros2_gz/`. `lbs_node_wrap` passed as the mock oracle. The partner launch Exists. `scripts/fa_checklist_run.sh partner` starts `harness/ros2_gz/launch_partner.sh`, which starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. Yield leaves the setpoint. Latch is not linked. The other checklist rows on that run are Gap. The current status is in the README.

CI is `.github/workflows/ci.yml`. It lints the docs, builds the MuJoCo harness when C or C++ sources are present, and runs `lbs_node_wrap`. The mock recompute of `fixtures/latch-robot-1.jsonl` Exists. The Latch bit-match is Gap. BM-09, BM-10, and L2 are Gap.

## Questions

Use GitHub Issues. For security, see [SECURITY.md](SECURITY.md).

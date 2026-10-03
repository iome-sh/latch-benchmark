# MuJoCo harness (L1 Must — primary CI truth)

**Status (tip `beef75f8ced96163c44aaca18fd9038352a96da1`):** L1 sources **Exist**. Public CI runs the mock oracle. `lbs_verify` and `lbs_soak` ran; the log says this is not a Latch certificate and still printed `not Latch 60/60 bit-match`. The 60-row golden through the bench harness is **Gap** (external library and goldens are not present on public CI). No Latch `.so` is committed.  
**Plan:** [`../../docs/INTEGRATION.md`](../../docs/INTEGRATION.md) · metrics [`../../docs/SPEC.md`](../../docs/SPEC.md)

## Goal

Headless C harness is the merge-adjacent gate: bit-match, chatter, latency,
no-alloc. No DDS in this number.

## Wrap

Single process, two rates.

- `mj_step` at ~1 kHz. The plant step never calls Latch mid-tick.
- Every N steps, `latch_consider` at 10–50 Hz. Sense from `mjData` contacts
  and forces.
- `latch_apply_reject` before the setpoint is consumed.
- Mode name → position setpoints only (`qpos` / `ctrl` target table). **No
  torque path.**

## Coverage

| ID | Here |
|----|------|
| BM-01 | JSONL runner. Public CI matches the mock-oracle fixture only. Exported Latch rows run when `LATCH_GOLDENS` is set and a real library is linked |
| BM-02..06 | Chatter, stale, deny, budget, apply-reject fixtures |
| BM-07 | Yield leaves `qpos` target; independent stop inject stub |
| BM-08 | p50/p99/max `latch_consider` histogram (the CI number) |
| BM-09 | `lbs_verify` replays the in-tree fixture through the linked ABI and checks evidence. Mock-oracle in public CI. Not a Latch evidence-v2 certificate |
| BM-10 | `lbs_soak` runs two processes on that same sense stream and compares name, plane, and evidence. Mock-oracle |
| BM-12 | Illegal emission = 0; malloc wrap on the election tick. Optional ASan via `LATCH_BENCH_ASAN` |

`lbs_contact` elects grasp from contact plus a high wrench, approach from a seen target with a low wrench, and yield from e-stop. The same state keeps approach for three dwell ticks when contact and a high wrench arrive, then elects grasp. It is the mock oracle. This binary does not start gz-sim.

Partner FA for the same metrics is `harness/ros2_gz` (L3 Must). `lbs_node_wrap` Passed as mock-oracle. A gz-sim server launch ran headless for a fixed iteration count. The world has no Latch plugin. A separate CI workflow starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command without reading the mode. A separate CI workflow clones BehaviorTree.ROS2 at 72a3bf51dad332c67b99fc3373ccd5242f94f680 and ticks a read-only condition on /latch/mode_name. The condition does not elect and does not publish. Another workflow starts that mock controller and that condition in one job. A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs that condition in the same job. Approach on /latch/mode_name moves the sim joint through the position controller. Yield leaves that command. The one-joint gz job runs that mock-oracle process with --drive-sim. Approach moves the sim joint. E-stop elects yield and leaves the command. It is not Latch. Latch is not linked. The empty world does not elect a mode. The partner checklist runs that one-joint path. `scripts/fa_checklist_run.sh partner` starts `harness/ros2_gz/launch_partner.sh`, which starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The log prints `gz-sim launched` or `gz-sim did not start`. Yield leaves the setpoint. The partner launch **Exists** as mock-oracle. It is not a Latch certificate. Bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram stay **Gap** on that run.
Isaac is Stretch and is not this gate.

## No Latch `.so` in-tree

Do not vendor Latch source or `liblatch*`. Link via `LATCH_LIB_DIR` or
`LATCH_LIBRARY` (optional external library, never vendored in this tree). `latch_mock.c`
is the in-tree stand-in and is not Latch. MuJoCo is Apache-2.0 and is fetched
at configure time, not committed.

## Build

Default configure fetches Apache-2.0 MuJoCo 3.14.0 for linux-x86_64 (not
committed) and compiles `latch_mock.c` when no private library is set.
`cmake --build` runs the fixture binaries. `LATCH_BENCH_BUILD_MUJOCO=ON` is
the default and is not a configure error.

```bash
cmake -S harness/mujoco -B build/mujoco
cmake --build build/mujoco
ctest --test-dir build/mujoco --output-on-failure
```

Public CI results are **mock-oracle**. They are not a Latch 60/60 bit-match
and the mock digest is not evidence payload v2. BM-02 chatter counts, BM-08
microseconds, and BM-12 illegal=0 are measured against that mock. BM-12's
alloc gate wraps `malloc` / `calloc` / `realloc` on the election tick
(`lbs_noalloc`); `mj_step` is outside that gate.

Optional real Latch (still not committed):

```bash
cmake -S harness/mujoco -B build/mujoco -DLATCH_LIB_DIR=/opt/latch
LATCH_GOLDENS=/path/to/exported.jsonl cmake --build build/mujoco
```

`LATCH_LIBRARY` may be a full path instead of `LATCH_LIB_DIR`. Without
`LATCH_GOLDENS`, a real link does not claim bit-match. `MUJOCO_DIR` skips the
fetch. `-DLATCH_BENCH_ASAN=ON` turns on ASan/UBSan when the toolchain ships
compiler-rt (MuJoCo's headers include the sanitizer interface in that mode).

Set `-DLATCH_BENCH_BUILD_MUJOCO=OFF` to configure without building targets.

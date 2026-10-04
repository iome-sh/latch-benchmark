# MuJoCo harness

This is the CI path, and L1 requires it. The L1 sources Exist. The mock oracle recomputes `fixtures/latch-robot-1.jsonl`. That recompute Exists. The Latch bit-match is Gap. BM-09, BM-10, and L2 are Gap. The 10-row mock fixture is a separate file. No Latch `.so` is committed.

How the harnesses fit together is in [`../../docs/INTEGRATION.md`](../../docs/INTEGRATION.md). The metrics are in [`../../docs/SPEC.md`](../../docs/SPEC.md).

## Goal

Headless C harness is the merge-adjacent gate: bit-match, chatter, latency,
no-alloc. No DDS in this number.

## Wrap

Single process, two rates.

- `mj_step` at ~1 kHz. The plant step never calls Latch mid-tick.
- Every N steps, `latch_consider` at 10–50 Hz. Sense from `mjData` contacts
  and forces.
- `latch_apply_reject` before the setpoint is consumed.
- The mode name selects a position setpoint (`qpos` or `ctrl`). There is no torque path.

## Coverage

| ID | Here |
|----|------|
| BM-01 | JSONL runner. The mock recompute of `fixtures/latch-robot-1.jsonl` Exists. The Latch bit-match is Gap. The 10-row mock fixture is a separate file. |
| BM-02..06 | Chatter, stale, deny, budget, apply-reject fixtures |
| BM-07 | Yield leaves `qpos` target; independent stop inject stub |
| BM-08 | p50/p99/max `latch_consider` histogram (the CI number) |
| BM-09 | Gap. Public CI does not link a Latch library. |
| BM-10 | Gap. Public CI does not link a Latch library. |
| BM-12 | Illegal emission = 0; malloc wrap on the election tick. Optional ASan via `LATCH_BENCH_ASAN` |

`lbs_contact` elects grasp from contact plus a high wrench, approach from a seen target with a low wrench, and yield from e-stop. The same state keeps approach for three dwell ticks when contact and a high wrench arrive, then elects grasp. It is the mock oracle. This binary does not start gz-sim.

The partner path for the same metrics is `harness/ros2_gz`, which L3 requires. `lbs_node_wrap` passed as the mock oracle. That launch Exists. Yield leaves the setpoint. Latch is not linked. The checklist rows that launch does not run are Gap. Isaac is stretch work and is not this gate.

## No Latch `.so` in-tree

Do not vendor Latch source or `liblatch*`. Link via `LATCH_LIB_DIR` or
`LATCH_LIBRARY` (optional external library, never vendored in this tree). `latch_mock.c`
is the in-tree stand-in and is not Latch. MuJoCo is Apache-2.0 and is fetched
at configure time, not committed.

## Build

Default configure fetches Apache-2.0 MuJoCo 3.14.0 for linux-x86_64 (not
committed) and compiles `latch_mock.c` when no library is set.
`cmake --build` runs the fixture binaries. `LATCH_BENCH_BUILD_MUJOCO=ON` is
the default and is not a configure error.

```bash
cmake -S harness/mujoco -B build/mujoco
cmake --build build/mujoco
ctest --test-dir build/mujoco --output-on-failure
```

The mock recompute of `fixtures/latch-robot-1.jsonl` Exists. The Latch bit-match is Gap. BM-09, BM-10, and L2 are Gap. The 10-row mock digest is not evidence payload v2. BM-02 chatter counts, BM-08
microseconds, and BM-12 illegal=0 are measured against that mock. BM-12's
alloc gate wraps `malloc` / `calloc` / `realloc` on the election tick
(`lbs_noalloc`); `mj_step` is outside that gate.

A provided shared library (still not committed) is linked and called:

```bash
cmake -S harness/mujoco -B build/mujoco -DLATCH_LIB_DIR=/opt/latch
LATCH_GOLDENS=/path/to/exported.jsonl cmake --build build/mujoco
```

`LATCH_LIBRARY` may be a full path instead of `LATCH_LIB_DIR`. The directory of
a shared library is on the binary runtime search path. `BM-01 Latch bit-match 60/60`
is printed only when that linked run matches all 60 rows. With the library
linked, `lbs_verify` and `lbs_soak` compare `LATCH_GOLDENS` and exit 0 only
when both match all 60 rows. The in-tree fixture replay does not exit 0.
BM-09, BM-10, and L2 may be Exists only for that 60-row match. A mock
recompute does not mark them Exists. Without `LATCH_GOLDENS`, the link does
not claim bit-match. Mock score checks are not applied to that
link. `MUJOCO_DIR` skips the fetch. `-DLATCH_BENCH_ASAN=ON` turns on ASan/UBSan
when the toolchain ships compiler-rt (MuJoCo's headers include the sanitizer
interface in that mode).

Set `-DLATCH_BENCH_BUILD_MUJOCO=OFF` to configure without building targets.

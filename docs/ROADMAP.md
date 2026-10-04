# ROADMAP — Latch Benchmark Suite (L0–L4)

**Status (2026-10-03):** L0 **Exists**. L1 mock-oracle **Exists**. Public-boundary lint **Exists**. Mock-oracle recompute of `fixtures/latch-robot-1.jsonl`: **Exists**. BM-01 Latch bit-match: **Gap**. BM-09: **Gap**. BM-10: **Gap**. L2: **Gap**. `lbs_node_wrap` Passed. A gz-sim server launch ran headless for a fixed iteration count. The world has no Latch plugin. A separate CI workflow starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command without reading the mode. A separate CI workflow clones BehaviorTree.ROS2 at 72a3bf51dad332c67b99fc3373ccd5242f94f680 and ticks a read-only condition on /latch/mode_name. The condition does not elect and does not publish. Another workflow starts that mock controller and that condition in one job. A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs that condition in the same job. Approach on /latch/mode_name moves the sim joint through the position controller. Yield leaves that command. The partner launch **Exists** in this repository. It starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The evidence of those earlier one-joint Actions runs is not in this repository. Approach moves the sim joint. Yield leaves the setpoint. Latch is not linked. This is not a Latch certificate. Bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram stay **Gap** on that run. Mock-oracle recompute of `fixtures/latch-robot-1.jsonl`: **Exists**. BM-01 Latch bit-match: **Gap**. BM-09: **Gap**. BM-10: **Gap**. L2: **Gap**. No Latch `.so` is committed. Remaining partner-checklist rows stay **Gap**.  
**Validation speech only** — never “we ship in Gazebo / Isaac / MuJoCo.”  
**Wrap, BM map, non-claims:** [`INTEGRATION.md`](INTEGRATION.md).

## Exists / Gap

The evidence for the earlier status check is not in this repository. No Latch `.so` is committed. `latch_mock.c` is the in-tree stand-in, not Latch. An external shared library may be supplied with `LATCH_LIB_DIR` or `LATCH_LIBRARY` and is not committed. When it is supplied, the harness links that file and calls it. `BM-01 Latch bit-match 60/60` is printed only when that run matches all 60 rows. With the library linked, `lbs_verify` and `lbs_soak` compare `LATCH_GOLDENS` and exit 0 only when both match all 60 rows. The in-tree fixture replay does not exit 0. BM-09, BM-10, and L2 may be **Exists** only for that match. A mock recompute does not mark them Exists. Public CI leaves both variables unset and runs the tracked file `fixtures/latch-robot-1.jsonl`.

| Item | Verdict | Note |
|------|---------|------|
| L0 spec, schemas, OSS spine | **Exists** | Docs lint is green on main. |
| Public-boundary lint | **Exists** | `scripts/lint_public_boundary.sh` is on main. The docs CI job runs it. |
| L1 MuJoCo C sources | **Exists** | `harness/mujoco`: ~1 kHz `mj_step`, `latch_consider` at 20 Hz, `latch_apply_reject` before the position setpoint. Mock oracle. |
| L1 public CI when sources are present | **Exists** | `.github/workflows/ci.yml` takes the cmake branch when sources are present. Mock-oracle recompute of `fixtures/latch-robot-1.jsonl`: **Exists**. BM-01 Latch bit-match: **Gap**. BM-09: **Gap**. BM-10: **Gap**. L2: **Gap**. |
| Mock recompute of `fixtures/latch-robot-1.jsonl` | **Exists** | Public mock recomputes its own 60-row fixture. BM-01 Latch bit-match: **Gap**. |
| BM-01..08 and BM-12 | **Exists** (mock-oracle) | Targeted by L1. Public PASS lines are not a Latch oracle certificate. Raw flip counts keep the previous name when a row sets reset, so an alternating raw sequence is not counted as zero. |
| BM-09 / BM-10 | **Gap** | Latch library is unset on public CI. With a library linked, verify and soak compare `LATCH_GOLDENS` and exit 0 only on a 60-row match. The in-tree fixture replay does not exit 0. **Exists** only for that match. A mock recompute does not mark them Exists. |
| L2 evidence verify + soak | **Gap** | Latch library is unset on public CI. **Exists** only when that same `LATCH_GOLDENS` verify and soak match all 60 rows. A mock recompute does not mark L2 Exists. |
| `lbs_node_wrap` | **Exists** (mock-oracle) | CI reported `lbs_node_wrap` Passed. It links the in-tree mock oracle. It is not a gz-sim launch. |
| L3 ROS 2 / gz FA launch | **Exists** | A gz-sim server launch ran headless for a fixed iteration count. The world has no Latch plugin. A separate CI workflow starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command without reading the mode. A separate CI workflow clones BehaviorTree.ROS2 at 72a3bf51dad332c67b99fc3373ccd5242f94f680 and ticks a read-only condition on /latch/mode_name. The condition does not elect and does not publish. Another workflow starts that mock controller and that condition in one job. A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs that condition in the same job. Approach on /latch/mode_name moves the sim joint through the position controller. Yield leaves that command. The partner launch **Exists** in this repository. It starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The evidence of those earlier one-joint Actions runs is not in this repository. Approach moves the sim joint. Yield leaves the setpoint. Latch is not linked. This is not a Latch certificate. Bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram stay **Gap** on that run. Mock-oracle recompute of `fixtures/latch-robot-1.jsonl`: **Exists**. BM-01 Latch bit-match: **Gap**. BM-09: **Gap**. BM-10: **Gap**. L2: **Gap**. |
| `scripts/fa_checklist.md` | **Exists** | `scripts/fa_checklist_run.sh` names the MuJoCo binaries (`lbs_bm`, `lbs_verify`, `lbs_soak`, `lbs_noalloc`). Mock-oracle recompute of `fixtures/latch-robot-1.jsonl`: **Exists**. BM-01 Latch bit-match: **Gap**. BM-09: **Gap**. BM-10: **Gap**. L2: **Gap**. If `build/ros2_gz/lbs_node_wrap` exists it runs that binary and prints that it is mock-oracle, not a gz-sim launch. If that binary is absent it prints that the node wrap binary was not built and the MuJoCo checklist continues. The headless gz-sim server is not the MuJoCo invocation. `scripts/fa_checklist_run.sh partner` runs `harness/ros2_gz/launch_partner.sh` and scores this list against that launch. The MuJoCo invocation does not start gz-sim. A separate CI workflow starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command without reading the mode. A separate CI workflow clones BehaviorTree.ROS2 at 72a3bf51dad332c67b99fc3373ccd5242f94f680 and ticks a read-only condition on /latch/mode_name. The condition does not elect and does not publish. Another workflow starts that mock controller and that condition in one job. A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs that condition in the same job. Approach on /latch/mode_name moves the sim joint through the position controller. Yield leaves that command. The partner launch **Exists** in this repository. It starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The evidence of those earlier one-joint Actions runs is not in this repository. Approach moves the sim joint. Yield leaves the setpoint. Latch is not linked. This is not a Latch certificate. Bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram stay **Gap** on that run. Mock-oracle recompute of `fixtures/latch-robot-1.jsonl`: **Exists**. BM-01 Latch bit-match: **Gap**. BM-09: **Gap**. BM-10: **Gap**. L2: **Gap**. |

**Next**

1. **Exists** (60-row golden) — `fixtures/latch-robot-1.jsonl` is tracked. Mock-oracle recompute of `fixtures/latch-robot-1.jsonl`: **Exists**. BM-01 Latch bit-match: **Gap**. BM-09: **Gap**. BM-10: **Gap**. L2: **Gap**. No Latch `.so` is committed.
2. **Gap** (partner checklist rows this launch does not run) — bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram. The partner launch **Exists** in this repository. It starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The evidence of those earlier one-joint Actions runs is not in this repository. Approach moves the sim joint. Yield leaves the setpoint. Latch is not linked. This is not a Latch certificate. Bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram stay **Gap** on that run. Mock-oracle recompute of `fixtures/latch-robot-1.jsonl`: **Exists**. BM-01 Latch bit-match: **Gap**. BM-09: **Gap**. BM-10: **Gap**. L2: **Gap**. `lbs_node_wrap` is not that launch. A **node** wraps Latch. A physics plugin does not own election.

## Dual path (locked)

| Role | Stack | Phase | Gate |
|------|-------|-------|------|
| **Primary CI truth** | MuJoCo C harness (`harness/mujoco`) — bit-match, chatter, latency, no-alloc | **L1 Must** | Yes |
| **Primary partner FA** | ROS 2 + gz-sim + gz_ros2_control + BT.ROS2 condition (`harness/ros2_gz`) | **L3 Must** | Yes |
| **Optional internal** | Isaac clip | **Stretch** (L5 / BM-S8) | No |

Sim Must paths are L1 and L3. Isaac is Stretch and never a gate.

## Phases

### L0 — Spec freeze (**Exists**)

- SPEC / NONGOALS / ADJACENCY / schemas stubs
- OSS community files (LICENSE Apache-2.0, NOTICE, CoC, SECURITY, CONTRIBUTING)
- Harness directory stubs at L0; CI docs-lint. The MuJoCo stub was replaced by L1 sources. `harness/ros2_gz` has a mock-oracle node wrap (`lbs_node_wrap` Passed). A gz-sim server launch ran headless for a fixed iteration count. The world has no Latch plugin. A separate CI workflow starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command without reading the mode. A separate CI workflow clones BehaviorTree.ROS2 at 72a3bf51dad332c67b99fc3373ccd5242f94f680 and ticks a read-only condition on /latch/mode_name. The condition does not elect and does not publish. Another workflow starts that mock controller and that condition in one job. A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs that condition in the same job. Approach on /latch/mode_name moves the sim joint through the position controller. Yield leaves that command. The partner launch **Exists** in this repository. It starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The evidence of those earlier one-joint Actions runs is not in this repository. Approach moves the sim joint. Yield leaves the setpoint. Latch is not linked. This is not a Latch certificate. Bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram stay **Gap** on that run. Mock-oracle recompute of `fixtures/latch-robot-1.jsonl`: **Exists**. BM-01 Latch bit-match: **Gap**. BM-09: **Gap**. BM-10: **Gap**. L2: **Gap**.
- **Exit:** BM IDs frozen; honesty rails copy-pasteable into FA. Met.

### L1 — MuJoCo harness CI (**Must**)

Primary CI truth. Headless C harness; no DDS in the gate.

**In this repository:** sources **Exist**. `fixtures/latch-robot-1.jsonl` is tracked. Mock-oracle recompute of `fixtures/latch-robot-1.jsonl`: **Exists**. BM-01 Latch bit-match: **Gap**. BM-01..08 and BM-12 are the L1 targets. BM-09: **Gap**. BM-10: **Gap**. L2: **Gap**.

- Two-rate loop (~1 kHz `mj_step` / 10–50 Hz `latch_consider`) — sources in `harness/mujoco`
- `latch_apply_reject` before setpoint consume
- mode→setpoint table (position only; **no torque path**)
- BM-01..08 and BM-12 are the L1 targets. Mock-oracle recompute of `fixtures/latch-robot-1.jsonl`: **Exists**. BM-01 Latch bit-match: **Gap**
- Private Latch `.so` via internal artifact store (never public cache, never in-tree)
- **Exit:** primary CI truth green on the reference box with that artifact

### L2 — Evidence verify + multi-process soak (**Gap**)

BM-09: **Gap**. BM-10: **Gap**. L2: **Gap**. Public CI does not link a Latch library.

- verify CLI → BM-09 **Gap**
- soak ≥2 processes → BM-10 **Gap**
- Optional BM-S6 (sense jitter) is not in this tree
- **Exit:** BM-09 **Gap**. BM-10 **Gap**. L2 **Gap**.

### L3 — ROS 2 / gz one-arm FA + BT condition (**Must**, launch **Exists**, other rows **Gap**)

Primary partner FA: ROS 2 + gz-sim + gz_ros2_control + BT.ROS2 condition.
`lbs_node_wrap` Passed in CI and links the mock oracle. A physics plugin does not own election. A gz-sim server launch ran headless for a fixed iteration count. The world has no Latch plugin. A separate CI workflow starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command without reading the mode. A separate CI workflow clones BehaviorTree.ROS2 at 72a3bf51dad332c67b99fc3373ccd5242f94f680 and ticks a read-only condition on /latch/mode_name. The condition does not elect and does not publish. Another workflow starts that mock controller and that condition in one job. A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs that condition in the same job. Approach on /latch/mode_name moves the sim joint through the position controller. Yield leaves that command. The partner launch **Exists** in this repository. It starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The evidence of those earlier one-joint Actions runs is not in this repository. Approach moves the sim joint. Yield leaves the setpoint. Latch is not linked. This is not a Latch certificate. Bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram stay **Gap** on that run. Mock-oracle recompute of `fixtures/latch-robot-1.jsonl`: **Exists**. BM-01 Latch bit-match: **Gap**. BM-09: **Gap**. BM-10: **Gap**. L2: **Gap**.

- `latch_consider` @ ~20 Hz; `latch_apply_reject` before controller consume
- Existing ros2_control maps name → behavior; CM keeps lifecycle
- Independent `/emergency_stop` (or gz plug) → BM-07 on topics
- BT.ROS2 Condition read-only → BM-11
- FA launch + checklist spine (`scripts/fa_checklist.md`)
- **Exit:** partner-facing path runnable. The partner launch **Exists**. The other checklist rows stay **Gap** on that partner run. Mock-oracle recompute of `fixtures/latch-robot-1.jsonl`: **Exists**. BM-01 Latch bit-match: **Gap**. BM-09: **Gap**. BM-10: **Gap**. L2: **Gap**.

### L4 — Partner acceptance pack

- Runbook + artifact bundle under NDA share path
- Acceptance signature slot
- **Exit:** partner acceptance pack (validation path)

### L5 — Stretch (after Must green)

MCAP export (BM-S4), informative AAT map (BM-S5), BM-S1/S2 override rails.
**Isaac** internal yield≠stop clip (BM-S8) is Stretch only:
cinematic / sensor-rich, and it does not block L1, L3, or first partner open.
Packaging a Kit-bundled turnkey needs a separate NVIDIA license check.

## Latch tree boundary

Proprietary Latch keeps ABI headers, native core, and oracle goldens.
This repo **consumes** ABI via an optional external library, never vendored in this tree.

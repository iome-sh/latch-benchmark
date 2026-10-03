# ROADMAP — Latch Benchmark Suite (L0–L4)

**Status (2026-10-03, tip `beef75f8ced96163c44aaca18fd9038352a96da1`):** L0 **Exists**. L1 mock-oracle **Exists**. Public-boundary lint **Exists**. L2 verify and soak stay **Gap**. BM-09 and BM-10 stay **Gap**. The 60-row golden on public CI stays **Gap**. The optional BM-01 step does not claim the 60-row golden and skips configure. `lbs_node_wrap` Passed. A gz-sim server launch ran headless for a fixed iteration count. The world has no Latch plugin. A separate CI workflow starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command without reading the mode. A separate CI workflow clones BehaviorTree.ROS2 at 72a3bf51dad332c67b99fc3373ccd5242f94f680 and ticks a read-only condition on /latch/mode_name. The condition does not elect and does not publish. Another workflow starts that mock controller and that condition in one job. A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs that condition in the same job. Approach on /latch/mode_name moves the sim joint through the position controller. Yield leaves that command. The partner launch **Exists** on this tip. It starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The pull-request one-joint job 111175592122 (Actions run 37113438572) on head `2117880edc7f69367083ff607267c1bb777df9bb` and the post-merge one-joint job 111195532468 (Actions run 37120511712) on `2fba664eb8dccd1e735ddd6f7fd9d49386d34b52` both succeeded and printed `gz-sim launched` via `harness/ros2_gz/launch/partner_fa.launch.py`. Approach moves the sim joint. Yield leaves the setpoint. Latch is not linked. This is not a Latch certificate. Bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram stay **Gap** on that run. The 60-row golden on public CI stays **Gap**. BM-09 and BM-10 stay **Gap**. No Latch `.so` is committed. **Gap:** the 60-row golden through the bench harness (external library and goldens are not present on public CI). Remaining partner-checklist rows stay **Gap**.  
**Validation speech only** — never “we ship in Gazebo / Isaac / MuJoCo.”  
**Wrap, BM map, non-claims:** [`INTEGRATION.md`](INTEGRATION.md).

## Exists / Gap

Checked against tip `beef75f8ced96163c44aaca18fd9038352a96da1`. No Latch `.so` is committed. `latch_mock.c` is the in-tree stand-in, not Latch. A private `liblatch` is optional via `find_library` (`LATCH_LIB_DIR` / `LATCH_LIBRARY`) and is not committed. Public CI leaves `LATCH_LIB_DIR`, `LATCH_LIBRARY`, and `LATCH_GOLDENS` unset. The optional BM-01 step does not claim the 60-row golden and skips configure.

| Item | Verdict | Note |
|------|---------|------|
| L0 spec, schemas, OSS spine | **Exists** | Docs lint is green on main. |
| Public-boundary lint | **Exists** | `scripts/lint_public_boundary.sh` is on main. The docs CI job runs it. |
| L1 MuJoCo C sources | **Exists** | `harness/mujoco`: ~1 kHz `mj_step`, `latch_consider` at 20 Hz, `latch_apply_reject` before the position setpoint. Mock oracle. |
| L1 public CI when sources are present | **Exists** | `.github/workflows/ci.yml` takes the cmake branch when sources are present. BM-09 and BM-10 stay **Gap**. The 60-row golden on public CI stays **Gap**. |
| 60-row golden on public CI | **Gap** | The 60-row golden on public CI stays **Gap**. External library and goldens are not present on public CI. The optional BM-01 step does not claim the 60-row golden and skips configure. `scripts/w2_optional_artifact.sh` builds and runs `lbs_bm` only when a library path and `LATCH_GOLDENS` are both set. |
| BM-01..08 and BM-12 | **Exists** (mock-oracle) | Targeted by L1. Public PASS lines are not a Latch oracle certificate. Raw flip counts keep the previous name when a row sets reset, so an alternating raw sequence is not counted as zero. |
| BM-09 / BM-10 | **Gap** | BM-09 and BM-10 stay **Gap**. This is not a Latch certificate. |
| L2 evidence verify + soak | **Gap** | BM-09 and BM-10 stay **Gap**. This is not a Latch certificate. |
| `lbs_node_wrap` | **Exists** (mock-oracle) | CI reported `lbs_node_wrap` Passed. It links the in-tree mock oracle. It is not a gz-sim launch. |
| L3 ROS 2 / gz FA launch | **Exists** | A gz-sim server launch ran headless for a fixed iteration count. The world has no Latch plugin. A separate CI workflow starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command without reading the mode. A separate CI workflow clones BehaviorTree.ROS2 at 72a3bf51dad332c67b99fc3373ccd5242f94f680 and ticks a read-only condition on /latch/mode_name. The condition does not elect and does not publish. Another workflow starts that mock controller and that condition in one job. A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs that condition in the same job. Approach on /latch/mode_name moves the sim joint through the position controller. Yield leaves that command. The partner launch **Exists** on this tip. It starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The pull-request one-joint job 111175592122 (Actions run 37113438572) on head `2117880edc7f69367083ff607267c1bb777df9bb` and the post-merge one-joint job 111195532468 (Actions run 37120511712) on `2fba664eb8dccd1e735ddd6f7fd9d49386d34b52` both succeeded and printed `gz-sim launched` via `harness/ros2_gz/launch/partner_fa.launch.py`. Approach moves the sim joint. Yield leaves the setpoint. Latch is not linked. This is not a Latch certificate. Bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram stay **Gap** on that run. The 60-row golden on public CI stays **Gap**. BM-09 and BM-10 stay **Gap**. |
| `scripts/fa_checklist.md` | **Exists** | `scripts/fa_checklist_run.sh` names the MuJoCo binaries (`lbs_bm`, `lbs_verify`, `lbs_soak`, `lbs_noalloc`). BM-09 and BM-10 stay **Gap**. The 60-row golden on public CI stays **Gap**. If `build/ros2_gz/lbs_node_wrap` exists it runs that binary and prints that it is mock-oracle, not a gz-sim launch. If that binary is absent it prints that the node wrap binary was not built and the MuJoCo checklist continues. The headless gz-sim server is not the MuJoCo invocation. `scripts/fa_checklist_run.sh partner` runs `harness/ros2_gz/launch_partner.sh` and scores this list against that launch. The MuJoCo invocation does not start gz-sim. A separate CI workflow starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command without reading the mode. A separate CI workflow clones BehaviorTree.ROS2 at 72a3bf51dad332c67b99fc3373ccd5242f94f680 and ticks a read-only condition on /latch/mode_name. The condition does not elect and does not publish. Another workflow starts that mock controller and that condition in one job. A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs that condition in the same job. Approach on /latch/mode_name moves the sim joint through the position controller. Yield leaves that command. The partner launch **Exists** on this tip. It starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The pull-request one-joint job 111175592122 (Actions run 37113438572) on head `2117880edc7f69367083ff607267c1bb777df9bb` and the post-merge one-joint job 111195532468 (Actions run 37120511712) on `2fba664eb8dccd1e735ddd6f7fd9d49386d34b52` both succeeded and printed `gz-sim launched` via `harness/ros2_gz/launch/partner_fa.launch.py`. Approach moves the sim joint. Yield leaves the setpoint. Latch is not linked. This is not a Latch certificate. Bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram stay **Gap** on that run. The 60-row golden on public CI stays **Gap**. BM-09 and BM-10 stay **Gap**. |

**Next**

1. **Gap** (60-row golden) — external library and goldens are not present on public CI. `scripts/w2_optional_artifact.sh` builds and runs `lbs_bm` only when a library path and `LATCH_GOLDENS` are both set. It configures `harness/mujoco` (`-DLATCH_LIBRARY` when `LATCH_LIBRARY` is a file, otherwise `-DLATCH_LIB_DIR`) and does not claim success before the binary prints the bit-match line. It does not fetch a repository. On public CI both are unset, so the step does not claim the 60-row golden and skips configure. Never an in-tree `.so` and never a public Actions cache. No Latch `.so` is committed. The 60-row golden on public CI stays **Gap**. BM-09 and BM-10 stay **Gap**.
2. **Gap** (partner checklist rows this launch does not run) — bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram. The partner launch **Exists** on this tip. It starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The pull-request one-joint job 111175592122 (Actions run 37113438572) on head `2117880edc7f69367083ff607267c1bb777df9bb` and the post-merge one-joint job 111195532468 (Actions run 37120511712) on `2fba664eb8dccd1e735ddd6f7fd9d49386d34b52` both succeeded and printed `gz-sim launched` via `harness/ros2_gz/launch/partner_fa.launch.py`. Approach moves the sim joint. Yield leaves the setpoint. Latch is not linked. This is not a Latch certificate. Bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram stay **Gap** on that run. The 60-row golden on public CI stays **Gap**. BM-09 and BM-10 stay **Gap**. `lbs_node_wrap` is not that launch. A **node** wraps Latch. A physics plugin does not own election.

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
- Harness directory stubs at L0; CI docs-lint. The MuJoCo stub was replaced by L1 sources. `harness/ros2_gz` has a mock-oracle node wrap (`lbs_node_wrap` Passed). A gz-sim server launch ran headless for a fixed iteration count. The world has no Latch plugin. A separate CI workflow starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command without reading the mode. A separate CI workflow clones BehaviorTree.ROS2 at 72a3bf51dad332c67b99fc3373ccd5242f94f680 and ticks a read-only condition on /latch/mode_name. The condition does not elect and does not publish. Another workflow starts that mock controller and that condition in one job. A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs that condition in the same job. Approach on /latch/mode_name moves the sim joint through the position controller. Yield leaves that command. The partner launch **Exists** on this tip. It starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The pull-request one-joint job 111175592122 (Actions run 37113438572) on head `2117880edc7f69367083ff607267c1bb777df9bb` and the post-merge one-joint job 111195532468 (Actions run 37120511712) on `2fba664eb8dccd1e735ddd6f7fd9d49386d34b52` both succeeded and printed `gz-sim launched` via `harness/ros2_gz/launch/partner_fa.launch.py`. Approach moves the sim joint. Yield leaves the setpoint. Latch is not linked. This is not a Latch certificate. Bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram stay **Gap** on that run. The 60-row golden on public CI stays **Gap**. BM-09 and BM-10 stay **Gap**.
- **Exit:** BM IDs frozen; honesty rails copy-pasteable into FA. Met.

### L1 — MuJoCo harness CI (**Must**)

Primary CI truth. Headless C harness; no DDS in the gate.

**At this tip (`beef75f8ced96163c44aaca18fd9038352a96da1`):** sources **Exist**. The 60-row golden on public CI stays **Gap**. External library and goldens are not present on public CI. The optional BM-01 step does not claim the 60-row golden and skips configure. BM-01..08 and BM-12 are the L1 targets. BM-09 and BM-10 stay **Gap**.

- Two-rate loop (~1 kHz `mj_step` / 10–50 Hz `latch_consider`) — sources in `harness/mujoco`
- `latch_apply_reject` before setpoint consume
- mode→setpoint table (position only; **no torque path**)
- BM-01..08 and BM-12 are the L1 targets. The 60-row golden on public CI stays **Gap**
- Private Latch `.so` via internal artifact store (never public cache, never in-tree)
- **Exit:** primary CI truth green on the reference box with that artifact

### L2 — Evidence verify + multi-process soak (**Gap**)

BM-09 and BM-10 stay **Gap** on public CI. This is not a Latch certificate.

- verify CLI → BM-09 stays **Gap**
- soak ≥2 processes → BM-10 stays **Gap**
- Optional BM-S6 (sense jitter) is not in this tree
- **Exit:** forensic speech without an AAT badge. BM-09 and BM-10 stay **Gap**. This is not a Latch certificate.

### L3 — ROS 2 / gz one-arm FA + BT condition (**Must**, launch **Exists**, other rows **Gap**)

Primary partner FA: ROS 2 + gz-sim + gz_ros2_control + BT.ROS2 condition.
`lbs_node_wrap` Passed in CI and links the mock oracle. A physics plugin does not own election. A gz-sim server launch ran headless for a fixed iteration count. The world has no Latch plugin. A separate CI workflow starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command without reading the mode. A separate CI workflow clones BehaviorTree.ROS2 at 72a3bf51dad332c67b99fc3373ccd5242f94f680 and ticks a read-only condition on /latch/mode_name. The condition does not elect and does not publish. Another workflow starts that mock controller and that condition in one job. A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs that condition in the same job. Approach on /latch/mode_name moves the sim joint through the position controller. Yield leaves that command. The partner launch **Exists** on this tip. It starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The pull-request one-joint job 111175592122 (Actions run 37113438572) on head `2117880edc7f69367083ff607267c1bb777df9bb` and the post-merge one-joint job 111195532468 (Actions run 37120511712) on `2fba664eb8dccd1e735ddd6f7fd9d49386d34b52` both succeeded and printed `gz-sim launched` via `harness/ros2_gz/launch/partner_fa.launch.py`. Approach moves the sim joint. Yield leaves the setpoint. Latch is not linked. This is not a Latch certificate. Bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram stay **Gap** on that run. The 60-row golden on public CI stays **Gap**. BM-09 and BM-10 stay **Gap**.

- `latch_consider` @ ~20 Hz; `latch_apply_reject` before controller consume
- Existing ros2_control maps name → behavior; CM keeps lifecycle
- Independent `/emergency_stop` (or gz plug) → BM-07 on topics
- BT.ROS2 Condition read-only → BM-11
- FA launch + checklist spine (`scripts/fa_checklist.md`)
- **Exit:** partner-facing path runnable. The partner launch **Exists**. The other checklist rows stay **Gap**. BM-09 and BM-10 stay **Gap**. The 60-row golden on public CI stays **Gap**.

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

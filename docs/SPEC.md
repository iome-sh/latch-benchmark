# LBS SPEC v0 — Metric definitions (BM-01..12)

**Status:**

**Exists.** L0 frozen (**Exists**). **Implementation:** MuJoCo L1 harness **Exists** as mock-oracle.

MuJoCo CI ran `lbs_verify` and `lbs_soak`, and the log says this is not a Latch certificate. The same log still printed `not Latch 60/60 bit-match`. The optional BM-01 step printed `BM-01 60/60 not claimed` and skipped configure. `lbs_node_wrap` Passed.

A gz-sim server launch ran headless for a fixed iteration count. The world has no Latch plugin. A separate CI workflow starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command without reading the mode.

A separate CI workflow clones BehaviorTree.ROS2 at 72a3bf51dad332c67b99fc3373ccd5242f94f680 and ticks a read-only condition on /latch/mode_name. The condition does not elect and does not publish. Another workflow starts that mock controller and that condition in one job.

**One-joint partner launch.** A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs that condition in the same job. Approach on /latch/mode_name moves the sim joint through the position controller. Yield leaves that command.

The one-joint gz job runs that mock-oracle process with --drive-sim. Approach moves the sim joint. E-stop elects yield and leaves the command. It is not Latch. Latch is not linked. The empty world does not elect a mode.

The partner checklist runs that one-joint path. `scripts/fa_checklist_run.sh partner` starts `harness/ros2_gz/launch_partner.sh`, which starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The log prints `gz-sim launched` or `gz-sim did not start`. Yield leaves the setpoint. The partner launch **Exists** as mock-oracle. It is not a Latch certificate.

**Gap.** L2 verify and soak stay **Gap**. BM-09 and BM-10 stay **Gap**. Bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram stay **Gap** on that run. **Gap:** the 60-row golden through the bench harness (external library and goldens are not present on public CI). The only committed rows are the 10-row mock oracle, which is not that golden and not an exported session. Remaining partner-checklist rows stay **Gap**. No Latch `.so` is committed.

**Suite:** Latch Benchmark Suite · Apache-2.0  
**Atom:** in-process election of one of six modes for an *existing* controller;
signed mode record; no torque/traj; no hosted model on tick; no alloc on tick;
PLC/e-stop in series; `yield` = leave setpoint ≠ protective stop / monitored standstill.

This SPEC defines **invent-fresh** Must metrics for signed mode-election quality.
Partial adjacents are cited for buyer speech only — see [`ADJACENCY.md`](ADJACENCY.md).
Miss KPIs are banned — see [`NONGOALS.md`](NONGOALS.md).

**Honesty:** BM IDs BM-01..12 are frozen after L0. Do not renumber without
maintainer approval. Pass rules below are design targets; harness code that
implements them lands in L1–L4 (see [`ROADMAP.md`](ROADMAP.md)). Which harness
may speak for each ID: [`INTEGRATION.md`](INTEGRATION.md) and the table below.

---

## Must metrics

| ID | Metric | Pass rule (v0) | Phase gate | Demo speech |
|----|--------|----------------|------------|-------------|
| **BM-01** | Golden bit-match | 60/60 rows: name + score + margin + dwell + plane + evidence hex | L1 | “Oracle frozen.” |
| **BM-02** | Chatter damp | Chatter fixture: Latch mode flips ≤ 3 (dwell); raw argmax flips ≥ 11 (or documented N); evidence stable across run | L1 | “Noise does not thrash the name.” |
| **BM-03** | Stale → soft yield | `sense_age ≥ 4` ⇒ yield, plane `l2` (not `deny`) | L1 | “Stale is soft, not e-stop theater.” |
| **BM-04** | Deny planes | e-stop / collision / joint limit ⇒ yield, plane `deny`; dwell cleared | L1 | “Hard denials named.” |
| **BM-05** | Budget keep | Over budget ⇒ last legal or hold, plane `budget`; plant stub tick not stalled (see BM-08) | L1 | “Budget never freezes the servo.” |
| **BM-06** | Apply-time reject | After consider, precondition dies ⇒ hold, plane `reject` via apply-reject path | L1 | “Late death → hold, not stale name.” |
| **BM-07** | Yield ≠ stop series | Yield leaves setpoint in plant stub; independent stop inject still trips plant; glossary shown | L1–L3 | “Yield is leave-setpoint; PLC stays in series.” |
| **BM-08** | Consider latency | Report p50/p99/max consider μs under soak; **Must:** max &lt; 1000 μs on reference box (tune per partner HW); budget path does not block plant stub tick | L1 | “Decision plane fits under the servo.” |
| **BM-09** | Evidence replay | Session JSONL + verify CLI: 100% hash recompute match | L2 | “Why this mode then?” |
| **BM-10** | Multi-process soak | ≥2 processes, same blob + same sense stream ⇒ identical modes + evidence | L2 | “Determinism clip.” |
| **BM-11** | No-torque / not-BT / not-CM | Binding publishes mode-record fields only; BT kit is read-only condition; no activate/deactivate API as Latch product | L3 | Honesty exits baked into FA |
| **BM-12** | Illegal emission = 0 | No illegal mode under mask; property held in soak | L1 | Contract integrity |

Goldens / oracle bit-match for BM-01 live in the proprietary Latch tree; this
repo consumes exported fixtures or CI artifacts — never ships Latch source.

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

## Stretch metrics (labeled; not required to open first partner)

| ID | Metric | Phase | Note |
|----|--------|-------|------|
| **BM-S1** | Override rate rail | L5 | `rejected_elections / elections`; caption: *not contact-success* |
| **BM-S2** | Cancel-log completeness | L5 | Row: mode, score, margin, snapshot, plane, evidence, override flag |
| **BM-S3** | Seventh-mode-via-snapshot | L5 | New bit + rank + behavior without tick rewrite |
| **BM-S4** | MCAP evidence export | L5 | Packaging adjacency to Foxglove; not sealed forensics |
| **BM-S5** | Informative AAT field map | L5 | Docs table only; **no** conformance badge |
| **BM-S6** | Dual-process soak under sense jitter | L5 | Hardens BM-10 |
| **BM-S7** | Asymmetric enter/exit margins | L5 | Only if fixture demands |
| **BM-S8** | Isaac internal yield≠stop clip | Stretch | Internal R&D video only. Not a CI gate. See [`INTEGRATION.md`](INTEGRATION.md). |

BM-S1, BM-S2, and BM-S3 enter FA only after a cancel-log schema exists. Do not fake them early.

---

## Improvement loop (Δ card)

Releases that claim “we got better” should report Δ vs prior LBS baseline:

| Signal | Better direction | Gate |
|--------|------------------|------|
| Δ flip ratio (raw / Latch) on chatter fixture | ↑ ratio or ↓ Latch flips | BM-02 |
| Δ p99 consider μs | ↓; max stays &lt; 1000 μs on ref box | BM-08 |
| illegal = 0 | Must remain 0 | BM-12 |
| soak N | N ≥ 2; raise over time | BM-10 |
| verify match % | Must stay 100% | BM-09 |
| BM-07 series | Pass/fail; never softened | BM-07 |

No Miss KPIs on the Δ card.

---

## Schemas

See [`../schemas/`](../schemas/) for evidence-v2, mode-record, and golden-jsonl
field notes (stubs until eng).

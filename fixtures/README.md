# Fixtures

How goldens work in LBS — **no Latch binary here.**

## Source of truth

BM-01 compares the harness with the tracked 60-row file
[`latch-robot-1.jsonl`](latch-robot-1.jsonl). Each line is one JSON object.
`snapshot` is the blob id. `grasped` is a sense input on the row.

The harness-local mock fixture is
[`../harness/mujoco/fixtures/mock-oracle.jsonl.example`](../harness/mujoco/fixtures/mock-oracle.jsonl.example)
(10 data rows). It is not the 60-row golden and it is not a session export.
Schema: [`../schemas/golden-jsonl.md`](../schemas/golden-jsonl.md).

## In this tree

Public CI runs the harness on `fixtures/latch-robot-1.jsonl`. It prints
`BM-01 Latch bit-match 60/60` only when that run matches all 60 rows.
Otherwise it prints `BM-01 60/60 not claimed`.

Verify and soak recompute that same file. The current public run matches all
60 rows, and both tools exit 0. The log still says this is not a Latch
evidence-v2 certificate. When a recompute fails, BM-09 and BM-10 stay **Gap**.

## Rules

- Never commit `liblatch*`, `.so`, or Latch source under `fixtures/`
- Never put customer logs or proprietary session packs in the public-future tree
- Example / synthetic rows may use `*.jsonl.example` (gitignored patterns allow)

## Linking Latch

Optional external library, never vendored in this tree.
Public Actions must not cache Latch artifacts.

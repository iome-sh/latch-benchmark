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

Mock-oracle recompute of `fixtures/latch-robot-1.jsonl`: **Exists**.
BM-01 Latch bit-match: **Gap**. BM-09: **Gap**. BM-10: **Gap**. L2: **Gap**.

## Rules

- Never commit `liblatch*`, `.so`, or Latch source under `fixtures/`
- Never put customer logs or proprietary session packs in the public-future tree
- Example / synthetic rows may use `*.jsonl.example` (gitignored patterns allow)

## Linking Latch

Optional external library, never vendored in this tree.
Public Actions must not cache Latch artifacts.
When `LATCH_LIBRARY` or `LATCH_LIB_DIR` is set, the harness links that shared library and calls it. `BM-01 Latch bit-match 60/60` is printed only when that run matches all 60 rows. Public CI leaves the library unset, so BM-09, BM-10, and L2 stay **Gap**.

# Fixtures

How goldens work in LBS — **no Latch binary here.**

## Source of truth

Oracle goldens (60-row JSONL bit-match) live in the proprietary Latch
repository. This suite:

1. Documents the schema ([`../schemas/golden-jsonl.md`](../schemas/golden-jsonl.md))
2. Will accept **exported** golden snapshots or CI-fetched artifacts for BM-01
3. Holds a harness-local mock fixture at `harness/mujoco/fixtures/mock-oracle.jsonl.example` (not the 60-row Latch oracle)

## In this tree

The only committed rows are `harness/mujoco/fixtures/mock-oracle.jsonl.example`
(10 data rows). That file is the mock oracle. It is not an exported 60-row
golden. It is not an exported session. Public CI scans tracked `*.jsonl`
files. There are none. The step prints `BM-01 60/60 not claimed` and leaves
BM-09 and BM-10 as **Gap**. A later tracked 60-row export is run through the
existing harness. The not-claimed line stays unless that run prints
`BM-01 Latch bit-match 60/60`. A later tracked session export is replayed
and soaked only when it carries one `snapshot_id`. This is not a Latch
certificate.

## Rules

- Never commit `liblatch*`, `.so`, or Latch source under `fixtures/`
- Never put customer logs or proprietary session packs in the public-future tree
- Example / synthetic rows may use `*.jsonl.example` (gitignored patterns allow)

## Linking Latch

Optional external library, never vendored in this tree.
Public Actions must not cache Latch artifacts.

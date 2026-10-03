# Fixtures

How goldens work in LBS — **no Latch binary here.**

## Source of truth

Oracle goldens (60-row JSONL bit-match) live in the proprietary Latch
repository. This suite:

1. Documents the schema ([`../schemas/golden-jsonl.md`](../schemas/golden-jsonl.md))
2. Will accept **exported** golden snapshots or CI-fetched artifacts for BM-01
3. Holds a harness-local mock fixture at `harness/mujoco/fixtures/mock-oracle.jsonl.example` (not the 60-row Latch oracle)

## Rules

- Never commit `liblatch*`, `.so`, or Latch source under `fixtures/`
- Never put customer logs or proprietary session packs in the public-future tree
- Example / synthetic rows may use `*.jsonl.example` (gitignored patterns allow)

## Linking Latch

Optional external library, never vendored in this tree.
Public Actions must not cache Latch artifacts.

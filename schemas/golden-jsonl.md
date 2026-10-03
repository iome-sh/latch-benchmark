# Schema — golden JSONL (stub)

**Status:** documentation stub · oracle goldens live in Latch tree; LBS consumes exports.

## Intent

One JSON object per line. BM-01 requires bit-match of name, score, margin,
dwell, plane, and evidence against the frozen external oracle (60 rows).

## Row shape (informative)

```json
{"id":"...","sense":{...},"name":"...","score":0,"margin":0,"dwell":0,"plane":"...","evidence_hex":"0123456789abcdef"}
```

Exact keys follow Latch golden exporter. Session JSONL for BM-09 adds run
metadata and may include sense stream references for soak (BM-10).

## Fixtures in this repo

See [`../fixtures/README.md`](../fixtures/README.md). Do not commit Latch
binaries or proprietary `.so` beside goldens.

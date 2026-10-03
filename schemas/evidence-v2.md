# Schema — evidence v2 (stub)

**Status:** documentation stub · wire format owned by Latch core; LBS verifies.

## Intent

Evidence v2 is a fixed-width hex digest (16 hex chars of SHA-256 over an
`ev2|…` payload) bound to a mode record + sense inputs. LBS **BM-09** requires
offline recompute match at 100%.

## Fields (informative)

| Field | Notes |
|-------|-------|
| `evidence_hex` | 16-hex digest string |
| Payload inputs | sense + mode-record fields as defined by Latch ABI / oracle |
| Algorithm | SHA-256 truncated/encoded per Latch evidence-v2 contract |

Exact payload concatenation lives with Latch ABI docs. This repo’s verify CLI
(L2) must bit-match that contract — do not invent a divergent hash here.

## Non-claims

- Not “legally sealed” / not AAT-conformant by itself
- Session JSONL may later add `prev_hash` chaining as Stretch (informative)

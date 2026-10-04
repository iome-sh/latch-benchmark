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

The tracked file `fixtures/latch-robot-1.jsonl` matches this concatenation.
The digest is the first 16 hex characters of SHA-256 over one line, with
decimal integers and no spaces:

`ev2|{snapshot}|{name}|{score}|{margin}|{dwell}|{plane}|{joint}|{limit}|{collision}|{estop}|{contact}|{grasped}|{target_seen}|{sense_age}|{wrench}`

`grasped` is a sense input in that line. Apply rows hash the apply-time sense.
The public mock matches that line for `fixtures/latch-robot-1.jsonl`: **Exists**.
Snapshot `lbs-mock-oracle` keeps its labeled mock digest.
BM-09: **Gap**. L2: **Gap**. When `LATCH_LIBRARY` or `LATCH_LIB_DIR` is set, verify and soak call that library. They report a match only when the compare succeeds. Public CI leaves the library unset.

## Non-claims

- Not “legally sealed” / not AAT-conformant by itself
- Session JSONL may later add `prev_hash` chaining as Stretch (informative)

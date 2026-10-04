# Mode record

This page is a documentation stub. The binding or the harness publishes the record. Latch elects the mode.

## Intent

A mode record is the signed election output consumed by existing controllers
and by LBS fixtures. Latch publishes **name + quality fields only** — no
torque, no trajectory, no controller activate/deactivate.

## Expected fields (informative)

| Field | Role |
|-------|------|
| `name` | One of six modes (approach / grasp / release / retract / yield / hold — exact enum from Latch ABI) |
| `score` | Election score |
| `margin` | Margin vs runner-up |
| `dwell` | Dwell / chatter state |
| `plane` | Plane label (`l2`, `deny`, `budget`, `reject`, …) |
| `evidence_hex` | evidence v2 digest |

## Binding rules (BM-11)

- Publish mode-record fields only
- Map `name` onto the controller's existing behavior table. The outputs are position setpoints.
- BT kits **read** the name; they do not own election
- CM / lifecycle remains outside Latch

# Measures this suite does not use

These measures describe a different product. Do not put them in partner scripts, CI gates, slides, or the improvement table.

## Banned metrics

- Grasp / pick / place success %, GraspNet AP
- MetaWorld / RLBench / ManiSkill / robosuite / OpenVLA (or similar) task success
- ISO / SIL / PLd **certification** as Latch; stop distance/time as Latch KPI
- Monitored standstill / protective stop **as Latch** (ISO 10218 naming contrast
  is OK in glossary speech — Latch is **not** that function)
- Contact-success % from hand rank
- “WCET certified” / hard-RT proof without partner-owned analysis
- BT tree coverage / Groot2 Pro feature parity / “Groot2-compatible”
- “AAT-conformant” / “EU AI Act compliant logging”
- “We ship in Gazebo / Isaac / MuJoCo” as a product claim
- FlexBE replacement / mission-edit UX / OCS creep

## Words that do not belong in partner copy

Do not write "safe yield", "ISO-aligned stop", or "certified WCET". Do not claim SIL or PLd for Latch. Do not describe Latch as a protective stop. Do not use internal ticket-tracker product names.

## What we measure instead

Signed mode-election quality: BM-01..12 in [`SPEC.md`](SPEC.md).

# FA checklist spine (partner FA)

Partner path is **ROS 2 + gz-sim + gz_ros2_control + BT.ROS2 condition**
(L3 Must). CI truth for the same metrics is the **MuJoCo C harness**
(L1 Must: bit-match, chatter, latency, no-alloc). Isaac is Stretch and is
not on this checklist.

[`fa_checklist_run.sh`](fa_checklist_run.sh) with no arguments is the MuJoCo path (mock-oracle harness binaries).
The MuJoCo CI job runs that script after the build. That invocation does not start gz-sim.

[`fa_checklist_run.sh partner`](fa_checklist_run.sh) is the partner run. It starts
[`../harness/ros2_gz/launch_partner.sh`](../harness/ros2_gz/launch_partner.sh), which starts gz-sim through
[`../harness/ros2_gz/launch/partner_fa.launch.py`](../harness/ros2_gz/launch/partner_fa.launch.py)
and then the existing one-joint path: position controller, mode mapper, read-only condition, and the mock-oracle drive.
Approach moves the sim joint. Yield leaves the setpoint. The log prints `gz-sim launched` or `gz-sim did not start`.
This run is mock-oracle. It is not a Latch certificate. Latch is not linked.

On that run, the one-arm gz binding, yield leaving the setpoint, and the read-only condition are what the log shows.
Bit-match, the live evidence path, chatter, stale, deny, budget, reject, evidence replay, soak, the operator signature, and the latency histogram stay **Gap**.

Wrap pattern (`latch_consider` / `latch_apply_reject`), BM→harness map, and
honesty non-claims: [`docs/INTEGRATION.md`](../docs/INTEGRATION.md).

Map to LBS Must BM-01..12. Operator signs after run. **Validation path only.**

1. Bit-match goldens (BM-01)
2. One-arm sim binding 10–50 Hz + live evidence path (BM-09 live)
3. Chatter / stale / deny / budget / reject demos (BM-02..06)
4. Yield ≠ stop series + glossary vs monitored standstill (BM-07)
5. No-torque / not-BT / not-CM (BM-11)
6. BT-condition reader sees one name
7. Evidence replay verify CLI (BM-09)
8. Multi-process soak clip (BM-10)
9. Acceptance checklist signed by operator (this pack)
10. Latency histogram artifact (BM-08)

## Captions (mandatory)

- Not contact-success / grasp-success
- Yield is leave-setpoint; PLC / e-stop stays in series
- Not SIL / ISO certification; not a BehaviorTree product
- Validation harness — not “we ship in Gazebo / Isaac”

BM-S1, BM-S2, and BM-S3 enter only after a cancel-log schema exists.

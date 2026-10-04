# Partner checklist

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
This run is mock-oracle. Latch is not linked.

On that run, the one-arm gz binding, yield leaving the setpoint, and the read-only condition are what the log shows.
Bit-match, the live evidence path, chatter, stale sense, deny, budget, reject, evidence replay, soak, the operator signature, and the latency histogram stay Gap.

How `latch_consider` and `latch_apply_reject` are called, which harness covers each metric, and what this suite does not claim: [`docs/INTEGRATION.md`](../docs/INTEGRATION.md).

Map to LBS Must BM-01..12. Operator signs after run. **Validation path only.**

1. Bit-match against the golden file (BM-01).
2. One arm in simulation, calling Latch at 10 to 50 Hz, with a live evidence path (BM-09).
3. Chatter, stale sense, deny, budget, and apply-time reject (BM-02 through BM-06).
4. Yield leaves the setpoint, and a separate stop still trips (BM-07). Yield is not a protective stop and not a rated stop.
5. The binding does not command torque, does not run a behavior tree, and does not activate controllers (BM-11).
6. The behavior-tree condition reads one mode name.
7. The verify tool recomputes a recorded session (BM-09).
8. Two processes produce the same result (BM-10).
9. An operator signs this checklist.
10. A latency histogram is saved (BM-08).

## What to say with the results

- This is not a contact-success score and not a grasp-success score.
- Yield means the mode leaves the setpoint. It is not a protective stop and not a rated stop. The separate stop still trips.
- This is not a SIL or ISO pack, and it is not a behavior tree.
- This is a validation harness. It is not a claim that the product ships inside Gazebo or Isaac.

BM-S1, BM-S2, and BM-S3 enter only after a cancel-log schema exists.

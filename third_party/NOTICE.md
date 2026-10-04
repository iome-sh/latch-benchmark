# Third-party notices

MuJoCo is fetched at configure time for the L1 harness. It is not committed.
The partner launch Exists as the mock oracle. The checklist rows that launch does not run are Gap. gz-sim is not vendored.

## Fetched at build (not vendored)

| Component | License | Role in LBS |
|-----------|---------|-------------|
| MuJoCo 3.14.0 linux-x86_64 release | Apache-2.0 | L1 headless CI harness (`MUJOCO_DIR` overrides the fetch) |

## Planned / adjacent (not vendored)

| Component | License (typical) | Role in LBS |
|-----------|-------------------|-------------|
| Gazebo / gz-sim | Apache-2.0 | L3 partner FA path |
| ROS 2 / ros2_control / BehaviorTree.ROS2 | Apache-2.0 / BSD (ecosystem) | L3 FA + BT condition reader |

## Not included

- **Latch core** — proprietary All Rights Reserved; linked
  via an optional external library, never vendored here.

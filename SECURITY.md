# Security Policy

## Supported versions

Security fixes apply to `main`. The L1 MuJoCo harness is on `main`. `lbs_node_wrap` Passed as mock-oracle. A gz-sim server launch ran headless for a fixed iteration count. The world has no Latch plugin. A separate CI workflow starts a ros2_control forward position controller on mock hardware. Yield leaves that command. An emergency stop holds the command without reading the mode. A separate CI workflow clones BehaviorTree.ROS2 at 72a3bf51dad332c67b99fc3373ccd5242f94f680 and ticks a read-only condition on /latch/mode_name. The condition does not elect and does not publish. Another workflow starts that mock controller and that condition in one job. A gz_ros2_control job spawns one prismatic joint, loads the same position controller, and runs that condition in the same job. Approach on /latch/mode_name moves the sim joint through the position controller. Yield leaves that command. The one-joint gz job runs that mock-oracle process with --drive-sim. Approach moves the sim joint. E-stop elects yield and leaves the command. It is not Latch. Latch is not linked. The empty world does not elect a mode. The partner checklist runs that one-joint path. `scripts/fa_checklist_run.sh partner` starts `harness/ros2_gz/launch_partner.sh`, which starts gz-sim through `harness/ros2_gz/launch/partner_fa.launch.py`. The log prints `gz-sim launched` or `gz-sim did not start`. Yield leaves the setpoint. The partner launch **Exists** as mock-oracle. It is not a Latch certificate. Bit-match, live evidence, chatter, stale, deny, budget, reject, evidence replay, soak, operator signature, and the latency histogram stay **Gap** on that run.

## Reporting a vulnerability

Please **do not** open a public GitHub issue for security reports.

Email: **security@iome.sh** (preferred)

Include:

- Description of the issue and impact
- Steps to reproduce or proof-of-concept (if available)
- Affected paths / commit SHA if known

You should receive an acknowledgment within a few business days.

## Scope notes

- This suite is a **harness and fixtures** repo. Latch core (proprietary) is
  out of scope here; report Latch-core issues through your Latch channel.
- Do not attach proprietary Latch binaries or customer logs to public issues.

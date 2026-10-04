# One joint under gz_ros2_control

One CI job starts a headless Harmonic server from the ROS vendor packages,
spawns this prismatic joint through `gz_ros2_control/GazeboSimSystem`, and
loads the stock forward position controller. The existing mode mapper and the
read-only condition share `/latch/mode_name` in that job.

Latch is not linked. `harness/ros2_gz/worlds/empty.sdf` is unchanged and still
does not elect a mode. The partner checklist calls this sequence through
`harness/ros2_gz/launch_partner.sh`. Yield leaves the setpoint. This is
mock-oracle. Checklist rows this run does not cover
stay Gap.

The one-joint job's mock-oracle process keeps approach for three ticks when
contact and a high wrench arrive, then publishes grasp, then yield on e-stop.
Grasp does not publish a new position command. It is not Latch.

After approach moves the sim joint, `/emergency_stop` is published and the
mapper stops issuing position commands without using the mode byte. The joint
stays. It is not Latch and not a protective stop.

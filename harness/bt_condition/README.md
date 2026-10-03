# Read-only condition

CI clones BehaviorTree.ROS2 at commit `72a3bf51dad332c67b99fc3373ccd5242f94f680` and builds `latch_bt_condition`.
The condition subscribes to `/latch/mode_name` (`std_msgs/UInt8`) and returns
success only when the byte matches the `expected` port. It does not publish
and it does not elect.

ros2_control is not started here. gz-sim is not started here. The partner
launch is `harness/ros2_gz`.
The upstream tree is not vendored into this repository.

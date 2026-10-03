# ros2_control on mock hardware

A CI workflow starts Controller Manager, `joint_state_broadcaster`, and the stock
`forward_command_controller/ForwardCommandController` against
`mock_components/GenericSystem` (one position joint). `mode_mapper.py` turns a
mode name on `/latch/mode_name` into a position command. Approach adds a small
delta. Yield and hold publish no new command, so a stale joint sample cannot
pull the joint backward. `/emergency_stop` holds the last command and does not
use the mode byte for the decision.

Approach adds the step, retract subtracts it, and grasp, release, yield, hold, and an emergency stop publish no command. The unit test locks that.

Controller Manager keeps lifecycle. This process does not activate or
deactivate controllers, and it does not elect a mode.

A second workflow starts this controller and the read-only condition in one
job. The condition subscribes to `/latch/mode_name` and does not elect.
gz-sim is not started in that job. The partner launch is `harness/ros2_gz`.
No Latch library is linked.

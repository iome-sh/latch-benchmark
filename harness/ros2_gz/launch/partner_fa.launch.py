# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 iome-sh contributors
"""Partner-path gz-sim bringup for the one-joint bench.

Starts a headless gz-sim server, the clock bridge, and robot_state_publisher.
`harness/ros2_gz/launch_partner.sh` then spawns the joint, the position
controller, the read-only condition, and the mock-oracle drive. Yield leaves
the setpoint. This is not a Latch certificate.
"""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    gz_args = LaunchConfiguration("gz_args")
    rsp_params = LaunchConfiguration("rsp_params")
    gz_sim = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution(
                [FindPackageShare("ros_gz_sim"), "launch", "gz_sim.launch.py"]
            )
        ),
        launch_arguments={"gz_args": gz_args}.items(),
    )
    bridge = ExecuteProcess(
        cmd=[
            "ros2",
            "run",
            "ros_gz_bridge",
            "parameter_bridge",
            "/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock",
        ],
        output="screen",
    )
    robot_state = ExecuteProcess(
        cmd=[
            "ros2",
            "run",
            "robot_state_publisher",
            "robot_state_publisher",
            "--ros-args",
            "--params-file",
            rsp_params,
        ],
        output="screen",
    )
    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "gz_args",
                default_value="-s -r -v 1 empty.sdf",
            ),
            DeclareLaunchArgument("rsp_params"),
            gz_sim,
            bridge,
            robot_state,
        ]
    )

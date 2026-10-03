#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 iome-sh contributors
"""Assert the mock-hardware position command follows a mode name."""

import sys
import time

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import JointState
from std_msgs.msg import Bool, UInt8


def fail(msg: str) -> None:
    print(f"assert_motion: {msg}", file=sys.stderr, flush=True)
    sys.exit(1)


class Probe(Node):
    def __init__(self) -> None:
        super().__init__("mode_motion_probe")
        self.position = None
        self.create_subscription(JointState, "/joint_states", self.on_joint, 10)
        self.mode_pub = self.create_publisher(UInt8, "/latch/mode_name", 10)
        self.stop_pub = self.create_publisher(Bool, "/emergency_stop", 10)

    def on_joint(self, msg: JointState) -> None:
        if msg.name and "joint1" in msg.name:
            self.position = float(msg.position[msg.name.index("joint1")])
        elif msg.position:
            self.position = float(msg.position[0])

    def spin_for(self, seconds: float) -> None:
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            rclpy.spin_once(self, timeout_sec=0.05)

    def publish_mode(self, name: int, seconds: float) -> None:
        msg = UInt8()
        msg.data = name
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            self.mode_pub.publish(msg)
            rclpy.spin_once(self, timeout_sec=0.05)

    def publish_stop(self, stopped: bool) -> None:
        msg = Bool()
        msg.data = stopped
        self.stop_pub.publish(msg)
        self.spin_for(0.2)


def main() -> None:
    rclpy.init()
    node = Probe()
    deadline = time.monotonic() + 20.0
    while node.position is None:
        if time.monotonic() > deadline:
            fail("no joint state")
        rclpy.spin_once(node, timeout_sec=0.1)

    start = node.position
    advanced = False
    deadline = time.monotonic() + 5.0
    while time.monotonic() < deadline:
        node.publish_mode(0, 0.25)
        if node.position is not None and node.position >= start + 0.007:
            advanced = True
            break
    if not advanced:
        fail(f"approach did not advance ({start} -> {node.position})")

    # One approach command can still be applied after the name changes.
    # Sample again once that write has landed, then require the joint to stay.
    node.publish_mode(4, 0.8)
    held = node.position
    node.publish_mode(4, 0.8)
    if node.position is None or abs(node.position - held) > 0.004:
        fail(f"yield moved the joint ({held} -> {node.position})")
    print("yield leaves the command", flush=True)

    node.publish_stop(True)
    node.publish_mode(0, 0.4)
    frozen = node.position
    node.publish_mode(0, 0.6)
    if node.position is None or abs(node.position - frozen) > 0.004:
        fail(f"series stop did not hold ({frozen} -> {node.position})")

    print("Latch does not activate controllers", flush=True)
    print("BT.ROS2 not launched", flush=True)
    print("not a gz-sim partner launch", flush=True)
    print("mock hardware only", flush=True)
    print("this job does not start gz-sim", flush=True)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 iome-sh contributors
"""Map a published mode name onto a position command.

This process does not elect a mode and does not activate or deactivate
controllers. Controller Manager keeps lifecycle.
"""

import importlib.util
import os
import pathlib

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import JointState
from std_msgs.msg import Bool, Float64MultiArray, UInt8

# Default keeps the mock-hardware delta. The sim launch overrides it.
step = float(os.environ.get("LATCH_BENCH_STEP", "0.008"))

_spec = importlib.util.spec_from_file_location(
    "mode_delta", pathlib.Path(__file__).with_name("mode_delta.py")
)
_mod = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_mod)
command_delta = _mod.command_delta


class ModeMapper(Node):
    def __init__(self) -> None:
        super().__init__("mode_to_position")
        self.measured = None
        self.stop = False
        self.stop_noted = False
        self.create_subscription(JointState, "/joint_states", self.on_joint, 10)
        self.create_subscription(UInt8, "/latch/mode_name", self.on_mode, 10)
        self.create_subscription(Bool, "/emergency_stop", self.on_stop, 10)
        self.pub = self.create_publisher(
            Float64MultiArray, "/forward_position_controller/commands", 10
        )

    def on_joint(self, msg: JointState) -> None:
        if msg.name and "joint1" in msg.name:
            self.measured = float(msg.position[msg.name.index("joint1")])
        elif msg.position:
            self.measured = float(msg.position[0])

    def note_stop(self) -> None:
        if not self.stop_noted:
            print("series stop tripped without reading the mode", flush=True)
            self.stop_noted = True

    def on_stop(self, msg: Bool) -> None:
        self.stop = bool(msg.data)
        if self.stop:
            self.note_stop()

    def on_mode(self, msg: UInt8) -> None:
        if self.measured is None:
            return
        delta = command_delta(int(msg.data), self.stop, step)
        if delta is None:
            if self.stop:
                self.note_stop()
            return
        command = Float64MultiArray()
        command.data = [self.measured + delta]
        self.pub.publish(command)


def main() -> None:
    rclpy.init()
    node = ModeMapper()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()

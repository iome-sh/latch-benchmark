# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 iome-sh contributors
"""Stdlib tests for the mode-byte position delta. Does not import ROS."""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import mode_delta


class CommandDeltaTest(unittest.TestCase):
    def test_stop_does_not_use_the_byte(self) -> None:
        self.assertIsNone(mode_delta.command_delta(0, True, 0.25))
        self.assertIsNone(mode_delta.command_delta(4, True, 0.25))

    def test_approach_adds_step(self) -> None:
        self.assertEqual(mode_delta.command_delta(0, False, 0.25), 0.25)

    def test_retract_subtracts_step(self) -> None:
        self.assertEqual(mode_delta.command_delta(3, False, 0.25), -0.25)

    def test_grasp_release_yield_hold_publish_nothing(self) -> None:
        for mode_byte in (1, 2, 4, 5):
            self.assertIsNone(mode_delta.command_delta(mode_byte, False, 0.25))

    def test_other_byte_publishes_nothing(self) -> None:
        self.assertIsNone(mode_delta.command_delta(9, False, 0.25))


if __name__ == "__main__":
    unittest.main()

# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 iome-sh contributors
"""Position delta for a mode byte. No ROS import."""


def command_delta(mode_byte: int, stopped: bool, step: float):
    """Return a position delta, or None when the mapper must not publish."""
    if stopped:
        return None
    if mode_byte == 0:
        return step
    if mode_byte == 3:
        return -step
    return None

/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 iome-sh contributors
 *
 * Mode name -> position setpoint. Yield and hold leave the target.
 * No torque, no trajectory.
 */
#include "lbs.h"

enum { kApproachMilli = 20, kRetractMilli = 20, kGraspMilli = 1000, kReleaseMilli = 200 };

int lbs_mode_to_setpoint(unsigned name, double current, double *qpos_target, int *leave) {
  if (!qpos_target || !leave) return 1;
  *leave = 0;
  switch (name) {
    case LATCH_APPROACH:
      *qpos_target = current + (kApproachMilli / 1000.0);
      break;
    case LATCH_GRASP:
      *qpos_target = kGraspMilli / 1000.0;
      break;
    case LATCH_RELEASE:
      *qpos_target = kReleaseMilli / 1000.0;
      break;
    case LATCH_RETRACT:
      *qpos_target = current - (kRetractMilli / 1000.0);
      break;
    case LATCH_YIELD:
    case LATCH_HOLD:
    default:
      *qpos_target = current;
      *leave = 1;
      break;
  }
  return 0;
}

/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 iome-sh contributors */
#include "lbs.h"

#include <string.h>

const char *lbs_mode_name(unsigned name) {
  switch (name) {
    case LATCH_APPROACH:
      return "approach";
    case LATCH_GRASP:
      return "grasp";
    case LATCH_RELEASE:
      return "release";
    case LATCH_RETRACT:
      return "retract";
    case LATCH_YIELD:
      return "yield";
    case LATCH_HOLD:
      return "hold";
    default:
      return "invalid";
  }
}

int lbs_mode_id(const char *text, unsigned *out) {
  unsigned i = 0;
  if (!text || !out) return 0;
  for (i = 0; i < LATCH_MODE_COUNT; ++i) {
    if (strcmp(text, lbs_mode_name(i)) == 0) {
      *out = i;
      return 1;
    }
  }
  return 0;
}

const char *lbs_plane_name(unsigned plane) {
  switch (plane) {
    case LATCH_PLANE_DENY:
      return "deny";
    case LATCH_PLANE_BUDGET:
      return "budget";
    case LATCH_PLANE_DWELL:
      return "dwell";
    case LATCH_PLANE_L2:
      return "l2";
    case LATCH_PLANE_REJECT:
      return "reject";
    default:
      return "invalid";
  }
}

int lbs_plane_id(const char *text, unsigned *out) {
  unsigned i = 0;
  if (!text || !out) return 0;
  for (i = 0; i <= LATCH_PLANE_REJECT; ++i) {
    if (strcmp(text, lbs_plane_name(i)) == 0) {
      *out = i;
      return 1;
    }
  }
  return 0;
}

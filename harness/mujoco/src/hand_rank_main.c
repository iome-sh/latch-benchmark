/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 iome-sh contributors
 *
 * Fresh binds on the mock oracle. Each case is its own LatchState.
 * Grasped with a seen target elects release. No target, contact, or
 * grasp elects retract. E-stop elects yield on plane deny.
 * Not Latch. L3 stays open.
 */
#include "lbs.h"

#include <stdio.h>
#include <string.h>

#if LBS_MOCK_ORACLE
static int fresh_consider(const LatchSense *sense, LatchMode *out) {
  LatchState state;
  const LatchBlob blob = {LATCH_BLOB_SCHEMA, 0, "lbs-mock-oracle"};
  memset(&state, 0, sizeof state);
  memset(out, 0, sizeof *out);
  if (latch_bind(&state, &blob) != 0) return 1;
  if (state.has_mode != 0) return 1;
  return latch_consider(sense, &state, 0, out);
}
#endif

int main(void) {
#if !LBS_MOCK_ORACLE
  printf("mock rank not run\n");
  printf("provided library linked; mock score table not applied\n");
  return 0;
#else
  const LatchSense grasped = {20, 0, 0, 0, 0, 1, 1, 0, 5};
  const LatchSense gone = {20, 0, 0, 0, 0, 0, 0, 0, 5};
  LatchSense stopped = gone;
  LatchMode mode;
  int fails = 0;

  stopped.estop = 1;
  memset(&mode, 0, sizeof mode);

  if (fresh_consider(&grasped, &mode) != 0 || mode.name != LATCH_RELEASE) {
    fprintf(stderr, "FAIL grasped name=%s\n", lbs_mode_name(mode.name));
    fails += 1;
  } else {
    printf("grasped elects release\n");
  }

  if (fresh_consider(&gone, &mode) != 0 || mode.name != LATCH_RETRACT) {
    fprintf(stderr, "FAIL target gone name=%s\n", lbs_mode_name(mode.name));
    fails += 1;
  } else {
    printf("target gone elects retract\n");
  }

  if (fresh_consider(&stopped, &mode) != 0 || mode.name != LATCH_YIELD || mode.plane != LATCH_PLANE_DENY) {
    fprintf(stderr, "FAIL estop name=%s plane=%u\n", lbs_mode_name(mode.name), (unsigned)mode.plane);
    fails += 1;
  } else {
    printf("estop elects yield\n");
  }

  if (fails) return 1;
  printf("mock-oracle hand rank\n");
  printf("not Latch\n");
  printf("L3 stays open\n");
  return 0;
#endif
}

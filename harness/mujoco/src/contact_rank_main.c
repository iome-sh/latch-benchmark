/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 iome-sh contributors
 *
 * Fresh binds on the mock oracle. Each of those cases is its own LatchState.
 * Contact plus a high wrench elects grasp. A seen target with a low
 * wrench elects approach. E-stop elects yield on plane deny.
 * One more case reuses a single state: after approach, contact and a high
 * wrench keep approach for three dwell ticks, then elect grasp.
 * Not Latch. This binary does not start gz-sim.
 */
#include "lbs.h"

#include <stdio.h>
#include <string.h>

static int fresh_consider(const LatchSense *sense, LatchMode *out) {
  LatchState state;
  const LatchBlob blob = {LATCH_BLOB_SCHEMA, 0, "lbs-mock-oracle"};
  memset(&state, 0, sizeof state);
  memset(out, 0, sizeof *out);
  if (latch_bind(&state, &blob) != 0) return 1;
  if (state.has_mode != 0) return 1;
  return latch_consider(sense, &state, 0, out);
}

/* Same state, not a fresh bind. Approach is elected once. Contact and
 * wrench 60 then challenge for three ticks (plane dwell) before grasp. */
static int same_state_contact_dwell(void) {
  LatchState state;
  const LatchBlob blob = {LATCH_BLOB_SCHEMA, 0, "lbs-mock-oracle"};
  LatchSense sense = {20, 0, 0, 0, 0, 0, 1, 0, 5};
  LatchMode mode;
  int tick = 0;

  memset(&state, 0, sizeof state);
  memset(&mode, 0, sizeof mode);
  if (latch_bind(&state, &blob) != 0 || latch_consider(&sense, &state, 0, &mode) != 0 ||
      mode.name != LATCH_APPROACH) {
    fprintf(stderr, "FAIL same state approach name=%s\n", lbs_mode_name(mode.name));
    return 1;
  }

  sense.contact = 1;
  sense.wrench = 60;
  for (tick = 0; tick < 3; ++tick) {
    if (latch_consider(&sense, &state, 0, &mode) != 0 || mode.name != LATCH_APPROACH ||
        mode.plane != LATCH_PLANE_DWELL) {
      fprintf(stderr, "FAIL contact dwell tick=%d name=%s plane=%u\n", tick + 1, lbs_mode_name(mode.name),
              (unsigned)mode.plane);
      return 1;
    }
  }
  printf("contact dwell keeps approach\n");

  if (latch_consider(&sense, &state, 0, &mode) != 0 || mode.name != LATCH_GRASP) {
    fprintf(stderr, "FAIL dwell expired name=%s\n", lbs_mode_name(mode.name));
    return 1;
  }
  printf("dwell expired elects grasp\n");
  return 0;
}

int main(void) {
  const LatchSense low = {20, 0, 0, 0, 0, 0, 1, 0, 5};
  const LatchSense high = {20, 0, 0, 0, 1, 0, 1, 0, 60};
  LatchSense stopped = low;
  LatchMode mode;
  int fails = 0;

  stopped.estop = 1;
  memset(&mode, 0, sizeof mode);

  if (fresh_consider(&low, &mode) != 0 || mode.name != LATCH_APPROACH) {
    fprintf(stderr, "FAIL low wrench name=%s\n", lbs_mode_name(mode.name));
    fails += 1;
  } else {
    printf("low wrench elects approach\n");
  }

  if (fresh_consider(&high, &mode) != 0 || mode.name != LATCH_GRASP) {
    fprintf(stderr, "FAIL contact wrench name=%s\n", lbs_mode_name(mode.name));
    fails += 1;
  } else {
    printf("contact and wrench elect approach aside\n");
  }

  if (fresh_consider(&stopped, &mode) != 0 || mode.name != LATCH_YIELD || mode.plane != LATCH_PLANE_DENY) {
    fprintf(stderr, "FAIL estop name=%s plane=%u\n", lbs_mode_name(mode.name), (unsigned)mode.plane);
    fails += 1;
  } else {
    printf("estop elects yield\n");
  }

  fails += same_state_contact_dwell();

  if (fails) return 1;
  printf("mock-oracle contact rank\n");
  printf("not Latch\n");
  return 0;
}

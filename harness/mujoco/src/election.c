/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 iome-sh contributors
 *
 * One 10–50 Hz election. Consider, then apply-reject, then consume the name
 * as a position setpoint. No allocation. Not called from inside mj_step.
 */
#define _POSIX_C_SOURCE 200809L

#include "lbs.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

static int64_t now_us(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (int64_t)ts.tv_sec * 1000000 + (int64_t)ts.tv_nsec / 1000;
}

static void push_order(LbsElection *election, int event) {
  if (election->order_n < LBS_ORDER_CAP) election->order[election->order_n++] = event;
}

static void push_name(LbsElection *election, uint8_t name) {
  if (election->names_n < LBS_NAME_CAP) election->names[election->names_n++] = name;
}

static void note_illegal(LbsElection *election, const LatchSense *sense, unsigned name) {
  uint32_t mask = latch_legal_mask(sense);
  if ((mask & (1u << name)) == 0) election->illegal += 1;
}

static double clamp_qpos(double q) {
  const double lo = LBS_QPOS_MIN_MILLI / 1000.0;
  const double hi = LBS_QPOS_MAX_MILLI / 1000.0;
  if (q < lo) return lo;
  if (q > hi) return hi;
  return q;
}

int lbs_election_init(LbsElection *election, const char *snapshot_id) {
  int rc = 0;
  if (!election || !snapshot_id || !snapshot_id[0]) return 1;
  memset(election, 0, sizeof *election);
  election->snapshot_id = snapshot_id;
  election->blob.schema = LATCH_BLOB_SCHEMA;
  election->blob.snapshot_id = election->snapshot_id;
  rc = latch_bind(&election->state, &election->blob);
  return rc;
}

int lbs_election_tick(LbsElection *election, LatchSense sense) {
  LatchMode considered;
  LatchMode applied;
  LatchSense apply;
  int64_t t0 = 0;
  int64_t dt = 0;
  int rc = 0;
  int leave = 0;
  double next = 0;
  if (!election) return 1;
  if (election->mj_guard && *election->mj_guard) {
    if (election->mj_violations) *election->mj_violations += 1;
    return 5;
  }

  memset(&considered, 0, sizeof considered);
  memset(&applied, 0, sizeof applied);
  if (election->hooks.adjust_sense)
    election->hooks.adjust_sense(&sense, election->consider_calls, election->hooks.user);

  push_order(election, LBS_EV_CONSIDER);
  t0 = now_us();
  rc = latch_consider(&sense, &election->state, election->budget_hit, &considered);
  dt = now_us() - t0;
  if (election->hist_n < LBS_HIST_CAP) election->hist_us[election->hist_n++] = dt < 0 ? 0 : dt;
  election->consider_calls += 1;
  if (rc != 0) return rc;
  note_illegal(election, &sense, considered.name);
  if (considered.plane == LATCH_PLANE_REJECT) election->illegal += 1;
  election->decision_sense = sense;

  apply = sense;
  if (election->hooks.adjust_apply)
    election->hooks.adjust_apply(&apply, election->consider_calls - 1, election->hooks.user);
  election->apply_sense = apply;

  push_order(election, LBS_EV_REJECT);
  rc = latch_apply_reject(&sense, &apply, &considered, &applied);
  election->reject_calls += 1;
  if (rc < 0) return 6;
  note_illegal(election, &apply, applied.name);

  push_order(election, LBS_EV_CONSUME);
  election->consume_calls += 1;
  next = election->setpoint;
  if (lbs_mode_to_setpoint(applied.name, election->setpoint, &next, &leave) != 0) return 7;
  election->last_leave = leave;
  if (!leave) {
    election->setpoint = clamp_qpos(next);
    election->have_setpoint = 1;
  }
  election->mode = applied;
  push_name(election, applied.name);
  latch_evidence(&election->state, &apply, &applied, election->evidence);
  return 0;
}

static int cmp_i64(const void *a, const void *b) {
  int64_t x = *(const int64_t *)a;
  int64_t y = *(const int64_t *)b;
  return (x > y) - (x < y);
}

void lbs_hist_summarize(const int64_t *samples, int n, int64_t *p50, int64_t *p99, int64_t *max_us) {
  int64_t sorted[LBS_HIST_CAP];
  int ncopy = 0;
  int i50 = 0;
  int i99 = 0;
  if (!p50 || !p99 || !max_us) return;
  *p50 = 0;
  *p99 = 0;
  *max_us = 0;
  if (!samples || n <= 0) return;
  ncopy = n > LBS_HIST_CAP ? LBS_HIST_CAP : n;
  memcpy(sorted, samples, (size_t)ncopy * sizeof sorted[0]);
  qsort(sorted, (size_t)ncopy, sizeof sorted[0], cmp_i64);
  i50 = (50 * ncopy) / 100;
  i99 = (99 * ncopy) / 100;
  if (i50 >= ncopy) i50 = ncopy - 1;
  if (i99 >= ncopy) i99 = ncopy - 1;
  *p50 = sorted[i50];
  *p99 = sorted[i99];
  *max_us = sorted[ncopy - 1];
}

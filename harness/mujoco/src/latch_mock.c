/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 iome-sh contributors
 *
 * Compile-time Latch stand-in used when no private liblatch is on the link line.
 *
 * MOCK-ORACLE. This is not Latch. Scores, margins, and the 16-hex digest are
 * frozen only for harness/mujoco/fixtures/mock-oracle.jsonl.example. They are
 * not a 60-row Latch bit-match and not evidence payload v2.
 *
 * Behavior covered so fixture tests can run without the artifact:
 *   deny (estop / collision / limit) before budget, yield on plane deny, dwell cleared
 *   sense_age >= 4 -> yield on plane l2 (not deny)
 *   budget keeps the last legal mode, or hold if none, on plane budget
 *   dwell holds a legal name while the challenger margin is under 15
 *   apply-reject writes hold / plane reject when a motion precondition dies
 *   emitted names stay inside latch_legal_mask (illegal emission stays 0)
 * The tick path does not allocate.
 */
#include "latch_abi.h"

#include <string.h>

static void clear_mode(LatchMode *mode) {
  mode->name = 0;
  mode->score = 0;
  mode->margin = 0;
  mode->dwell = 0;
  mode->plane = 0;
  mode->_pad[0] = 0;
  mode->_pad[1] = 0;
}

static int motion_name(unsigned name) {
  return name == LATCH_APPROACH || name == LATCH_GRASP || name == LATCH_RELEASE || name == LATCH_RETRACT;
}

uint32_t latch_legal_mask(const LatchSense *in) {
  uint32_t mask = (1u << LATCH_HOLD) | (1u << LATCH_YIELD);
  if (!in) return mask;
  if (in->estop || in->collision || in->limit || in->sense_age >= 4) return mask;
  if (in->target_seen && in->joint < 90) mask |= 1u << LATCH_APPROACH;
  if (in->contact && !in->grasped) mask |= 1u << LATCH_GRASP;
  if (in->grasped) mask |= 1u << LATCH_RELEASE;
  if (in->joint > 5) mask |= 1u << LATCH_RETRACT;
  return mask;
}

int latch_bind(LatchState *state, const LatchBlob *blob) {
  size_t n = 0;
  if (!state || !blob || !blob->snapshot_id) return 1;
  if (blob->schema != LATCH_BLOB_SCHEMA) return 2;
  n = strlen(blob->snapshot_id);
  if (n == 0 || n > LATCH_SNAPSHOT_ID_MAX) return 3;
  if (state->has_mode && state->blob && state->blob != blob) return 4;
  state->blob = blob;
  return 0;
}

/* Mock ranks only. Wrench tilts grasp vs approach by 10, under the
 * switch margin of 15, so a legal name can dwell through contact noise.
 * Not the Latch hand-rank table. */
static int mock_rank(unsigned name, const LatchSense *in) {
  int tilt = in->wrench >= 50;
  switch (name) {
    case LATCH_GRASP:
      return tilt ? 60 : 50;
    case LATCH_RELEASE:
      return 66;
    case LATCH_APPROACH:
      return tilt ? 50 : 60;
    case LATCH_RETRACT:
      return 40;
    case LATCH_YIELD:
      return 20;
    default:
      return 0;
  }
}

static void write_mode(LatchState *state, LatchMode *out, unsigned name, int score, int margin, int dwell,
                       unsigned plane) {
  clear_mode(out);
  out->name = (uint8_t)name;
  out->score = score;
  out->margin = margin;
  out->dwell = dwell;
  out->plane = (uint8_t)plane;
  state->mode = *out;
  state->has_mode = 1;
}

int latch_consider(const LatchSense *in, LatchState *state, int budget_hit, LatchMode *out) {
  uint32_t mask = 0;
  int best_name = LATCH_HOLD;
  int best_score = -1;
  int second = -1;
  int name = 0;
  int margin = 0;
  if (!in || !state || !out) return 1;
  if (!state->blob || state->blob->schema != LATCH_BLOB_SCHEMA) return 2;

  mask = latch_legal_mask(in);
  if (in->estop || in->collision || in->limit) {
    state->dwell_left = 0;
    write_mode(state, out, LATCH_YIELD, 100, 100, 0, LATCH_PLANE_DENY);
    return 0;
  }
  if (budget_hit) {
    if (state->has_mode && (mask & (1u << state->mode.name)) != 0) {
      *out = state->mode;
      out->_pad[0] = 0;
      out->_pad[1] = 0;
      out->plane = LATCH_PLANE_BUDGET;
      state->mode = *out;
      state->has_mode = 1;
      return 0;
    }
    state->dwell_left = 0;
    write_mode(state, out, LATCH_HOLD, 0, 0, 0, LATCH_PLANE_BUDGET);
    return 0;
  }
  if (in->sense_age >= 4) {
    state->dwell_left = 0;
    write_mode(state, out, LATCH_YIELD, 80, 80, 0, LATCH_PLANE_L2);
    return 0;
  }

  for (name = 0; name < LATCH_MODE_COUNT; ++name) {
    int score = 0;
    if ((mask & (1u << name)) == 0) continue;
    score = mock_rank((unsigned)name, in);
    if (score > best_score) {
      second = best_score;
      best_score = score;
      best_name = name;
    } else if (score > second) {
      second = score;
    }
  }
  if (best_score < 0) best_score = 0;
  if (second < 0) second = 0;
  margin = best_score - second;

  if (state->has_mode && state->mode.name != (uint8_t)best_name) {
    int prev_legal = (mask & (1u << state->mode.name)) != 0;
    if (prev_legal && margin < LATCH_SWITCH_MARGIN && state->dwell_left > 0) {
      *out = state->mode;
      out->_pad[0] = 0;
      out->_pad[1] = 0;
      out->plane = LATCH_PLANE_DWELL;
      state->dwell_left -= 1;
      out->dwell = state->dwell_left;
      state->mode = *out;
      return 0;
    }
  }

  if (state->has_mode && state->mode.name == (uint8_t)best_name) {
    int dwell = state->dwell_left;
    write_mode(state, out, (unsigned)best_name, best_score, margin, dwell, LATCH_PLANE_L2);
    return 0;
  }

  state->dwell_left = LATCH_DWELL_TICKS;
  write_mode(state, out, (unsigned)best_name, best_score, margin, LATCH_DWELL_TICKS, LATCH_PLANE_L2);
  return 0;
}

static void mix_bytes(uint64_t *hash, const void *data, size_t n) {
  const unsigned char *p = (const unsigned char *)data;
  size_t i = 0;
  for (i = 0; i < n; ++i) {
    *hash ^= p[i];
    *hash *= 1099511628211ull;
  }
}

static void mix_i32(uint64_t *hash, int32_t v) { mix_bytes(hash, &v, sizeof v); }

void latch_evidence(const LatchState *state, const LatchSense *in, const LatchMode *mode, char out[17]) {
  static const char kTag[] = "mock-oracle-not-ev2";
  static const char kHex[] = "0123456789abcdef";
  uint64_t hash = 14695981039346656037ull;
  int i = 0;
  if (!out) return;
  if (!state || !in || !mode || !state->blob || !state->blob->snapshot_id) {
    memset(out, '0', 16);
    out[16] = '\0';
    return;
  }
  mix_bytes(&hash, kTag, sizeof kTag - 1);
  mix_bytes(&hash, state->blob->snapshot_id, strlen(state->blob->snapshot_id));
  mix_i32(&hash, (int32_t)mode->name);
  mix_i32(&hash, mode->score);
  mix_i32(&hash, mode->margin);
  mix_i32(&hash, mode->dwell);
  mix_i32(&hash, (int32_t)mode->plane);
  mix_i32(&hash, in->joint);
  mix_i32(&hash, in->limit);
  mix_i32(&hash, in->collision);
  mix_i32(&hash, in->estop);
  mix_i32(&hash, in->contact);
  mix_i32(&hash, in->grasped);
  mix_i32(&hash, in->target_seen);
  mix_i32(&hash, in->sense_age);
  mix_i32(&hash, in->wrench);
  for (i = 0; i < 16; ++i) {
    unsigned shift = (unsigned)(15 - i) * 4u;
    out[i] = kHex[(hash >> shift) & 0xfu];
  }
  out[16] = '\0';
}

int latch_apply_reject(const LatchSense *decision_sense, const LatchSense *apply_sense,
                       const LatchMode *decision, LatchMode *out) {
  int died = 0;
  if (!decision_sense || !apply_sense || !decision || !out) return -1;
  *out = *decision;
  out->_pad[0] = 0;
  out->_pad[1] = 0;
  if (decision->plane == LATCH_PLANE_DENY || decision->name == LATCH_YIELD || decision->name == LATCH_HOLD)
    return 0;
  if (decision->name == LATCH_GRASP && decision_sense->contact && !apply_sense->contact) died = 1;
  if (decision->name == LATCH_APPROACH && decision_sense->target_seen && decision_sense->sense_age < 4 &&
      (apply_sense->sense_age >= 4 || !apply_sense->target_seen))
    died = 1;
  if (motion_name(decision->name) && !decision_sense->limit && apply_sense->limit) died = 1;
  if (!died) return 0;
  out->name = LATCH_HOLD;
  out->plane = LATCH_PLANE_REJECT;
  out->dwell = 0;
  return 1;
}

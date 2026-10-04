/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 iome-sh contributors
 *
 * Compile-time Latch stand-in used when no private liblatch is on the link line.
 *
 * MOCK-ORACLE. This is not Latch and not a certificate.
 * Snapshot lbs-mock-oracle keeps the scores, margins, and labeled digest
 * frozen for harness/mujoco/fixtures/mock-oracle.jsonl.example.
 * Snapshot latch-robot-1 recomputes fixtures/latch-robot-1.jsonl: rank,
 * dwell, reject quality, and the evidence-v2 SHA-256 prefix. grasped is a
 * sense input in that payload.
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

#include <stdio.h>
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

static char g_bound_snapshot[LATCH_SNAPSHOT_ID_MAX + 1];

int latch_bind(LatchState *state, const LatchBlob *blob) {
  size_t n = 0;
  if (!state || !blob || !blob->snapshot_id) return 1;
  if (blob->schema != LATCH_BLOB_SCHEMA) return 2;
  n = strlen(blob->snapshot_id);
  if (n == 0 || n > LATCH_SNAPSHOT_ID_MAX) return 3;
  if (state->has_mode && state->blob && state->blob != blob) return 4;
  memcpy(g_bound_snapshot, blob->snapshot_id, n);
  g_bound_snapshot[n] = '\0';
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

static int robot_snapshot(const char *snapshot_id) {
  return snapshot_id && strcmp(snapshot_id, "latch-robot-1") == 0;
}

static int robot_rank(unsigned name) {
  switch (name) {
    case LATCH_GRASP:
      return 70;
    case LATCH_RELEASE:
      return 20;
    case LATCH_APPROACH:
      return 60;
    case LATCH_RETRACT:
      return 40;
    case LATCH_YIELD:
      return 100;
    case LATCH_HOLD:
      return 25;
    default:
      return 0;
  }
}

static const char *mode_word(unsigned name) {
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

static const char *plane_word(unsigned plane) {
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
  if (robot_snapshot(state->blob->snapshot_id)) {
    int best_name = LATCH_HOLD;
    int best_score = -1;
    int second = -1;
    for (name = 0; name < LATCH_MODE_COUNT; ++name) {
      int score = 0;
      if ((mask & (1u << name)) == 0) continue;
      if (name == LATCH_YIELD && in->sense_age < 4) continue;
      if (name == LATCH_RETRACT && in->target_seen) continue;
      score = robot_rank((unsigned)name);
      if (score > best_score) {
        second = best_score;
        best_score = score;
        best_name = name;
      } else if (score > second) {
        second = score;
      }
    }
    if (best_score < 0) best_score = 0;
    margin = second < 0 ? 0 : best_score - second;
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
      if (state->dwell_left > 0) state->dwell_left -= 1;
      write_mode(state, out, (unsigned)best_name, best_score, margin, state->dwell_left, LATCH_PLANE_L2);
      return 0;
    }
    state->dwell_left = LATCH_DWELL_TICKS;
    write_mode(state, out, (unsigned)best_name, best_score, margin, LATCH_DWELL_TICKS, LATCH_PLANE_L2);
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

static uint32_t sha_rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32u - n)); }

static void sha256_block(uint32_t h[8], const unsigned char block[64]) {
  static const uint32_t k[64] = {
      0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
      0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
      0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
      0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
      0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
      0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
      0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
      0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};
  uint32_t w[64];
  uint32_t a = 0;
  uint32_t b = 0;
  uint32_t c = 0;
  uint32_t d = 0;
  uint32_t e = 0;
  uint32_t f = 0;
  uint32_t g = 0;
  uint32_t hh = 0;
  int i = 0;
  for (i = 0; i < 16; ++i) {
    w[i] = ((uint32_t)block[i * 4] << 24) | ((uint32_t)block[i * 4 + 1] << 16) | ((uint32_t)block[i * 4 + 2] << 8) |
           (uint32_t)block[i * 4 + 3];
  }
  for (i = 16; i < 64; ++i) {
    uint32_t s0 = sha_rotr(w[i - 15], 7) ^ sha_rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
    uint32_t s1 = sha_rotr(w[i - 2], 17) ^ sha_rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
    w[i] = w[i - 16] + s0 + w[i - 7] + s1;
  }
  a = h[0];
  b = h[1];
  c = h[2];
  d = h[3];
  e = h[4];
  f = h[5];
  g = h[6];
  hh = h[7];
  for (i = 0; i < 64; ++i) {
    uint32_t s1 = sha_rotr(e, 6) ^ sha_rotr(e, 11) ^ sha_rotr(e, 25);
    uint32_t ch = (e & f) ^ ((~e) & g);
    uint32_t t1 = hh + s1 + ch + k[i] + w[i];
    uint32_t s0 = sha_rotr(a, 2) ^ sha_rotr(a, 13) ^ sha_rotr(a, 22);
    uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
    uint32_t t2 = s0 + maj;
    hh = g;
    g = f;
    f = e;
    e = d + t1;
    d = c;
    c = b;
    b = a;
    a = t1 + t2;
  }
  h[0] += a;
  h[1] += b;
  h[2] += c;
  h[3] += d;
  h[4] += e;
  h[5] += f;
  h[6] += g;
  h[7] += hh;
}

static void sha256_sum(const unsigned char *msg, size_t len, unsigned char out[32]) {
  uint32_t h[8] = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
                   0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};
  unsigned char block[64];
  size_t off = 0;
  uint64_t bits = (uint64_t)len * 8u;
  int i = 0;
  while (len - off >= 64u) {
    sha256_block(h, msg + off);
    off += 64u;
  }
  memset(block, 0, sizeof block);
  if (len > off) memcpy(block, msg + off, len - off);
  block[len - off] = 0x80u;
  if (len - off >= 56u) {
    sha256_block(h, block);
    memset(block, 0, sizeof block);
  }
  block[63] = (unsigned char)bits;
  block[62] = (unsigned char)(bits >> 8);
  block[61] = (unsigned char)(bits >> 16);
  block[60] = (unsigned char)(bits >> 24);
  block[59] = (unsigned char)(bits >> 32);
  block[58] = (unsigned char)(bits >> 40);
  block[57] = (unsigned char)(bits >> 48);
  block[56] = (unsigned char)(bits >> 56);
  sha256_block(h, block);
  for (i = 0; i < 8; ++i) {
    out[i * 4] = (unsigned char)(h[i] >> 24);
    out[i * 4 + 1] = (unsigned char)(h[i] >> 16);
    out[i * 4 + 2] = (unsigned char)(h[i] >> 8);
    out[i * 4 + 3] = (unsigned char)h[i];
  }
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

static void ev2_evidence(const LatchState *state, const LatchSense *in, const LatchMode *mode, char out[17]) {
  static const char kHex[] = "0123456789abcdef";
  char payload[256];
  unsigned char dig[32];
  int n = 0;
  int i = 0;
  n = snprintf(payload, sizeof payload, "ev2|%s|%s|%d|%d|%d|%s|%d|%d|%d|%d|%d|%d|%d|%d|%d", state->blob->snapshot_id,
               mode_word(mode->name), mode->score, mode->margin, mode->dwell, plane_word(mode->plane), in->joint,
               in->limit, in->collision, in->estop, in->contact, in->grasped, in->target_seen, in->sense_age,
               in->wrench);
  if (n <= 0 || (size_t)n >= sizeof payload) {
    memset(out, '0', 16);
    out[16] = '\0';
    return;
  }
  sha256_sum((const unsigned char *)payload, (size_t)n, dig);
  for (i = 0; i < 8; ++i) {
    out[i * 2] = kHex[dig[i] >> 4];
    out[i * 2 + 1] = kHex[dig[i] & 0xfu];
  }
  out[16] = '\0';
}

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
  if (robot_snapshot(state->blob->snapshot_id)) {
    ev2_evidence(state, in, mode, out);
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
  if (robot_snapshot(g_bound_snapshot)) {
    out->score = 0;
    out->margin = 0;
  }
  return 1;
}

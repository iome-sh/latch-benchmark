/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 iome-sh contributors
 *
 * Thin declarations of the Latch C ABI this harness links.
 * This is not Latch source. The symbols come from a provided shared library
 * (LATCH_LIB_DIR / LATCH_LIBRARY) or from latch_mock.c when that
 * artifact is absent. Layout uses natural SysV alignment so a provided
 * library can be linked without this tree shipping Latch code.
 */
#ifndef LATCH_ABI_H
#define LATCH_ABI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  LATCH_APPROACH = 0,
  LATCH_GRASP = 1,
  LATCH_RELEASE = 2,
  LATCH_RETRACT = 3,
  LATCH_YIELD = 4,
  LATCH_HOLD = 5,
  LATCH_MODE_COUNT = 6
};

enum {
  LATCH_PLANE_DENY = 0,
  LATCH_PLANE_BUDGET = 1,
  LATCH_PLANE_DWELL = 2,
  LATCH_PLANE_L2 = 3,
  LATCH_PLANE_REJECT = 4
};

enum {
  LATCH_SWITCH_MARGIN = 15,
  LATCH_DWELL_TICKS = 3,
  LATCH_BLOB_SCHEMA = 2,
  LATCH_SNAPSHOT_ID_MAX = 63
};

typedef struct LatchSense {
  int32_t joint; /* 0 home, 100 at target */
  int32_t limit;
  int32_t collision;
  int32_t estop;
  int32_t contact;
  int32_t grasped;
  int32_t target_seen;
  int32_t sense_age;
  int32_t wrench; /* 0-100 */
} LatchSense;

typedef struct LatchMode {
  uint8_t name;
  int32_t score;
  int32_t margin;
  int32_t dwell;
  uint8_t plane;
  uint8_t _pad[2];
} LatchMode;

typedef struct LatchBlob {
  uint32_t schema;
  uint32_t _pad;
  const char *snapshot_id;
} LatchBlob;

typedef struct LatchState {
  LatchMode mode;
  int32_t dwell_left;
  int32_t has_mode;
  uint32_t _pad;
  const struct LatchBlob *blob;
} LatchState;

/* 0 success. 1 bad argument. 2 schema is not LATCH_BLOB_SCHEMA.
 * 3 snapshot id empty or longer than LATCH_SNAPSHOT_ID_MAX.
 * 4 refused: a different blob while a mode is already held. */
int latch_bind(LatchState *state, const LatchBlob *blob);

/* 0 success. 1 bad argument. 2 state has no bound schema-2 blob.
 * budget_hit nonzero skips rank and keeps the last legal mode.
 * Does not sleep and does not allocate. */
int latch_consider(const LatchSense *in, LatchState *state, int budget_hit, LatchMode *out);

/* Bit i set means mode i is legal. Hold is always set. */
uint32_t latch_legal_mask(const LatchSense *in);

/* Writes 16 hex chars and a NUL into out[17].
 * Real liblatch: evidence payload v2 (SHA-256 prefix).
 * Mock oracle: a labeled mock digest, not payload v2. */
void latch_evidence(const LatchState *state, const LatchSense *in, const LatchMode *mode,
                    char out[17]);

/* Apply-time check. Not the 1 kHz plant tick.
 * 0 precondition still holds (decision copied to out).
 * 1 hold / plane reject: grasp contact lost, approach target stale or gone,
 *   or a joint limit newly active.
 * Yield, hold, and a deny record pass through.
 * -1 bad argument. */
int latch_apply_reject(const LatchSense *decision_sense, const LatchSense *apply_sense,
                       const LatchMode *decision, LatchMode *out);

#if UINTPTR_MAX == UINT64_MAX
#ifdef __cplusplus
static_assert(sizeof(LatchSense) == 36, "LatchSense ABI size");
static_assert(offsetof(LatchMode, score) == 4, "LatchMode.score ABI");
static_assert(offsetof(LatchMode, plane) == 16, "LatchMode.plane ABI");
static_assert(sizeof(LatchMode) == 20, "LatchMode ABI size");
static_assert(offsetof(LatchState, blob) == 32, "LatchState.blob ABI");
static_assert(sizeof(LatchBlob) == 16, "LatchBlob ABI size");
#else
_Static_assert(sizeof(LatchSense) == 36, "LatchSense ABI size");
_Static_assert(offsetof(LatchMode, score) == 4, "LatchMode.score ABI");
_Static_assert(offsetof(LatchMode, plane) == 16, "LatchMode.plane ABI");
_Static_assert(sizeof(LatchMode) == 20, "LatchMode ABI size");
_Static_assert(offsetof(LatchState, blob) == 32, "LatchState.blob ABI");
_Static_assert(sizeof(LatchBlob) == 16, "LatchBlob ABI size");
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif

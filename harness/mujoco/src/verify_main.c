/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 iome-sh contributors
 *
 * BM-09 verify. Replays the in-tree fixture through the linked latch_* ABI
 * and latch_evidence. LATCH_SESSION, when set, is that fixture path.
 * LATCH_SNAPSHOT_ID, when set, is the blob id. Unset, the path is the
 * mock-oracle example and the id is lbs-mock-oracle. Public results are
 * the mock-oracle digest, not a Latch evidence certificate.
 */
#include "latch_abi.h"
#include "oracle_runner.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef LBS_FIXTURE_DIR
#define LBS_FIXTURE_DIR "fixtures"
#endif

static const char *snapshot_id(void) {
  const char *snap = getenv("LATCH_SNAPSHOT_ID");
  if (snap && snap[0]) return snap;
  return "lbs-mock-oracle";
}

static int fixture_path(char *path, int cap) {
  const char *session = getenv("LATCH_SESSION");
  int n = 0;
  if (session && session[0]) {
    n = snprintf(path, (size_t)cap, "%s", session);
  } else {
    n = snprintf(path, (size_t)cap, "%s/mock-oracle.jsonl.example", LBS_FIXTURE_DIR);
  }
  if (n <= 0 || n >= cap) {
    fprintf(stderr, "FAIL BM-09 fixture path\n");
    return 0;
  }
  return 1;
}

#if !LBS_MOCK_ORACLE
/* 1 and writes path when LATCH_SESSION or LATCH_GOLDENS names a file.
 * 0 when neither is set. -1 when the path does not fit. */
static int provided_fixture(char *path, int cap) {
  const char *session = getenv("LATCH_SESSION");
  const char *goldens = getenv("LATCH_GOLDENS");
  const char *use = NULL;
  int n = 0;
  if (session && session[0]) use = session;
  else if (goldens && goldens[0]) use = goldens;
  if (!use) return 0;
  n = snprintf(path, (size_t)cap, "%s", use);
  if (n <= 0 || n >= cap) return -1;
  return 1;
}

static int touch_provided_library(void) {
  LatchState state;
  LatchBlob blob;
  LatchSense sense;
  LatchMode mode;
  LatchMode applied;
  char hex[17];
  memset(&state, 0, sizeof state);
  memset(&blob, 0, sizeof blob);
  memset(&sense, 0, sizeof sense);
  memset(&mode, 0, sizeof mode);
  memset(&applied, 0, sizeof applied);
  memset(hex, 0, sizeof hex);
  blob.schema = LATCH_BLOB_SCHEMA;
  blob.snapshot_id = "latch-robot-1";
  sense.joint = 20;
  sense.target_seen = 1;
  if (latch_bind(&state, &blob) != 0) return 1;
  if (latch_consider(&sense, &state, 0, &mode) != 0) return 1;
  latch_evidence(&state, &sense, &mode, hex);
  if (latch_legal_mask(&sense) == 0) return 1;
  if (latch_apply_reject(&sense, &sense, &mode, &applied) < -1) return 1;
  return strlen(hex) == 16 ? 0 : 1;
}
#endif

int main(void) {
  OracleRun run;
  char path[512];
  const char *snap = NULL;
  if (!fixture_path(path, (int)sizeof path)) return 1;
  snap = snapshot_id();
#if LBS_MOCK_ORACLE
  if (lbs_run_oracle_jsonl(path, snap, 1, &run) != 0 || run.fails != 0 || run.rows < 1) {
    fprintf(stderr, "FAIL BM-09 mock-oracle rows=%d fails=%d\n", run.rows, run.fails);
    return 1;
  }
  printf("BM-09 mock-oracle %d rows evidence match (not a Latch evidence-v2 certificate)\n", run.rows);
  return 0;
#else
  {
    int kind = provided_fixture(path, (int)sizeof path);
    if (kind < 0) {
      fprintf(stderr, "FAIL BM-09 fixture path\n");
      return 1;
    }
    if (kind == 0) {
      if (touch_provided_library() != 0) {
        fprintf(stderr, "FAIL BM-09 provided library call\n");
        return 1;
      }
      printf("BM-09 provided library linked; fixture unset — evidence match not claimed\n");
      return 0;
    }
    snap = snapshot_id();
    if (lbs_run_oracle_jsonl(path, snap, 1, &run) != 0 || run.fails != 0 || run.rows < 1 || run.illegal != 0) {
      fprintf(stderr, "FAIL BM-09 provided library rows=%d fails=%d illegal=%d\n", run.rows, run.fails, run.illegal);
      return 1;
    }
    printf("BM-09 Latch library evidence match %d rows\n", run.rows);
    return 0;
  }
#endif
}

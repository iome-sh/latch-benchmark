/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 iome-sh contributors
 *
 * BM-09 verify. The mock build replays the in-tree example.
 * A provided library compares LATCH_GOLDENS (else LATCH_SESSION) through
 * the linked latch_* ABI. Exit 0 only when that file matches all 60 rows.
 * Replaying the in-tree example is not that pass.
 */
#include "oracle_runner.h"

#include <stdio.h>
#include <stdlib.h>

#ifndef LBS_FIXTURE_DIR
#define LBS_FIXTURE_DIR "fixtures"
#endif

static const char *snapshot_id(void) {
  const char *snap = getenv("LATCH_SNAPSHOT_ID");
  if (snap && snap[0]) return snap;
#if !LBS_MOCK_ORACLE
  {
    const char *goldens = getenv("LATCH_GOLDENS");
    if (goldens && goldens[0]) return "latch-robot-1";
  }
#endif
  return "lbs-mock-oracle";
}

#if LBS_MOCK_ORACLE
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
#endif

#if !LBS_MOCK_ORACLE
/* 1 and writes path when LATCH_GOLDENS (else LATCH_SESSION) names a file.
 * 0 when neither is set. -1 when the path does not fit. */
static int provided_fixture(char *path, int cap) {
  const char *session = getenv("LATCH_SESSION");
  const char *goldens = getenv("LATCH_GOLDENS");
  const char *use = NULL;
  int n = 0;
  if (goldens && goldens[0]) use = goldens;
  else if (session && session[0]) use = session;
  if (!use) return 0;
  n = snprintf(path, (size_t)cap, "%s", use);
  if (n <= 0 || n >= cap) return -1;
  return 1;
}
#endif

int main(void) {
  OracleRun run;
  char path[512];
  const char *snap = NULL;
#if LBS_MOCK_ORACLE
  if (!fixture_path(path, (int)sizeof path)) return 1;
  snap = snapshot_id();
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
      fprintf(stderr, "FAIL BM-09 LATCH_GOLDENS unset; in-tree fixture is not the 60-row compare\n");
      printf("BM-09 provided library linked; 60-row evidence match not claimed\n");
      return 1;
    }
    snap = snapshot_id();
    if (lbs_run_oracle_jsonl(path, snap, 1, &run) != 0 || run.fails != 0 || run.rows != 60 || run.illegal != 0) {
      fprintf(stderr, "FAIL BM-09 provided library rows=%d fails=%d illegal=%d\n", run.rows, run.fails, run.illegal);
      printf("BM-09 provided library linked; 60-row evidence match not claimed\n");
      return 1;
    }
    printf("BM-09 Latch library evidence match 60 rows\n");
    return 0;
  }
#endif
}

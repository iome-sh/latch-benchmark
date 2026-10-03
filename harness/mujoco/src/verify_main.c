/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 iome-sh contributors
 *
 * BM-09 verify. Replays the in-tree fixture through the linked latch_* ABI
 * and latch_evidence. Public results are the mock-oracle digest, not a
 * Latch evidence certificate.
 */
#include "oracle_runner.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef LBS_FIXTURE_DIR
#define LBS_FIXTURE_DIR "fixtures"
#endif

static int fixture_path(char *path, int cap) {
  int n = snprintf(path, (size_t)cap, "%s/mock-oracle.jsonl.example", LBS_FIXTURE_DIR);
  if (n <= 0 || n >= cap) {
    fprintf(stderr, "FAIL BM-09 fixture path\n");
    return 0;
  }
  return 1;
}

int main(void) {
  OracleRun run;
  char path[512];
  if (!fixture_path(path, (int)sizeof path)) return 1;
#if LBS_MOCK_ORACLE
  if (lbs_run_oracle_jsonl(path, "lbs-mock-oracle", 1, &run) != 0 || run.fails != 0 || run.rows < 1) {
    fprintf(stderr, "FAIL BM-09 mock-oracle rows=%d fails=%d\n", run.rows, run.fails);
    return 1;
  }
  printf("BM-09 mock-oracle %d rows evidence match (not a Latch evidence-v2 certificate)\n", run.rows);
  return 0;
#else
  /* In-tree rows carry the mock digest. Replay them, but do not treat a
   * match or a miss as a Latch bit-match certificate. */
  if (lbs_replay_oracle_jsonl(path, "lbs-mock-oracle", 0, 0, NULL, NULL, &run) != 0 || run.fails != 0 ||
      run.rows < 1) {
    fprintf(stderr, "FAIL BM-09 replay rows=%d fails=%d\n", run.rows, run.fails);
    return 1;
  }
  if (getenv("LATCH_GOLDENS") && getenv("LATCH_GOLDENS")[0]) {
    printf("BM-09 in-tree fixture replay only; exported goldens are not certified here\n");
  } else {
    printf("BM-09 in-tree fixture replay; Latch bit-match not claimed\n");
  }
  return 0;
#endif
}

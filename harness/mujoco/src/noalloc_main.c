/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 iome-sh contributors
 *
 * BM-12 alloc gate for the election tick (consider, evidence, apply-reject,
 * position consume). mj_step is not in this binary: MuJoCo may allocate.
 * --wrap counts malloc/calloc/realloc from this link, including the mock.
 */
#include "lbs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_allocs = 0;
static int g_count = 0;

extern void *__real_malloc(size_t);
extern void *__real_calloc(size_t, size_t);
extern void *__real_realloc(void *, size_t);

void *__wrap_malloc(size_t n) {
  if (g_count) g_allocs += 1;
  return __real_malloc(n ? n : 1);
}

void *__wrap_calloc(size_t n, size_t sz) {
  if (g_count) g_allocs += 1;
  return __real_calloc(n, sz);
}

void *__wrap_realloc(void *p, size_t n) {
  if (g_count) g_allocs += 1;
  return __real_realloc(p, n);
}

int main(void) {
  LbsElection election;
  int i = 0;
  int fails = 0;
  if (lbs_election_init(&election, "lbs-mock-oracle") != 0) {
    fprintf(stderr, "FAIL BM-12 noalloc bind\n");
    return 1;
  }
  g_allocs = 0;
  g_count = 1;
  for (i = 0; i < 200; ++i) {
    LatchSense sense;
    int rc = 0;
    memset(&sense, 0, sizeof sense);
    sense.joint = 20 + (i % 5);
    sense.target_seen = 1;
    sense.contact = (i % 2);
    sense.wrench = sense.contact ? 70 : 5;
    sense.sense_age = (i % 17 == 0) ? 4 : 0;
    sense.estop = (i % 19 == 0) ? 1 : 0;
    election.budget_hit = (i % 23 == 0) ? 1 : 0;
    if (i == 40) {
      /* Precondition dies after the next consider samples contact. */
      election.hooks.adjust_apply = NULL;
    }
    rc = lbs_election_tick(&election, sense);
    if (rc != 0) {
      fprintf(stderr, "FAIL BM-12 noalloc tick %d rc=%d\n", i, rc);
      fails += 1;
      break;
    }
  }
  g_count = 0;
  if (g_allocs != 0) {
    fprintf(stderr, "FAIL BM-12 noalloc allocs=%d\n", g_allocs);
    fails += 1;
  }
  if (election.illegal != 0) {
    fprintf(stderr, "FAIL BM-12 noalloc illegal=%d\n", election.illegal);
    fails += 1;
  }
  if (election.consider_calls != 200 || election.reject_calls != 200 || election.consume_calls != 200) {
    fprintf(stderr, "FAIL BM-12 noalloc call counts\n");
    fails += 1;
  }
  if (fails) return 1;
  printf("PASS BM-12 no-alloc election tick (allocs=0 illegal=0 calls=200)\n");
  return 0;
}

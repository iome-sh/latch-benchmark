/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 iome-sh contributors
 *
 * JSONL oracle runner. Mock rows are labeled mock-oracle.
 * Exported Latch goldens are compared only when a real liblatch is linked.
 */
#ifndef ORACLE_RUNNER_H
#define ORACLE_RUNNER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OracleRun {
  int rows;
  int fails;
  int chatter_flips;
  int raw_flips;
  int chatter_rows;
  int raw_rows;
  int illegal;
} OracleRun;

/* Produced name, plane, and evidence hex for one replayed row. */
typedef void (*LbsOracleEmit)(const char *id, const char *name, const char *plane, const char *evidence,
                             void *user);

/* compare_evidence nonzero: each row must carry a 16-hex evidence field.
 * snapshot_id is bound for the run (mock fixture vs exported Latch id).
 * Returns the fail count. */
int lbs_run_oracle_jsonl(const char *path, const char *snapshot_id, int compare_evidence,
                         OracleRun *out);

/* Same replay as lbs_run_oracle_jsonl. match_fields checks name, plane, score,
 * margin, and dwell. emit, if non-NULL, receives the ABI result after
 * latch_evidence. Returns the fail count. */
int lbs_replay_oracle_jsonl(const char *path, const char *snapshot_id, int compare_evidence,
                            int match_fields, LbsOracleEmit emit, void *user, OracleRun *out);

#ifdef __cplusplus
}
#endif

#endif

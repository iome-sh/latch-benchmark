/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 iome-sh contributors
 *
 * L1 MuJoCo harness: two-rate plant, position setpoints, fixture hooks.
 * Latch is linked, not implemented here (see latch_abi.h / latch_mock.c).
 */
#ifndef LBS_H
#define LBS_H

#include "latch_abi.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  LBS_PLANT_HZ = 1000,
  LBS_CONSIDER_EVERY = 50, /* 20 Hz, inside the 10–50 Hz band */
  LBS_HIST_CAP = 512,
  LBS_ORDER_CAP = 2048,
  LBS_NAME_CAP = 512
};

enum {
  LBS_EV_CONSIDER = 1,
  LBS_EV_REJECT = 2,
  LBS_EV_CONSUME = 3
};

enum {
  LBS_QPOS_MIN_MILLI = -1570, /* milliradians, joint range */
  LBS_QPOS_MAX_MILLI = 1570
};

typedef struct LbsHooks {
  void (*adjust_sense)(LatchSense *sense, int consider_index, void *user);
  void (*adjust_apply)(LatchSense *sense, int consider_index, void *user);
  void *user;
} LbsHooks;

typedef struct LbsElection {
  LatchState state;
  LatchBlob blob;
  const char *snapshot_id;
  LatchMode mode;
  LatchSense decision_sense;
  LatchSense apply_sense;
  char evidence[17];
  double setpoint;
  int have_setpoint;
  int last_leave;
  int consider_calls;
  int reject_calls;
  int consume_calls;
  int illegal;
  int budget_hit;
  int order[LBS_ORDER_CAP];
  int order_n;
  uint8_t names[LBS_NAME_CAP];
  int names_n;
  int64_t hist_us[LBS_HIST_CAP];
  int hist_n;
  LbsHooks hooks;
  const int *mj_guard;
  int *mj_violations;
} LbsElection;

typedef struct LbsPlant LbsPlant;

typedef struct LbsPlantStats {
  int steps;
  int considers;
  int rejects;
  int consumes;
  int mid_tick;
  int illegal;
  int stop_tripped;
  int stop_ctrl_overrides;
  int have_setpoint;
  int last_leave;
  double setpoint;
  double ctrl;
  double qvel;
  double qpos;
  double qfrc_applied;
  double timestep;
  uint8_t mode_name;
  uint8_t mode_plane;
  int32_t mode_dwell;
  int32_t mode_score;
  int32_t mode_margin;
  int hist_n;
  int64_t hist_p50_us;
  int64_t hist_p99_us;
  int64_t hist_max_us;
  int names_n;
  int order_n;
} LbsPlantStats;

const char *lbs_mode_name(unsigned name);
int lbs_mode_id(const char *text, unsigned *out);
const char *lbs_plane_name(unsigned plane);
int lbs_plane_id(const char *text, unsigned *out);

/* Position setpoint only. leave=1 means the name does not move the target
 * (yield / hold). There is no torque output. Returns 0. */
int lbs_mode_to_setpoint(unsigned name, double current, double *qpos_target, int *leave);

int lbs_election_init(LbsElection *election, const char *snapshot_id);
int lbs_election_tick(LbsElection *election, LatchSense sense);

void lbs_hist_summarize(const int64_t *samples, int n, int64_t *p50, int64_t *p99, int64_t *max_us);

/* Headless plant. MuJoCo timestep is 0.001 s. Returns NULL on failure. */
LbsPlant *lbs_plant_create(void);
void lbs_plant_destroy(LbsPlant *plant);
void lbs_plant_set_hooks(LbsPlant *plant, const LbsHooks *hooks);
void lbs_plant_set_budget(LbsPlant *plant, int budget_hit);
void lbs_plant_inject_stop(LbsPlant *plant);
void lbs_plant_poke_setpoint(LbsPlant *plant, double qpos_target);
int lbs_plant_run(LbsPlant *plant, int steps);
void lbs_plant_sense(const LbsPlant *plant, LatchSense *out);
void lbs_plant_stats(const LbsPlant *plant, LbsPlantStats *out);
int lbs_plant_copy_names(const LbsPlant *plant, uint8_t *dst, int cap);
int lbs_plant_copy_order(const LbsPlant *plant, int *dst, int cap);

#ifdef __cplusplus
}
#endif

#endif

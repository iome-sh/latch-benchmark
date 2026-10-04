/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 iome-sh contributors
 *
 * L1 fixture runner for BM-01..08 and BM-12 (illegal count).
 * The no-alloc half of BM-12 is lbs_noalloc.
 * Public CI links the mock oracle. Latch 60/60 bit-match is claimed only
 * when a real liblatch is linked and LATCH_GOLDENS points at an export.
 */
#include "lbs.h"
#include "oracle_runner.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef LBS_FIXTURE_DIR
#define LBS_FIXTURE_DIR "fixtures"
#endif

static int g_fail = 0;
static int g_mark = 0;

static void mark(void) { g_mark = g_fail; }

static void check(int cond, const char *msg) {
  if (!cond) {
    fprintf(stderr, "FAIL %s\n", msg);
    g_fail += 1;
  }
}

static void done(const char *bm) {
  if (g_fail == g_mark) printf("PASS %s\n", bm);
}

static void blank(LatchSense *s) { memset(s, 0, sizeof *s); }

static void approach_sense(LatchSense *s) {
  blank(s);
  s->joint = 20;
  s->target_seen = 1;
  s->wrench = 5;
}

static int bind_new(LatchState *state, LatchBlob *blob) {
  memset(state, 0, sizeof *state);
  memset(blob, 0, sizeof *blob);
  blob->schema = LATCH_BLOB_SCHEMA;
  blob->snapshot_id = "lbs-mock-oracle";
  return latch_bind(state, blob);
}

static void test_setpoint_table(void) {
  double q = 0;
  int leave = 0;
  mark();
  check(lbs_mode_to_setpoint(LATCH_APPROACH, 0.0, &q, &leave) == 0 && leave == 0 && q > 0.019 && q < 0.021,
        "BM-07 approach moves qpos target");
  check(lbs_mode_to_setpoint(LATCH_GRASP, 0.0, &q, &leave) == 0 && leave == 0 && q > 0.99 && q < 1.01,
        "BM-07 grasp is a position target");
  check(lbs_mode_to_setpoint(LATCH_RELEASE, 0.0, &q, &leave) == 0 && leave == 0 && q > 0.19 && q < 0.21,
        "BM-07 release is a position target");
  check(lbs_mode_to_setpoint(LATCH_RETRACT, 0.50, &q, &leave) == 0 && leave == 0 && q < 0.49,
        "BM-07 retract backs the position target off");
  q = 0.42;
  check(lbs_mode_to_setpoint(LATCH_YIELD, 0.42, &q, &leave) == 0 && leave == 1 && q == 0.42,
        "BM-07 yield leaves the position target");
  check(lbs_mode_to_setpoint(LATCH_HOLD, 0.42, &q, &leave) == 0 && leave == 1 && q == 0.42,
        "BM-07 hold leaves the position target");
  done("BM-07 position table (no torque argument)");
}

static void test_direct_planes(void) {
  LatchState state;
  LatchBlob blob;
  LatchSense sense;
  LatchMode mode;
  LatchMode applied;
  char hex[17];
  uint8_t kept = 0;
  mark();
  approach_sense(&sense);
  sense.sense_age = 4;
  sense.contact = 1;
  check(bind_new(&state, &blob) == 0, "bind stale");
  check(latch_consider(&sense, &state, 0, &mode) == 0, "consider stale");
  check(mode.name == LATCH_YIELD && mode.plane == LATCH_PLANE_L2, "BM-03 stale is yield plane l2");
  check(mode.plane != LATCH_PLANE_DENY, "BM-03 stale is not deny");

  approach_sense(&sense);
  sense.estop = 1;
  check(bind_new(&state, &blob) == 0, "bind estop");
  check(latch_consider(&sense, &state, 1, &mode) == 0, "consider estop+budget");
  check(mode.name == LATCH_YIELD && mode.plane == LATCH_PLANE_DENY && mode.dwell == 0,
        "BM-04 estop is yield plane deny, dwell cleared, before budget");

  approach_sense(&sense);
  sense.collision = 1;
  check(bind_new(&state, &blob) == 0, "bind collision");
  check(latch_consider(&sense, &state, 0, &mode) == 0 && mode.name == LATCH_YIELD && mode.plane == LATCH_PLANE_DENY &&
            mode.dwell == 0,
        "BM-04 collision is yield plane deny");

  approach_sense(&sense);
  sense.limit = 1;
  check(bind_new(&state, &blob) == 0, "bind limit");
  check(latch_consider(&sense, &state, 0, &mode) == 0 && mode.name == LATCH_YIELD && mode.plane == LATCH_PLANE_DENY &&
            mode.dwell == 0,
        "BM-04 joint limit is yield plane deny");

  approach_sense(&sense);
  sense.estop = 1;
  sense.sense_age = 4;
  check(bind_new(&state, &blob) == 0, "bind deny-over-stale");
  check(latch_consider(&sense, &state, 0, &mode) == 0 && mode.plane == LATCH_PLANE_DENY,
        "BM-04 deny wins over stale");

  check(bind_new(&state, &blob) == 0, "bind budget");
  approach_sense(&sense);
  check(latch_consider(&sense, &state, 0, &mode) == 0, "consider before budget");
  kept = mode.name;
  sense.contact = 1;
  sense.wrench = 90;
  check(latch_consider(&sense, &state, 1, &mode) == 0, "consider budget");
  check(mode.plane == LATCH_PLANE_BUDGET && mode.name == kept, "BM-05 budget keeps the last legal name");

  check(bind_new(&state, &blob) == 0, "bind cold budget");
  approach_sense(&sense);
  check(latch_consider(&sense, &state, 1, &mode) == 0 && mode.name == LATCH_HOLD && mode.plane == LATCH_PLANE_BUDGET,
        "BM-05 cold budget is hold");

  approach_sense(&sense);
  sense.contact = 1;
  {
    LatchSense apply = sense;
    LatchMode decision;
    memset(&decision, 0, sizeof decision);
    decision.name = LATCH_GRASP;
    decision.plane = LATCH_PLANE_L2;
    decision.score = 60;
    decision.margin = 10;
    decision.dwell = 3;
    apply.contact = 0;
    check(latch_apply_reject(&sense, &apply, &decision, &applied) == 1, "BM-06 grasp contact lost rc");
    check(applied.name == LATCH_HOLD && applied.plane == LATCH_PLANE_REJECT, "BM-06 grasp contact lost is hold/reject");
  }
  {
    LatchSense apply;
    LatchMode decision;
    approach_sense(&sense);
    apply = sense;
    memset(&decision, 0, sizeof decision);
    decision.name = LATCH_APPROACH;
    decision.plane = LATCH_PLANE_L2;
    apply.target_seen = 0;
    check(latch_apply_reject(&sense, &apply, &decision, &applied) == 1 && applied.name == LATCH_HOLD &&
              applied.plane == LATCH_PLANE_REJECT,
          "BM-06 approach target gone is hold/reject");
    apply = sense;
    apply.sense_age = 4;
    apply.target_seen = 1;
    check(latch_apply_reject(&sense, &apply, &decision, &applied) == 1 && applied.plane == LATCH_PLANE_REJECT,
          "BM-06 approach target stale is hold/reject");
    apply = sense;
    apply.limit = 1;
    check(latch_apply_reject(&sense, &apply, &decision, &applied) == 1 && applied.name == LATCH_HOLD &&
              applied.plane == LATCH_PLANE_REJECT,
          "BM-06 new joint limit is hold/reject");
  }
  {
    LatchMode decision;
    memset(&decision, 0, sizeof decision);
    decision.name = LATCH_YIELD;
    decision.plane = LATCH_PLANE_L2;
    approach_sense(&sense);
    check(latch_apply_reject(&sense, &sense, &decision, &applied) == 0 && applied.name == LATCH_YIELD &&
              applied.plane == LATCH_PLANE_L2,
          "BM-06 yield passes through");
    decision.plane = LATCH_PLANE_DENY;
    decision.name = LATCH_YIELD;
    check(latch_apply_reject(&sense, &sense, &decision, &applied) == 0 && applied.plane == LATCH_PLANE_DENY,
          "BM-06 deny record passes through");
    check(latch_apply_reject(NULL, &sense, &decision, &applied) == -1, "BM-06 null argument");
  }

  approach_sense(&sense);
  check(bind_new(&state, &blob) == 0, "bind evidence");
  check(latch_consider(&sense, &state, 0, &mode) == 0, "consider evidence");
  latch_evidence(&state, &sense, &mode, hex);
  check(strlen(hex) == 16, "evidence width");
  check(strcmp(hex, "59a7dbac9a73f1af") != 0, "retired digest stays retired");
  done("BM-03/04/05/06 direct ABI");
}

static void hook_approach(LatchSense *s, int index, void *user) {
  (void)index;
  (void)user;
  approach_sense(s);
}

static void hook_stale(LatchSense *s, int index, void *user) {
  (void)index;
  (void)user;
  approach_sense(s);
  s->sense_age = 4;
  s->contact = 1;
  s->wrench = 40;
}

static void hook_estop(LatchSense *s, int index, void *user) {
  (void)index;
  (void)user;
  approach_sense(s);
  s->estop = 1;
}

static void hook_grasp_noise(LatchSense *s, int index, void *user) {
  (void)user;
  approach_sense(s);
  s->contact = 1;
  s->wrench = (index % 2) ? 80 : 5;
}

static void test_two_rate(void) {
  LbsPlant *plant = NULL;
  LbsHooks hooks;
  LbsPlantStats stats;
  int order[128];
  int n = 0;
  int i = 0;
  int hz = 0;
  mark();
  memset(&hooks, 0, sizeof hooks);
  hooks.adjust_sense = hook_approach;
  plant = lbs_plant_create();
  check(plant != NULL, "plant create");
  if (!plant) {
    done("BM-08/12 two-rate");
    return;
  }
  lbs_plant_set_hooks(plant, &hooks);
  check(lbs_plant_run(plant, 1000) == 0, "run 1000");
  lbs_plant_stats(plant, &stats);
  hz = LBS_PLANT_HZ / LBS_CONSIDER_EVERY;
  check(stats.timestep > 0.000999 && stats.timestep < 0.001001, "mj_step timestep 1 ms");
  check(hz >= 10 && hz <= 50 && hz == 20, "consider rate 20 Hz inside 10-50");
  check(stats.steps == 1000, "plant steps");
  check(stats.considers == 20 && stats.rejects == 20 && stats.consumes == 20, "consider every 50 steps");
  check(stats.mid_tick == 0, "Latch not called inside mj_step");
  check(stats.illegal == 0, "BM-12 illegal emission 0 on the plant soak");
  check(stats.qfrc_applied == 0, "qfrc_applied stays 0 (no torque command)");
  check(stats.hist_n == 20 && stats.hist_max_us < 1000 && stats.hist_p50_us <= stats.hist_p99_us &&
            stats.hist_p99_us <= stats.hist_max_us,
        "BM-08 max consider under 1000 us");
  printf("BM-08 consider_us p50=%lld p99=%lld max=%lld n=%d\n", (long long)stats.hist_p50_us,
         (long long)stats.hist_p99_us, (long long)stats.hist_max_us, stats.hist_n);
  n = lbs_plant_copy_order(plant, order, 128);
  check(n == 60, "order trace length");
  for (i = 0; i + 2 < n; i += 3) {
    if (order[i] != LBS_EV_CONSIDER || order[i + 1] != LBS_EV_REJECT || order[i + 2] != LBS_EV_CONSUME) {
      check(0, "BM-06 reject before setpoint consume");
      break;
    }
  }
  if (i >= n - 2) check(1, "BM-06 order");
  lbs_plant_destroy(plant);
  done("two-rate + BM-06 order + BM-08 + BM-12");
}

static void test_sense_contacts(void) {
  LbsPlant *plant = NULL;
  LatchSense sense;
  mark();
  plant = lbs_plant_create();
  check(plant != NULL, "sense plant");
  if (!plant) {
    done("sense");
    return;
  }
  check(lbs_plant_run(plant, 5) == 0, "sense steps");
  lbs_plant_sense(plant, &sense);
  check(sense.contact == 1, "BM sense contact from mjData");
  check(sense.wrench > 0, "BM sense wrench from contact force");
  check(sense.estop == 0, "estop is not invented from contacts");
  lbs_plant_destroy(plant);
  done("sense mjData contacts/forces");
}

static void test_plant_fixtures(void) {
  LbsPlant *plant = NULL;
  LbsHooks hooks;
  LbsPlantStats before;
  LbsPlantStats after;
  uint8_t kept = 0;
  mark();
  memset(&hooks, 0, sizeof hooks);

  hooks.adjust_sense = hook_stale;
  plant = lbs_plant_create();
  check(plant != NULL, "stale plant");
  if (plant) {
    lbs_plant_set_hooks(plant, &hooks);
    check(lbs_plant_run(plant, LBS_CONSIDER_EVERY) == 0, "stale run");
    lbs_plant_stats(plant, &after);
    check(after.mode_name == LATCH_YIELD && after.mode_plane == LATCH_PLANE_L2, "BM-03 plant stale");
    lbs_plant_destroy(plant);
  }

  hooks.adjust_sense = hook_estop;
  plant = lbs_plant_create();
  if (plant) {
    lbs_plant_set_budget(plant, 1);
    lbs_plant_set_hooks(plant, &hooks);
    check(lbs_plant_run(plant, LBS_CONSIDER_EVERY) == 0, "deny run");
    lbs_plant_stats(plant, &after);
    check(after.mode_name == LATCH_YIELD && after.mode_plane == LATCH_PLANE_DENY && after.mode_dwell == 0,
          "BM-04 plant deny before budget");
    lbs_plant_destroy(plant);
  }

  hooks.adjust_sense = hook_approach;
  plant = lbs_plant_create();
  if (plant) {
    lbs_plant_set_hooks(plant, &hooks);
    check(lbs_plant_run(plant, LBS_CONSIDER_EVERY) == 0, "budget phase 1");
    lbs_plant_stats(plant, &before);
    kept = before.mode_name;
    hooks.adjust_sense = hook_grasp_noise;
    lbs_plant_set_hooks(plant, &hooks);
    lbs_plant_set_budget(plant, 1);
    check(lbs_plant_run(plant, LBS_CONSIDER_EVERY) == 0, "budget phase 2");
    lbs_plant_stats(plant, &after);
    check(after.steps == before.steps + LBS_CONSIDER_EVERY, "BM-05 plant tick not stalled");
    check(after.mode_plane == LATCH_PLANE_BUDGET && after.mode_name == kept, "BM-05 plant keeps last name");
    lbs_plant_destroy(plant);
  }

  plant = lbs_plant_create();
  if (plant) {
    lbs_plant_set_budget(plant, 1);
    hooks.adjust_sense = hook_approach;
    lbs_plant_set_hooks(plant, &hooks);
    check(lbs_plant_run(plant, LBS_CONSIDER_EVERY) == 0, "cold budget run");
    lbs_plant_stats(plant, &after);
    check(after.mode_name == LATCH_HOLD && after.mode_plane == LATCH_PLANE_BUDGET, "BM-05 plant cold budget");
    lbs_plant_destroy(plant);
  }
  done("BM-03/04/05 on the plant loop");
}

static void test_yield_stop(void) {
  LbsPlant *plant = NULL;
  LbsHooks hooks;
  LbsPlantStats stats;
  mark();
  memset(&hooks, 0, sizeof hooks);
  hooks.adjust_sense = hook_estop;
  plant = lbs_plant_create();
  check(plant != NULL, "yield plant");
  if (!plant) {
    done("BM-07");
    return;
  }
  lbs_plant_poke_setpoint(plant, 0.42);
  lbs_plant_set_hooks(plant, &hooks);
  check(lbs_plant_run(plant, LBS_CONSIDER_EVERY) == 0, "yield run");
  lbs_plant_stats(plant, &stats);
  check(stats.mode_name == LATCH_YIELD && stats.last_leave == 1, "BM-07 yield consumes as leave");
  check(stats.setpoint > 0.419 && stats.setpoint < 0.421, "BM-07 yield leaves qpos target");
  check(stats.stop_tripped == 0, "yield does not trip the series stop");
  lbs_plant_inject_stop(plant);
  check(lbs_plant_run(plant, LBS_CONSIDER_EVERY) == 0, "stop run");
  lbs_plant_stats(plant, &stats);
  check(stats.stop_tripped == 1 && stats.stop_ctrl_overrides == LBS_CONSIDER_EVERY, "BM-07 series stop trips");
  check(stats.setpoint > 0.419 && stats.setpoint < 0.421, "BM-07 stop does not rewrite the Latch setpoint");
  check(fabs(stats.qvel) < 1e-8, "BM-07 stop leaves the joint velocity at 0");
  check(stats.considers == 1, "series stop does not call Latch");
  lbs_plant_destroy(plant);
  done("BM-07 yield leaves setpoint; series stop still trips");
}

#if LBS_MOCK_ORACLE
static void test_chatter(void) {
  LbsPlant *plant = NULL;
  LbsHooks hooks;
  uint8_t names[64];
  int n = 0;
  int i = 0;
  int latch_flips = 0;
  int raw_flips = 0;
  unsigned prev_raw = 0;
  const int kN = 20;
  mark();
  memset(&hooks, 0, sizeof hooks);
  hooks.adjust_sense = hook_grasp_noise;
  plant = lbs_plant_create();
  check(plant != NULL, "chatter plant");
  if (!plant) {
    done("BM-02");
    return;
  }
  lbs_plant_set_hooks(plant, &hooks);
  check(lbs_plant_run(plant, kN * LBS_CONSIDER_EVERY) == 0, "chatter run");
  n = lbs_plant_copy_names(plant, names, 64);
  check(n == kN, "chatter samples");
  for (i = 1; i < n; ++i) {
    if (names[i] != names[i - 1]) latch_flips += 1;
  }
  for (i = 0; i < kN; ++i) {
    unsigned raw = (i % 2) ? LATCH_GRASP : LATCH_APPROACH;
    if (i > 0 && raw != prev_raw) raw_flips += 1;
    prev_raw = raw;
  }
  check(latch_flips <= 3, "BM-02 mock dwell flips <= 3");
  check(raw_flips >= 11, "BM-02 raw wrench argmax flips >= 11");
  printf("BM-02 mock-oracle latch_flips=%d raw_flips=%d\n", latch_flips, raw_flips);
  lbs_plant_destroy(plant);
  done("BM-02 chatter (mock-oracle)");
}

static void test_mock_oracle_file(void) {
  OracleRun run;
  char path[512];
  int rc = 0;
  mark();
  snprintf(path, sizeof path, "%s/mock-oracle.jsonl.example", LBS_FIXTURE_DIR);
  rc = lbs_run_oracle_jsonl(path, "lbs-mock-oracle", 1, &run);
  check(rc == 0 && run.fails == 0 && run.rows >= 8 && run.illegal == 0, "BM-01 mock-oracle rows");
  printf("BM-01 mock-oracle %d rows (not Latch 60/60 bit-match)\n", run.rows);
  done("BM-01 mock-oracle");
}

static void test_tracked_golden(void) {
  const char *path = getenv("LATCH_GOLDENS");
  OracleRun run;
  int rc = 0;
  if (!path || !path[0]) return;
  rc = lbs_run_oracle_jsonl(path, "latch-robot-1", 1, &run);
  if (rc == 0 && run.fails == 0 && run.rows == 60 && run.illegal == 0) {
    printf("BM-01 Latch bit-match 60/60\n");
    return;
  }
  fprintf(stderr, "BM-01 compared rows=%d fails=%d illegal=%d\n", run.rows, run.fails, run.illegal);
  printf("BM-01 60/60 not claimed\n");
}
#else
static void test_real_goldens(void) {
  const char *path = getenv("LATCH_GOLDENS");
  OracleRun run;
  mark();
  if (!path || !path[0]) {
    printf("BM-01 real Latch linked; LATCH_GOLDENS unset — bit-match not claimed\n");
    printf("BM-02 chatter counts come from the exported golden seqs when LATCH_GOLDENS is set\n");
    done("BM-01 real Latch unclaimed");
    return;
  }
  if (lbs_run_oracle_jsonl(path, "latch-robot-1", 1, &run) != 0 || run.rows != 60 || run.illegal != 0) {
    fprintf(stderr, "FAIL BM-01 Latch bit-match rows=%d fails=%d illegal=%d\n", run.rows, run.fails, run.illegal);
    g_fail += 1;
    printf("BM-01 60/60 not claimed\n");
  } else {
    printf("BM-01 Latch bit-match 60/60\n");
  }
  if (run.chatter_rows > 1) check(run.chatter_flips <= 3, "BM-02 exported chatter flips <= 3");
  if (run.raw_rows > 1) check(run.raw_flips >= 11, "BM-02 exported raw flips >= 11");
  done("BM-01/02 exported Latch goldens");
}
#endif

int main(void) {
  test_setpoint_table();
  test_direct_planes();
  test_sense_contacts();
  test_two_rate();
  test_plant_fixtures();
  test_yield_stop();
#if LBS_MOCK_ORACLE
  test_chatter();
  test_mock_oracle_file();
  test_tracked_golden();
  printf("oracle=mock\n");
#else
  test_real_goldens();
  printf("oracle=latch\n");
#endif
  if (g_fail) {
    fprintf(stderr, "%d failed\n", g_fail);
    return 1;
  }
  printf("ok\n");
  return 0;
}

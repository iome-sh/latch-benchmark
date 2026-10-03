/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 iome-sh contributors
 *
 * Single process, two rates. mj_step at 1 kHz. latch_consider every
 * LBS_CONSIDER_EVERY steps (20 Hz). Apply-reject runs before the position
 * setpoint is written to ctrl. The series stop zeros velocity and rewrites
 * ctrl from the measured qpos without asking Latch to move the setpoint.
 */
#include "lbs.h"

#include <mujoco/mujoco.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void lbs_sense_fill(const mjModel *model, const mjData *data, int hinge_id, LatchSense *out);

void lbs_plant_destroy(LbsPlant *plant);

static const char kModelXml[] =
    "<mujoco model=\"lbs_arm\">\n"
    "  <option timestep=\"0.001\" gravity=\"0 0 0\"/>\n"
    "  <worldbody>\n"
    "    <body name=\"arm\" pos=\"0 0 0\">\n"
    "      <joint name=\"hinge\" type=\"hinge\" axis=\"0 0 1\" limited=\"true\" range=\"-1.57 1.57\" damping=\"1\"/>\n"
    "      <geom name=\"link\" type=\"capsule\" fromto=\"0 0 0 0.2 0 0\" size=\"0.02\" mass=\"0.2\" contype=\"0\" conaffinity=\"0\"/>\n"
    "      <body name=\"tip\" pos=\"0.2 0 0\">\n"
    "        <geom name=\"tip\" type=\"sphere\" size=\"0.04\" mass=\"0.05\"/>\n"
    "      </body>\n"
    "    </body>\n"
    "    <body name=\"obstacle\" pos=\"0.2 0 0\">\n"
    "      <geom name=\"obstacle\" type=\"sphere\" size=\"0.04\"/>\n"
    "    </body>\n"
    "  </worldbody>\n"
    "  <actuator>\n"
    "    <position name=\"hinge_pos\" joint=\"hinge\" kp=\"40\" ctrlrange=\"-1.57 1.57\"/>\n"
    "  </actuator>\n"
    "</mujoco>\n";

struct LbsPlant {
  mjModel *model;
  mjData *data;
  mjSpec *spec;
  int hinge_id;
  int act_id;
  int dof_adr;
  int qpos_adr;
  int in_mj_step;
  int mid_tick_violations;
  int stop_tripped;
  int stop_ctrl_overrides;
  int budget_hit;
  int plant_steps;
  LbsElection election;
};

LbsPlant *lbs_plant_create(void) {
  LbsPlant *plant = NULL;
  char error[1024];
  plant = (LbsPlant *)calloc(1, sizeof *plant);
  if (!plant) return NULL;
  memset(error, 0, sizeof error);
  plant->spec = mj_parseXMLString(kModelXml, NULL, error, (int)sizeof error);
  if (!plant->spec) {
    fprintf(stderr, "mj_parseXMLString: %s\n", error);
    free(plant);
    return NULL;
  }
  plant->model = mj_compile(plant->spec, NULL);
  if (!plant->model) {
    fprintf(stderr, "mj_compile failed\n");
    mj_deleteSpec(plant->spec);
    free(plant);
    return NULL;
  }
  if (plant->model->opt.timestep < 0.000999 || plant->model->opt.timestep > 0.001001) {
    fprintf(stderr, "mujoco timestep %g is not 0.001\n", plant->model->opt.timestep);
    mj_deleteModel(plant->model);
    mj_deleteSpec(plant->spec);
    free(plant);
    return NULL;
  }
  plant->data = mj_makeData(plant->model);
  if (!plant->data) {
    fprintf(stderr, "mj_makeData failed\n");
    mj_deleteModel(plant->model);
    mj_deleteSpec(plant->spec);
    free(plant);
    return NULL;
  }
  plant->hinge_id = mj_name2id(plant->model, mjOBJ_JOINT, "hinge");
  plant->act_id = mj_name2id(plant->model, mjOBJ_ACTUATOR, "hinge_pos");
  if (plant->hinge_id < 0 || plant->act_id < 0) {
    fprintf(stderr, "hinge or position actuator missing\n");
    lbs_plant_destroy(plant);
    return NULL;
  }
  plant->dof_adr = plant->model->jnt_dofadr[plant->hinge_id];
  plant->qpos_adr = plant->model->jnt_qposadr[plant->hinge_id];
  if (lbs_election_init(&plant->election, "lbs-mock-oracle") != 0) {
    fprintf(stderr, "latch_bind failed\n");
    lbs_plant_destroy(plant);
    return NULL;
  }
  plant->election.mj_guard = &plant->in_mj_step;
  plant->election.mj_violations = &plant->mid_tick_violations;
  mj_forward(plant->model, plant->data);
  return plant;
}

void lbs_plant_destroy(LbsPlant *plant) {
  if (!plant) return;
  if (plant->data) mj_deleteData(plant->data);
  if (plant->model) mj_deleteModel(plant->model);
  if (plant->spec) mj_deleteSpec(plant->spec);
  free(plant);
}

void lbs_plant_set_hooks(LbsPlant *plant, const LbsHooks *hooks) {
  if (!plant) return;
  if (!hooks) {
    memset(&plant->election.hooks, 0, sizeof plant->election.hooks);
    return;
  }
  plant->election.hooks = *hooks;
}

void lbs_plant_set_budget(LbsPlant *plant, int budget_hit) {
  if (!plant) return;
  plant->budget_hit = budget_hit ? 1 : 0;
  plant->election.budget_hit = plant->budget_hit;
}

void lbs_plant_inject_stop(LbsPlant *plant) {
  if (!plant) return;
  plant->stop_tripped = 1;
}

void lbs_plant_poke_setpoint(LbsPlant *plant, double qpos_target) {
  if (!plant) return;
  plant->election.setpoint = qpos_target;
  plant->election.have_setpoint = 1;
  plant->data->ctrl[plant->act_id] = qpos_target;
}

int lbs_plant_run(LbsPlant *plant, int steps) {
  int i = 0;
  if (!plant || steps < 0) return 1;
  for (i = 0; i < steps; ++i) {
    if (!plant->stop_tripped && (plant->plant_steps % LBS_CONSIDER_EVERY) == 0) {
      LatchSense sense;
      int rc = 0;
      if (plant->in_mj_step) plant->mid_tick_violations += 1;
      lbs_sense_fill(plant->model, plant->data, plant->hinge_id, &sense);
      plant->election.budget_hit = plant->budget_hit;
      rc = lbs_election_tick(&plant->election, sense);
      if (rc != 0) return rc;
      if (plant->election.have_setpoint) plant->data->ctrl[plant->act_id] = plant->election.setpoint;
    }
    plant->in_mj_step = 1;
    mj_step(plant->model, plant->data);
    plant->in_mj_step = 0;
    if (plant->stop_tripped) {
      plant->data->qvel[plant->dof_adr] = 0;
      plant->data->ctrl[plant->act_id] = plant->data->qpos[plant->qpos_adr];
      plant->stop_ctrl_overrides += 1;
    }
    plant->plant_steps += 1;
  }
  return 0;
}

void lbs_plant_sense(const LbsPlant *plant, LatchSense *out) {
  if (!plant || !out) return;
  lbs_sense_fill(plant->model, plant->data, plant->hinge_id, out);
}

void lbs_plant_stats(const LbsPlant *plant, LbsPlantStats *out) {
  if (!out) return;
  memset(out, 0, sizeof *out);
  if (!plant) return;
  out->steps = plant->plant_steps;
  out->considers = plant->election.consider_calls;
  out->rejects = plant->election.reject_calls;
  out->consumes = plant->election.consume_calls;
  out->mid_tick = plant->mid_tick_violations;
  out->illegal = plant->election.illegal;
  out->stop_tripped = plant->stop_tripped;
  out->stop_ctrl_overrides = plant->stop_ctrl_overrides;
  out->have_setpoint = plant->election.have_setpoint;
  out->last_leave = plant->election.last_leave;
  out->setpoint = plant->election.setpoint;
  out->ctrl = plant->data->ctrl[plant->act_id];
  out->qvel = plant->data->qvel[plant->dof_adr];
  out->qpos = plant->data->qpos[plant->qpos_adr];
  out->qfrc_applied = plant->data->qfrc_applied[plant->dof_adr];
  out->timestep = plant->model->opt.timestep;
  out->mode_name = plant->election.mode.name;
  out->mode_plane = plant->election.mode.plane;
  out->mode_dwell = plant->election.mode.dwell;
  out->mode_score = plant->election.mode.score;
  out->mode_margin = plant->election.mode.margin;
  out->hist_n = plant->election.hist_n;
  lbs_hist_summarize(plant->election.hist_us, plant->election.hist_n, &out->hist_p50_us, &out->hist_p99_us,
                     &out->hist_max_us);
  out->names_n = plant->election.names_n;
  out->order_n = plant->election.order_n;
}

int lbs_plant_copy_names(const LbsPlant *plant, uint8_t *dst, int cap) {
  int n = 0;
  if (!plant || !dst || cap <= 0) return 0;
  n = plant->election.names_n < cap ? plant->election.names_n : cap;
  memcpy(dst, plant->election.names, (size_t)n);
  return n;
}

int lbs_plant_copy_order(const LbsPlant *plant, int *dst, int cap) {
  int n = 0;
  if (!plant || !dst || cap <= 0) return 0;
  n = plant->election.order_n < cap ? plant->election.order_n : cap;
  memcpy(dst, plant->election.order, (size_t)n * sizeof *dst);
  return n;
}

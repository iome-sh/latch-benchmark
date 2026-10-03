/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 iome-sh contributors
 *
 * Sense from mjData contacts and contact forces.
 * estop is not a MuJoCo field: the series stop stays outside this function.
 * The collision deny bit is left 0 here; a fixture or a later binding sets it.
 * Contact force is reported as wrench 0–100.
 */
#include "lbs.h"

#include <math.h>
#include <mujoco/mujoco.h>
#include <string.h>

void lbs_sense_fill(const mjModel *model, const mjData *data, int hinge_id, LatchSense *out) {
  int adr = 0;
  double q = 0;
  double lo = 0;
  double hi = 0;
  double span = 0;
  double fn = 0;
  int i = 0;
  int joint = 0;
  if (!out) return;
  memset(out, 0, sizeof *out);
  if (!model || !data || hinge_id < 0) return;

  adr = model->jnt_qposadr[hinge_id];
  q = data->qpos[adr];
  lo = model->jnt_range[2 * hinge_id];
  hi = model->jnt_range[2 * hinge_id + 1];
  span = hi - lo;
  if (span > 1e-9) {
    joint = (int)((q - lo) / span * 100.0);
    if (joint < 0) joint = 0;
    if (joint > 100) joint = 100;
  }
  out->joint = joint;
  if (q <= lo + 1e-4 || q >= hi - 1e-4) out->limit = 1;

  for (i = 0; i < data->ncon; ++i) {
    mjtNum force[6];
    memset(force, 0, sizeof force);
    mj_contactForce(model, data, i, force);
    fn += fabs(force[0]);
  }
  out->contact = data->ncon > 0 ? 1 : 0;
  if (fn < 0) fn = 0;
  if (fn > 5.0) fn = 5.0;
  out->wrench = (int32_t)(fn / 5.0 * 100.0);
  if (out->contact && out->wrench < 1 && fn > 0) out->wrench = 1;
  out->target_seen = mj_name2id(model, mjOBJ_GEOM, "obstacle") >= 0 ? 1 : 0;
  out->sense_age = 0;
  out->grasped = 0;
  out->estop = 0;
  out->collision = 0;
}

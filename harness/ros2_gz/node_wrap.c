/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 iome-sh contributors
 *
 * Node wrap for the partner path. The plant step does not elect.
 * Election is every 50 plant steps. An independent stop does not read
 * the mode name. Public link is the mock oracle, not a 60-row certificate.
 */
#include "latch_abi.h"

#include <stdio.h>
#include <string.h>

enum { kElectEvery = 50 };

typedef struct NodeWrap {
  LatchState state;
  LatchMode record;
  int has_record;
  int plant_steps;
  int joint;
  int tripped;
} NodeWrap;

static void node_elect(NodeWrap *node, const LatchSense *sense) {
  LatchMode decided;
  latch_consider(sense, &node->state, 0, &decided);
  latch_apply_reject(sense, sense, &decided, &node->record);
  node->has_record = 1;
}

/* Plant step. Does not call latch_consider. */
static void node_plant(NodeWrap *node, const LatchSense *sense) {
  node->plant_steps += 1;
  if ((node->plant_steps % kElectEvery) == 0) node_elect(node, sense);
  else if (node->has_record && node->record.name == LATCH_APPROACH) node->joint += 1;
}

/* Independent stop. Does not read the mode and does not elect. */
static void node_stop(NodeWrap *node) { node->tripped = 1; }

static int node_read_name(const NodeWrap *node, uint8_t *name) {
  if (!node || !name || !node->has_record) return 1;
  *name = node->record.name;
  return 0;
}

int main(void) {
  NodeWrap node;
  memset(&node, 0, sizeof node);
  const LatchBlob blob = {LATCH_BLOB_SCHEMA, 0, "lbs-mock-oracle"};
  if (latch_bind(&node.state, &blob) != 0) {
    fprintf(stderr, "FAIL bind\n");
    return 1;
  }

  LatchSense approach = {20, 0, 0, 0, 0, 0, 1, 0, 5};
  int i;
  for (i = 0; i < kElectEvery; ++i) node_plant(&node, &approach);
  if (!node.has_record) {
    fprintf(stderr, "FAIL no record\n");
    return 1;
  }
  uint8_t name = 0xff;
  uint8_t again = 0xff;
  NodeWrap copy = node;
  if (node_read_name(&node, &name) != 0 || node_read_name(&node, &again) != 0) return 1;
  if (name != again || memcmp(&node, &copy, sizeof node) != 0) {
    fprintf(stderr, "FAIL condition wrote back\n");
    return 1;
  }

  /* Gone target: reject the approach precondition before the plant reads the name. */
  LatchSense gone = approach;
  gone.target_seen = 0;
  if (node.record.name != LATCH_APPROACH) {
    fprintf(stderr, "FAIL approach record\n");
    return 1;
  }
  LatchMode rejected;
  memset(&rejected, 0, sizeof rejected);
  if (latch_apply_reject(&approach, &gone, &node.record, &rejected) != 1 ||
      rejected.name != LATCH_HOLD || rejected.plane != LATCH_PLANE_REJECT) {
    fprintf(stderr, "FAIL apply-reject\n");
    return 1;
  }
  node.record = rejected;
  const int joint_held = node.joint;
  node_plant(&node, &gone);
  if (node.plant_steps != kElectEvery + 1 || node.joint != joint_held ||
      node.record.name != LATCH_HOLD) {
    fprintf(stderr, "FAIL hold moved the joint\n");
    return 1;
  }
  printf("apply-reject before consume\n");
  printf("target gone holds\n");

  /* Grasp whose contact died: reject before the plant reads the name. */
  NodeWrap hand;
  memset(&hand, 0, sizeof hand);
  if (latch_bind(&hand.state, &blob) != 0) {
    fprintf(stderr, "FAIL hand bind\n");
    return 1;
  }
  LatchSense high = {20, 0, 0, 0, 1, 0, 1, 0, 60};
  for (i = 0; i < kElectEvery; ++i) node_plant(&hand, &high);
  if (hand.record.name != LATCH_GRASP) {
    fprintf(stderr, "FAIL grasp record\n");
    return 1;
  }
  LatchSense lost = high;
  lost.contact = 0;
  memset(&rejected, 0, sizeof rejected);
  if (latch_apply_reject(&high, &lost, &hand.record, &rejected) != 1 ||
      rejected.name != LATCH_HOLD || rejected.plane != LATCH_PLANE_REJECT) {
    fprintf(stderr, "FAIL grasp apply-reject\n");
    return 1;
  }
  hand.record = rejected;
  const int hand_joint = hand.joint;
  node_plant(&hand, &lost);
  if (hand.plant_steps != kElectEvery + 1 || hand.joint != hand_joint ||
      hand.record.name != LATCH_HOLD) {
    fprintf(stderr, "FAIL grasp hold moved the joint\n");
    return 1;
  }
  printf("grasp contact lost holds\n");

  const int joint_before = node.joint;
  node.record.name = LATCH_YIELD;
  node_plant(&node, &approach);
  if (node.joint != joint_before) {
    fprintf(stderr, "FAIL yield moved the joint\n");
    return 1;
  }

  node.record.name = 9;
  node_stop(&node);
  if (!node.tripped) {
    fprintf(stderr, "FAIL stop\n");
    return 1;
  }

  printf("node-wrap mock-oracle\n");
  return 0;
}

// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 iome-sh contributors
//
// Links the in-tree mock oracle. Elects approach, rejects that decision
// because the target is gone, and publishes the rejected name (hold).
// This is not Latch. No torque, no current, no trajectory, no joint motion.
// Does not call latch_consider again after the reject.

#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

#include "latch_abi.h"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/u_int8.hpp"

namespace {

static_assert(LATCH_APPROACH == 0, "approach name is 0");
static_assert(LATCH_HOLD == 5, "hold name is 5");
static_assert(LATCH_PLANE_L2 == 3, "plane l2 is 3");
static_assert(LATCH_PLANE_REJECT == 4, "plane reject is 4");

int fail(const char *msg) {
  std::fprintf(stderr, "latch_apply_publish: %s\n", msg);
  return 1;
}

}  // namespace

int main(int argc, char **argv) {
  LatchState state;
  std::memset(&state, 0, sizeof state);
  const LatchBlob blob = {LATCH_BLOB_SCHEMA, 0, "lbs-mock-oracle"};
  if (latch_bind(&state, &blob) != 0) return fail("bind");

  /* joint, limit, collision, estop, contact, grasped, target_seen, sense_age, wrench. */
  LatchSense approach = {20, 0, 0, 0, 0, 0, 1, 0, 5};
  LatchMode mode;
  std::memset(&mode, 0, sizeof mode);
  if (latch_consider(&approach, &state, 0, &mode) != 0 || mode.name != LATCH_APPROACH ||
      mode.score != 60 || mode.margin != 20 || mode.plane != LATCH_PLANE_L2) {
    return fail("consider does not match approach");
  }

  LatchSense gone = approach;
  gone.target_seen = 0;
  LatchMode rejected;
  std::memset(&rejected, 0, sizeof rejected);
  if (latch_apply_reject(&approach, &gone, &mode, &rejected) != 1 || rejected.name != LATCH_HOLD ||
      rejected.plane != LATCH_PLANE_REJECT) {
    return fail("apply-reject did not hold");
  }

  rclcpp::init(argc, argv);
  auto node = std::make_shared<rclcpp::Node>("latch_apply_publish");
  auto publisher = node->create_publisher<std_msgs::msg::UInt8>("/latch/mode_name", 10);
  std_msgs::msg::UInt8 msg;
  msg.data = rejected.name;
  for (int i = 0; i < 4; ++i) {
    publisher->publish(msg);
    rclcpp::spin_some(node);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  std::puts("apply-reject before consume");
  std::puts("target gone holds");
  std::puts("not Latch");
  std::puts("L3 stays open");
  rclcpp::shutdown();
  return 0;
}

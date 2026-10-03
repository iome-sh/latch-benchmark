// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 iome-sh contributors
//
// Links the in-tree mock oracle and publishes one mode record.
// This is not Latch. No torque, no trajectory, no joint motion, no controller.
// The record is not evidence payload v2. This process does not elect from it.

#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

#include "latch_abi.h"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

namespace {

static_assert(LATCH_APPROACH == 0, "approach name is 0");
static_assert(LATCH_PLANE_L2 == 3, "plane l2 is 3");

int fail(const char *msg) {
  std::fprintf(stderr, "latch_mode_record: %s\n", msg);
  return 1;
}

int hex_width(const char *hex) {
  int n = 0;
  if (!hex) return -1;
  for (n = 0; n < 16; ++n) {
    const char c = hex[n];
    const int digit = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    if (!digit) return -1;
  }
  if (hex[16] != '\0') return -1;
  return n;
}

}  // namespace

int main(int argc, char **argv) {
  LatchState state;
  std::memset(&state, 0, sizeof state);
  const LatchBlob blob = {LATCH_BLOB_SCHEMA, 0, "lbs-mock-oracle"};
  if (latch_bind(&state, &blob) != 0) return fail("bind");

  /* joint, limit, collision, estop, contact, grasped, target_seen, sense_age, wrench. */
  LatchSense sense = {20, 0, 0, 0, 0, 0, 1, 0, 5};
  LatchMode mode;
  std::memset(&mode, 0, sizeof mode);
  if (latch_consider(&sense, &state, 0, &mode) != 0 || mode.name != LATCH_APPROACH || mode.score != 60 ||
      mode.margin != 20 || mode.plane != LATCH_PLANE_L2) {
    return fail("consider does not match approach/60/20/plane 3");
  }

  char hex[17];
  std::memset(hex, 0, sizeof hex);
  latch_evidence(&state, &sense, &mode, hex);
  const int width = hex_width(hex);
  if (width != 16) return fail("evidence is not 16 hex digits");

  char record[160];
  const int wrote =
      std::snprintf(record, sizeof record, "name=%u score=%d margin=%d plane=%u evidence=%s snapshot=%s",
                    static_cast<unsigned>(mode.name), static_cast<int>(mode.score),
                    static_cast<int>(mode.margin), static_cast<unsigned>(mode.plane), hex, blob.snapshot_id);
  if (wrote < 0 || static_cast<size_t>(wrote) >= sizeof record) return fail("record format");

  rclcpp::init(argc, argv);
  auto node = std::make_shared<rclcpp::Node>("latch_mode_record");
  auto publisher = node->create_publisher<std_msgs::msg::String>("/latch/mode_record", 10);
  std_msgs::msg::String msg;
  msg.data = record;
  for (int i = 0; i < 4; ++i) {
    publisher->publish(msg);
    rclcpp::spin_some(node);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  std::puts("mode-record published");
  std::printf("name=%u score=%d margin=%d plane=%u\n", static_cast<unsigned>(mode.name),
              static_cast<int>(mode.score), static_cast<int>(mode.margin), static_cast<unsigned>(mode.plane));
  std::printf("evidence-width %d\n", width);
  std::puts("not evidence payload v2");
  std::puts("not Latch");
  std::puts("condition does not elect");
  std::puts("L3 stays open");
  rclcpp::shutdown();
  return 0;
}

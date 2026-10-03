// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 iome-sh contributors
//
// Links the in-tree mock oracle and publishes the elected name.
// This is not Latch. No torque, no trajectory, no controller.

#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

#include "latch_abi.h"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/u_int8.hpp"

namespace {

int fail(const char *msg) {
  std::fprintf(stderr, "latch_mock_elect: %s\n", msg);
  return 1;
}

}  // namespace

int publish_elected(const rclcpp::Publisher<std_msgs::msg::UInt8>::SharedPtr &publisher,
                   const rclcpp::Node::SharedPtr &node, LatchState *state, LatchSense *sense,
                   uint8_t expect, int duration_ms) {
  const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(duration_ms);
  while (std::chrono::steady_clock::now() < end) {
    LatchMode elected;
    std::memset(&elected, 0, sizeof elected);
    if (latch_consider(sense, state, 0, &elected) != 0 || elected.name != expect) return 1;
    std_msgs::msg::UInt8 msg;
    msg.data = elected.name;
    publisher->publish(msg);
    rclcpp::spin_some(node);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  return 0;
}

int main(int argc, char **argv) {
  bool drive_sim = false;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--drive-sim") == 0) drive_sim = true;
  }

  LatchState state;
  std::memset(&state, 0, sizeof state);
  const LatchBlob blob = {LATCH_BLOB_SCHEMA, 0, "lbs-mock-oracle"};
  if (latch_bind(&state, &blob) != 0) return fail("bind");

  /* joint, limit, collision, estop, contact, grasped, target_seen, sense_age, wrench.
   * Wrench stays under 50 so the mock rank for approach is 60. */
  LatchSense sense = {20, 0, 0, 0, 0, 0, 1, 0, 5};
  LatchMode mode;
  std::memset(&mode, 0, sizeof mode);
  if (latch_consider(&sense, &state, 0, &mode) != 0) return fail("consider");
  if (mode.name != LATCH_APPROACH) return fail("name is not approach");

  rclcpp::init(argc, argv);
  auto node = std::make_shared<rclcpp::Node>("latch_mock_elect");
  auto publisher = node->create_publisher<std_msgs::msg::UInt8>("/latch/mode_name", 10);

  if (drive_sim) {
    if (publish_elected(publisher, node, &state, &sense, LATCH_APPROACH, 4000) != 0) {
      rclcpp::shutdown();
      return fail("drive-sim left approach");
    }
    /* Contact plus a high wrench ranks grasp over approach by 10, under the
     * switch margin. The same state keeps approach for the three dwell ticks. */
    sense.contact = 1;
    sense.wrench = 60;
    for (int tick = 0; tick < 3; ++tick) {
      LatchMode elected;
      std::memset(&elected, 0, sizeof elected);
      if (latch_consider(&sense, &state, 0, &elected) != 0 || elected.name != LATCH_APPROACH ||
          elected.plane != LATCH_PLANE_DWELL) {
        rclcpp::shutdown();
        return fail("drive-sim contact dwell left approach");
      }
      std_msgs::msg::UInt8 msg;
      msg.data = elected.name;
      publisher->publish(msg);
      rclcpp::spin_some(node);
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::puts("contact dwell keeps approach");
    {
      LatchMode elected;
      std::memset(&elected, 0, sizeof elected);
      if (latch_consider(&sense, &state, 0, &elected) != 0 || elected.name != LATCH_GRASP) {
        rclcpp::shutdown();
        return fail("drive-sim dwell did not elect grasp");
      }
    }
    std::puts("dwell expired elects grasp");
    if (publish_elected(publisher, node, &state, &sense, LATCH_GRASP, 1500) != 0) {
      rclcpp::shutdown();
      return fail("drive-sim left grasp");
    }
    sense.estop = 1;
    if (publish_elected(publisher, node, &state, &sense, LATCH_YIELD, 2000) != 0) {
      rclcpp::shutdown();
      return fail("drive-sim did not elect yield");
    }
    std::puts("drive-sim approach, dwell, grasp, then yield");
  } else {
    std_msgs::msg::UInt8 msg;
    msg.data = mode.name;
    for (int i = 0; i < 4; ++i) {
      publisher->publish(msg);
      rclcpp::spin_some(node);
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    sense.estop = 1;
    if (latch_consider(&sense, &state, 0, &mode) != 0 || mode.name != LATCH_YIELD) {
      rclcpp::shutdown();
      return fail("estop did not elect yield");
    }
  }

  std::puts("estop elects yield");
  std::puts("mock-oracle elected approach");
  std::puts("not Latch");
  std::puts("condition does not elect");
  rclcpp::shutdown();
  return 0;
}

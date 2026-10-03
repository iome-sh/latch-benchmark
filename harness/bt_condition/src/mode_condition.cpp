// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 iome-sh contributors

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "behaviortree_cpp/bt_factory.h"
#include "behaviortree_ros2/bt_topic_sub_node.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/u_int8.hpp"

namespace {

class LatchModeIs : public BT::RosTopicSubNode<std_msgs::msg::UInt8> {
 public:
  LatchModeIs(const std::string &name, const BT::NodeConfig &conf, const BT::RosNodeParams &params)
      : BT::RosTopicSubNode<std_msgs::msg::UInt8>(name, conf, params) {}

  static BT::PortsList providedPorts() {
    return providedBasicPorts({BT::InputPort<int>("expected")});
  }

  bool latchLastMessage() const override { return true; }

  BT::NodeStatus onTick(const std::shared_ptr<std_msgs::msg::UInt8> &last) override {
    if (!last) return BT::NodeStatus::FAILURE;
    int expected = 0;
    if (!getInput("expected", expected)) return BT::NodeStatus::FAILURE;
    if (last->data == static_cast<uint8_t>(expected)) return BT::NodeStatus::SUCCESS;
    return BT::NodeStatus::FAILURE;
  }
};

void fail(const std::string &msg) {
  std::cerr << "bt-condition: " << msg << "\n";
  std::exit(1);
}

BT::Tree make_tree(BT::BehaviorTreeFactory &factory, const char *xml) {
  return factory.createTreeFromText(xml);
}

}  // namespace

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<rclcpp::Node>("latch_bench_bt");
  auto publisher = node->create_publisher<std_msgs::msg::UInt8>("/latch/mode_name", 10);

  BT::BehaviorTreeFactory factory;
  BT::RosNodeParams params;
  params.nh = node;
  params.default_port_value = "/latch/mode_name";
  factory.registerNodeType<LatchModeIs>("LatchModeIs", params);

  const char *match_xml =
      R"(<root BTCPP_format="4">
           <BehaviorTree ID="Main">
             <LatchModeIs expected="0" topic_name="/latch/mode_name"/>
           </BehaviorTree>
         </root>)";
  const char *miss_xml =
      R"(<root BTCPP_format="4">
           <BehaviorTree ID="Main">
             <LatchModeIs expected="1" topic_name="/latch/mode_name"/>
           </BehaviorTree>
         </root>)";

  auto match = make_tree(factory, match_xml);
  std_msgs::msg::UInt8 msg;
  msg.data = 0;
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  BT::NodeStatus matched = BT::NodeStatus::FAILURE;
  while (std::chrono::steady_clock::now() < deadline) {
    publisher->publish(msg);
    matched = match.tickOnce();
    if (matched == BT::NodeStatus::SUCCESS) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  if (matched != BT::NodeStatus::SUCCESS) fail("expected name 0 did not succeed");

  auto miss = make_tree(factory, miss_xml);
  publisher->publish(msg);
  if (miss.tickOnce() != BT::NodeStatus::FAILURE) fail("expected name 1 did not fail");

  const auto infos = node->get_publishers_info_by_topic("/latch/mode_name");
  if (infos.size() != 1) fail("mode topic publisher count is " + std::to_string(infos.size()));

  std::cout << "BT.ROS2 condition read-only\n";
  std::cout << "condition does not elect\n";
  rclcpp::shutdown();
  return 0;
}

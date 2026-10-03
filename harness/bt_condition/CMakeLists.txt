cmake_minimum_required(VERSION 3.16)
project(latch_bench_bt)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(behaviortree_cpp REQUIRED)
find_package(behaviortree_ros2 REQUIRED)

add_executable(latch_bt_condition src/mode_condition.cpp)
ament_target_dependencies(latch_bt_condition
  rclcpp
  std_msgs
  behaviortree_cpp
  behaviortree_ros2
)
target_compile_options(latch_bt_condition PRIVATE -Wall -Wextra -Wpedantic -Werror)

install(TARGETS latch_bt_condition DESTINATION lib/${PROJECT_NAME})

ament_package()

# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.16)
project(latch_bench_mock_elect LANGUAGES C CXX)

# Publishes the mock-oracle mode name. Compiles the in-tree mock; does not copy it.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(latch_mock_elect
  src/latch_mock_elect.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../mujoco/src/latch_mock.c
)
target_include_directories(latch_mock_elect PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../mujoco/include
)
ament_target_dependencies(latch_mock_elect rclcpp std_msgs)
target_compile_options(latch_mock_elect PRIVATE -Wall -Wextra -Wpedantic -Werror)

# One mode-record string from the same mock. Does not elect from that record.
add_executable(latch_mode_record
  src/latch_mode_record.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../mujoco/src/latch_mock.c
)
target_include_directories(latch_mode_record PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../mujoco/include
)
ament_target_dependencies(latch_mode_record rclcpp std_msgs)
target_compile_options(latch_mode_record PRIVATE -Wall -Wextra -Wpedantic -Werror)

# Rejects a gone approach target and publishes hold. Does not consider again.
add_executable(latch_apply_publish
  src/latch_apply_publish.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../mujoco/src/latch_mock.c
)
target_include_directories(latch_apply_publish PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../mujoco/include
)
ament_target_dependencies(latch_apply_publish rclcpp std_msgs)
target_compile_options(latch_apply_publish PRIVATE -Wall -Wextra -Wpedantic -Werror)

install(TARGETS latch_mock_elect latch_mode_record latch_apply_publish DESTINATION lib/${PROJECT_NAME})

ament_package()

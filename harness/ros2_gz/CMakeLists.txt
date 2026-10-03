cmake_minimum_required(VERSION 3.16)
project(latch_bench_node_wrap LANGUAGES C)

# Partner-path node wrap. No ROS and no gz-sim. Links the in-tree mock oracle.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(lbs_node_wrap
  node_wrap.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../mujoco/src/latch_mock.c
)
target_include_directories(lbs_node_wrap PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../mujoco/include
)
target_compile_options(lbs_node_wrap PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_compile_definitions(lbs_node_wrap PRIVATE LBS_MOCK_ORACLE=1)

enable_testing()
add_test(NAME lbs_node_wrap COMMAND lbs_node_wrap)

cmake_minimum_required(VERSION 3.20)
project(latch_bench_mujoco LANGUAGES C)

# L1 MuJoCo harness. Default configure builds and, on `cmake --build`, runs
# the fixture binaries. Latch is not vendored: find_library under LATCH_LIB_DIR
# or LATCH_LIBRARY, otherwise compile latch_mock.c and label results mock-oracle.
# MuJoCo (Apache-2.0) is located via MUJOCO_DIR or fetched as a release tarball.
# The tarball is not committed.

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(LATCH_BENCH_BUILD_MUJOCO "Build MuJoCo harness targets" ON)
option(LATCH_BENCH_ASAN "Address and undefined sanitizers" OFF)

if(NOT LATCH_BENCH_BUILD_MUJOCO)
  message(STATUS "latch-bench mujoco: sources present, targets skipped (LATCH_BENCH_BUILD_MUJOCO=OFF)")
  return()
endif()

if(LATCH_BENCH_ASAN)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

set(MUJOCO_DIR "" CACHE PATH "Apache-2.0 MuJoCo prefix (include/ and lib/)")
if(NOT MUJOCO_DIR AND DEFINED ENV{MUJOCO_DIR})
  set(MUJOCO_DIR "$ENV{MUJOCO_DIR}")
endif()

set(MUJOCO_VERSION "3.14.0")
set(MUJOCO_SHA256 "326f0da78a7767cc18fab7205c993a771b5f19eda913648f958ee3f625240944")

if(MUJOCO_DIR)
  set(MUJOCO_INCLUDE "${MUJOCO_DIR}/include")
  find_library(MUJOCO_LIBRARY NAMES mujoco PATHS "${MUJOCO_DIR}/lib" NO_DEFAULT_PATH REQUIRED)
else()
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
    message(FATAL_ERROR
      "MuJoCo auto-fetch supports linux-x86_64. Set MUJOCO_DIR to a local Apache-2.0 install.")
  endif()
  set(MUJOCO_SRC "${CMAKE_BINARY_DIR}/_deps/mujoco-${MUJOCO_VERSION}")
  set(MUJOCO_TAR "${CMAKE_BINARY_DIR}/_deps/mujoco-${MUJOCO_VERSION}-linux-x86_64.tar.gz")
  if(NOT EXISTS "${MUJOCO_SRC}/include/mujoco/mujoco.h")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/_deps")
    message(STATUS "Fetching MuJoCo ${MUJOCO_VERSION} (Apache-2.0, not vendored)")
    file(DOWNLOAD
      "https://github.com/google-deepmind/mujoco/releases/download/${MUJOCO_VERSION}/mujoco-${MUJOCO_VERSION}-linux-x86_64.tar.gz"
      "${MUJOCO_TAR}"
      EXPECTED_HASH "SHA256=${MUJOCO_SHA256}"
      TIMEOUT 180
      STATUS MUJOCO_DL
    )
    list(GET MUJOCO_DL 0 MUJOCO_DL_CODE)
    if(NOT MUJOCO_DL_CODE EQUAL 0)
      message(FATAL_ERROR "MuJoCo download failed: ${MUJOCO_DL}")
    endif()
    file(ARCHIVE_EXTRACT INPUT "${MUJOCO_TAR}" DESTINATION "${CMAKE_BINARY_DIR}/_deps")
  endif()
  set(MUJOCO_INCLUDE "${MUJOCO_SRC}/include")
  set(MUJOCO_LIBRARY "${MUJOCO_SRC}/lib/libmujoco.so")
  if(NOT EXISTS "${MUJOCO_LIBRARY}")
    message(FATAL_ERROR "MuJoCo library missing at ${MUJOCO_LIBRARY}")
  endif()
endif()

get_filename_component(MUJOCO_LIB_DIR "${MUJOCO_LIBRARY}" DIRECTORY)
add_library(mujoco SHARED IMPORTED)
set_target_properties(mujoco PROPERTIES
  IMPORTED_LOCATION "${MUJOCO_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES "${MUJOCO_INCLUDE}"
)

set(LATCH_LIB_DIR "" CACHE PATH "Private directory containing liblatch.so (never commit the artifact)")
set(LATCH_LIBRARY "" CACHE FILEPATH "Optional full path to liblatch.so or liblatch.a")
if(NOT LATCH_LIB_DIR AND DEFINED ENV{LATCH_LIB_DIR})
  set(LATCH_LIB_DIR "$ENV{LATCH_LIB_DIR}")
endif()
if(NOT LATCH_LIBRARY AND DEFINED ENV{LATCH_LIBRARY})
  set(LATCH_LIBRARY "$ENV{LATCH_LIBRARY}")
endif()
if(NOT LATCH_LIBRARY AND LATCH_LIB_DIR)
  find_library(LATCH_LIBRARY_FOUND NAMES latch PATHS "${LATCH_LIB_DIR}" NO_DEFAULT_PATH REQUIRED)
  set(LATCH_LIBRARY "${LATCH_LIBRARY_FOUND}")
endif()

add_library(latch_headers INTERFACE)
target_include_directories(latch_headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(LATCH_LIBRARY)
  message(STATUS "latch-bench mujoco: linking real Latch at ${LATCH_LIBRARY} (bit-match only with LATCH_GOLDENS)")
  add_library(latch_oracle INTERFACE)
  target_link_libraries(latch_oracle INTERFACE "${LATCH_LIBRARY}" latch_headers)
  target_compile_definitions(latch_oracle INTERFACE LBS_MOCK_ORACLE=0)
else()
  message(STATUS "latch-bench mujoco: no liblatch (LATCH_LIB_DIR unset) — mock-oracle")
  add_library(latch_mock STATIC src/latch_mock.c)
  target_link_libraries(latch_mock PUBLIC latch_headers)
  target_compile_options(latch_mock PRIVATE -Wall -Wextra -Wpedantic -Werror)
  add_library(latch_oracle INTERFACE)
  target_link_libraries(latch_oracle INTERFACE latch_mock latch_headers)
  target_compile_definitions(latch_oracle INTERFACE LBS_MOCK_ORACLE=1)
endif()

add_library(lbs_core STATIC
  src/names.c
  src/setpoint.c
  src/election.c
)
target_link_libraries(lbs_core PUBLIC latch_oracle)
target_compile_options(lbs_core PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_library(lbs_plant STATIC
  src/sense_mj.c
  src/plant.c
)
target_link_libraries(lbs_plant PUBLIC lbs_core mujoco m)
target_compile_options(lbs_plant PRIVATE -Wall -Wextra -Wpedantic -Werror)

function(lbs_binary name)
  add_executable(${name} ${ARGN})
  target_compile_options(${name} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  set_target_properties(${name} PROPERTIES
    BUILD_RPATH "${MUJOCO_LIB_DIR}"
    INSTALL_RPATH "${MUJOCO_LIB_DIR}"
  )
endfunction()

lbs_binary(lbs_bm src/bm_main.c src/oracle_runner.c)
target_link_libraries(lbs_bm PRIVATE lbs_plant)
target_compile_definitions(lbs_bm PRIVATE "LBS_FIXTURE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures\"")

lbs_binary(lbs_noalloc src/noalloc_main.c)
target_link_libraries(lbs_noalloc PRIVATE lbs_core)
target_link_options(lbs_noalloc PRIVATE
  -Wl,--wrap=malloc
  -Wl,--wrap=calloc
  -Wl,--wrap=realloc
)

lbs_binary(lbs_verify src/verify_main.c src/oracle_runner.c)
target_link_libraries(lbs_verify PRIVATE lbs_core)
target_compile_definitions(lbs_verify PRIVATE "LBS_FIXTURE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures\"")

lbs_binary(lbs_soak src/soak_main.c src/oracle_runner.c)
target_link_libraries(lbs_soak PRIVATE lbs_core)
target_compile_definitions(lbs_soak PRIVATE "LBS_FIXTURE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures\"")

lbs_binary(lbs_contact src/contact_rank_main.c)
target_link_libraries(lbs_contact PRIVATE lbs_core)

lbs_binary(lbs_hand src/hand_rank_main.c)
target_link_libraries(lbs_hand PRIVATE lbs_core)

include(CTest)
add_test(NAME lbs_bm COMMAND lbs_bm)
add_test(NAME lbs_noalloc COMMAND lbs_noalloc)
add_test(NAME lbs_verify COMMAND lbs_verify)
add_test(NAME lbs_soak COMMAND lbs_soak)
add_test(NAME lbs_contact COMMAND lbs_contact)
add_test(NAME lbs_hand COMMAND lbs_hand)
set_tests_properties(lbs_bm lbs_noalloc lbs_verify lbs_soak lbs_contact lbs_hand PROPERTIES
  ENVIRONMENT "LD_LIBRARY_PATH=${MUJOCO_LIB_DIR}"
)

# Public CI runs `cmake --build` and does not invoke ctest. Run the fixtures
# as part of the default build so the stub-skip path is actually exercised.
add_custom_target(lbs_fixtures ALL
  COMMAND lbs_bm
  COMMAND lbs_noalloc
  COMMAND lbs_verify
  COMMAND lbs_soak
  COMMAND lbs_contact
  COMMAND lbs_hand
  DEPENDS lbs_bm lbs_noalloc lbs_verify lbs_soak lbs_contact lbs_hand
  COMMENT "MuJoCo fixtures (mock-oracle unless liblatch is linked)"
  USES_TERMINAL
)

message(STATUS "latch-bench mujoco: L1 sources and L2 mock-oracle verify/soak (LATCH_BENCH_BUILD_MUJOCO=ON)")

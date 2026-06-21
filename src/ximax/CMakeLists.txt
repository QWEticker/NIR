cmake_minimum_required(VERSION 3.14)
project(nir_ximax LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch Eigen
FetchContent_Declare(
  eigen
  GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
  GIT_TAG 3.4.0
)
FetchContent_MakeAvailable(eigen)

# GoogleTest
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# nlohmann::json
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

add_library(nir_ximax
  src/HolevoCalculator.cpp
  src/FiniteKeyAnalyzer.cpp
  src/XiMaxSolver.cpp
  src/HeterodyneReceiverDetailed.cpp
  src/AttackBase.cpp
  src/attacks/EntanglingCloner.cpp
  src/attacks/LOManipulation.cpp
  src/attacks/DetectorSaturation.cpp
  src/attacks/InterceptResend.cpp
  src/attacks/AttackRegistry.cpp
  src/ConsoleUtils.cpp
)

target_include_directories(nir_ximax PUBLIC
  ${eigen_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(nir_ximax PUBLIC Eigen3::Eigen)

# executable
add_executable(cvqkd_sim
  src/cvqkd_sim.cpp
)
target_link_libraries(cvqkd_sim PRIVATE nir_ximax nlohmann_json::nlohmann_json)

# Tests
enable_testing()
add_executable(test_ximax tests/test_ximax.cpp)
target_link_libraries(test_ximax PRIVATE nir_ximax gtest_main)
add_test(NAME xi_max_test COMMAND test_ximax)

add_executable(test_channel utils/tests_channel.cpp)
target_link_libraries(test_channel PRIVATE gtest_main nir_ximax)
add_test(NAME channel_test COMMAND test_channel)

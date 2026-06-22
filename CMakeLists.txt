cmake_minimum_required(VERSION 3.8...4.2)
project(cvqkd_sim LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CVQKD_BUILD_TESTS "Сборка модульных тестов" ON)

# ============================================================
# === Eigen из extern (header-only) ===
# ============================================================
set(EIGEN_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/eigen")
if(NOT EXISTS "${EIGEN_INCLUDE_DIR}/Eigen/Core")
    message(FATAL_ERROR "Eigen не найден: ${EIGEN_INCLUDE_DIR}/Eigen/Core")
endif()
add_library(Eigen3::Eigen INTERFACE IMPORTED)
target_include_directories(Eigen3::Eigen INTERFACE "${EIGEN_INCLUDE_DIR}")
message(STATUS "✅ Eigen: ${EIGEN_INCLUDE_DIR}")

# ============================================================
# === nlohmann/json из extern (header-only) ===
# ============================================================
set(JSON_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/nlohmann")
if(NOT EXISTS "${JSON_INCLUDE_DIR}/json.hpp")
    message(FATAL_ERROR "nlohmann/json не найден: ${JSON_INCLUDE_DIR}/json.hpp")
endif()
add_library(nlohmann_json INTERFACE IMPORTED)
target_include_directories(nlohmann_json INTERFACE "${JSON_INCLUDE_DIR}")
message(STATUS "✅ nlohmann/json: ${JSON_INCLUDE_DIR}")

# ============================================================
# === Основная библиотека ===
# ============================================================
file(GLOB_RECURSE LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
# Exclude unit-test translation units (they depend on GoogleTest and define their own TEST cases)
list(FILTER LIB_SOURCES EXCLUDE REGEX ".*/tests/.*")
file(GLOB_RECURSE LIB_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

add_library(cvqkd_lib STATIC ${LIB_SOURCES} ${LIB_HEADERS})
set_target_properties(cvqkd_lib PROPERTIES OUTPUT_NAME "cvqkd")

target_include_directories(cvqkd_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/ximax/include>
    $<INSTALL_INTERFACE:include>
    "${EIGEN_INCLUDE_DIR}"
    "${JSON_INCLUDE_DIR}"
)

target_link_libraries(cvqkd_lib
    PUBLIC
        Eigen3::Eigen
        nlohmann_json
)

target_compile_options(cvqkd_lib PRIVATE
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/O2>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /EHsc>
    $<$<AND:$<CXX_COMPILER_ID:GNU,Clang,AppleClang>,$<CONFIG:Release>>:-O3>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>
)

# ============================================================
# === Исполняемый файл ===
# ============================================================
add_executable(cvqkd_sim main.cpp)
target_link_libraries(cvqkd_sim PRIVATE cvqkd_lib)

# ============================================================
# === GoogleTest (FetchContent) ===
# ============================================================
# ============================================================
# === GoogleTest (FetchContent) ===
# ============================================================
if(CVQKD_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
        GIT_SHALLOW    TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    
    file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
    
    # 1. Сначала создаём цель (исполняемый файл тестов)
    add_executable(cvqkd_tests ${TEST_SOURCES})
    
    # 2. Линкуем библиотеки
    target_link_libraries(cvqkd_tests PRIVATE cvqkd_lib GTest::gtest_main)
    
    # 3. Теперь можно добавлять include-пути (исправлена переменная googletest_SOURCE_DIR)
    target_include_directories(cvqkd_tests PRIVATE
        ${googletest_SOURCE_DIR}/include
        ${googletest_SOURCE_DIR}
    )
    
    include(GoogleTest)
    gtest_discover_tests(cvqkd_tests)
endif()
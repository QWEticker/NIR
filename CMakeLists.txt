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
add_executable(cvqkd_validation tools/validation_probe.cpp)
target_link_libraries(cvqkd_validation PRIVATE cvqkd_lib)

find_package(Git QUIET)
set(CVQKD_REVISION "unversioned")
if(GIT_FOUND)
    execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE CVQKD_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
endif()
file(GLOB_RECURSE FINGERPRINT_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "src/*.cpp" "include/*.hpp" "tools/*.cpp")
list(APPEND FINGERPRINT_FILES "CMakeLists.txt")
list(SORT FINGERPRINT_FILES)
set(FINGERPRINT_INPUT "")
foreach(SOURCE_FILE IN LISTS FINGERPRINT_FILES)
    file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}" SOURCE_HASH)
    string(APPEND FINGERPRINT_INPUT "${SOURCE_FILE}:${SOURCE_HASH}\n")
endforeach()
string(SHA256 CVQKD_FINGERPRINT "${FINGERPRINT_INPUT}")

target_compile_definitions(cvqkd_lib PRIVATE
    CVQKD_SOURCE_REVISION="${CVQKD_REVISION}"
    CVQKD_SOURCE_FINGERPRINT="${CVQKD_FINGERPRINT}")

# ============================================================
# === GoogleTest (FetchContent) ===
# ============================================================
# ============================================================
# === GoogleTest (FetchContent) ===
# ============================================================
# ============================================================
# === GoogleTest (Локальная сборка из extern) ===
# ============================================================
if(CVQKD_BUILD_TESTS)
    set(GOOGLETEST_LOCAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/googletest-1.14.0")
    
    if(NOT EXISTS "${GOOGLETEST_LOCAL_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "GoogleTest не найден в ${GOOGLETEST_LOCAL_DIR}. Пожалуйста, скачайте и распакуйте архив v1.14.0 в папку extern.")
    endif()

    # Настройки для корректной компиляции под MSVC
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    
    # Добавляем локальную директорию вместо сетевой загрузки
    add_subdirectory("${GOOGLETEST_LOCAL_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/_deps/googletest-build" EXCLUDE_FROM_ALL)

    enable_testing()
    
    file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
    
    add_executable(cvqkd_tests ${TEST_SOURCES})
    
    target_link_libraries(cvqkd_tests PRIVATE 
        cvqkd_lib 
        GTest::gtest_main
    )
    
    # Явное указание путей для IntelliSense и компилятора
    target_include_directories(cvqkd_tests PRIVATE
        "${GOOGLETEST_LOCAL_DIR}/googletest/include"
        "${GOOGLETEST_LOCAL_DIR}/googletest"
    )
    
    include(GoogleTest)
    gtest_discover_tests(cvqkd_tests)
    
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(NAME ExperimentIntegration
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_experiment_integration.py"
                --exe "$<TARGET_FILE:cvqkd_sim>"
                --workdir "${CMAKE_CURRENT_BINARY_DIR}/integration-tests")
    endif()
endif()
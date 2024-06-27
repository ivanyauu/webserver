cmake_minimum_required(VERSION 3.10.0)
project(new-grad-ten-years-experience)

# Make sure developers do not run cmake in the main project directory, to keep
# build artifacts from becoming clutter
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed.
    Please make a new directory (called a build directory) and run CMake from there.
    You may need to remove CMakeCache.txt." )
endif()

# Turn on debug builds if we are building in a devel shell
if (CMAKE_BUILD_TYPE STREQUAL "" AND "$ENV{DEVEL_SHELL}" STREQUAL "1")
    message(STATUS "Setting debug build type by default in devel shell")
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Output binaries to a sub directory "bin"
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Enable cmake testing
include(CTest)
enable_testing()

set(BUILD_TESTING OFF CACHE BOOL "Enable/disable testing for external repository" FORCE)

# Enable GoogleTest
include(GoogleTest)
add_subdirectory(/usr/src/googletest googletest)

# Enable Boost
# Use static libraries so binaries can be deployed without a full boost install
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.50 REQUIRED COMPONENTS system filesystem log_setup log regex)
message(STATUS "Boost version: ${Boost_VERSION}")

include_directories(include)

add_library(logger src/logger.cc)

add_library(file_io src/file_io.cc)

add_subdirectory(external/cmark)

add_library(markdown_to_html src/markdown_to_html.cc)
target_link_libraries(markdown_to_html cmark)
add_executable(markdown_to_html_test tests/markdown_to_html_test.cc)
target_link_libraries(markdown_to_html_test markdown_to_html gtest_main)
gtest_discover_tests(markdown_to_html_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

add_executable(server src/server_main.cc src/server.cc)
target_link_libraries(server session config_parser logger Boost::system Boost::filesystem 
                      Boost::regex Boost::log_setup Boost::log)

add_library(config_parser src/config_parser.cc)
add_executable(config_parser_test tests/config_parser_test.cc)
target_link_libraries(config_parser_test config_parser gtest_main)
gtest_discover_tests(config_parser_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

add_library(router src/router.cc)
target_link_libraries(router request_handlers config_parser)
add_executable(router_test tests/router_test.cc)
target_link_libraries(router_test router gtest_main logger Boost::system Boost::filesystem Boost::regex Boost::log_setup Boost::log)
gtest_discover_tests(router_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

add_library(request_handlers src/echo_handler.cc src/static_handler.cc src/notfound_handler.cc src/crud_handler.cc src/sleep_handler.cc src/health_handler.cc src/markdown_handler.cc)
target_link_libraries(request_handlers file_io markdown_to_html)
add_executable(request_handlers_test tests/request_handlers_test.cc)
target_link_libraries(request_handlers_test request_handlers gtest_main logger Boost::system Boost::filesystem Boost::regex Boost::log_setup Boost::log)
gtest_discover_tests(request_handlers_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

add_library(session src/session.cc)
target_link_libraries(session router request_handlers config_parser)
add_executable(session_test tests/session_test.cc)
target_link_libraries(session_test session gtest_main logger Boost::system Boost::filesystem Boost::regex Boost::log_setup Boost::log)
gtest_discover_tests(session_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

add_test(NAME integration_test COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration_tests.py)

include(cmake/CodeCoverageReportConfig.cmake)
generate_coverage_report(TARGETS file_io server session config_parser request_handlers router markdown_to_html TESTS config_parser_test session_test request_handlers_test router_test markdown_to_html_test)

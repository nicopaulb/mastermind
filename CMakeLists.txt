# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mastermind)

FILE(GLOB app_sources src/*.cpp)
target_sources(app PRIVATE ${app_sources})

set(GIT_DIR_LOOKUP_POLICY ALLOW_LOOKING_ABOVE_CMAKE_SOURCE_DIR)
add_subdirectory(src/etl)
target_link_libraries(app PRIVATE etl::etl)
cmake_minimum_required(VERSION 3.2)
project(MiniWeb VERSION 0.1.0)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake/Modules)

include(MacroEnsureOutOfSourceBuild)
macro_ensure_out_of_source_build("building in the source tree is not supported")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug"
    CACHE STRING
    "Build type: Debug, Release, RelWithDebInfo, Profile"
    FORCE
  )
endif(NOT CMAKE_BUILD_TYPE)
option(TAGS "Generate CTags" ON)
option(VALGRIND "Configure build for Valgrind memcheck" OFF)
option(BUILD_DOCUMENTATION "Build Doxygen Documentation (requires doxygen)" OFF)
option(BUILD_TESTS "Build tests" ON)
option(PROFILE "Generate coverage information" ON)

set(WARNING_FLAGS
  "-Wall -Wextra -pedantic -Wno-gnu-zero-variadic-macro-arguments -std=c11"
)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS}")

if(PROFILE)
  include(CodeCoverage)
  set( CMAKE_C_FLAGS_DEBUG
       "${CMAKE_C_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage" )
endif(PROFILE)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")

if(TAGS)
  include(CTags)
  add_ctags_target(${CMAKE_CURRENT_LIST_DIR}/.git/tags)
endif(TAGS)

include_directories(src)
add_subdirectory(src)

if(BUILD_DOCUMENTATION)
  add_subdirectory(doc)
endif(BUILD_DOCUMENTATION)

if(BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(test)
  include_directories(test)
endif(BUILD_TESTS)

#  vim: set ts=8 sw=2 tw=0 ft=cmake et :

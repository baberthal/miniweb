cmake_minimum_required(VERSION 3.2)

set(HEADERS
  miniweb.h
  miniweb_logging.h
  miniweb_request.h
  mw_mempool.h
  mw_buffer.h
)

set(SOURCES
  miniweb.c
  miniweb_logging.c
  miniweb_request.c
  mw_mempool.c
  mw_buffer.c
  main.c
  ${HEADERS}
)

set(PROGRAM "miniweb")

set(OBJECTLIBS "")

macro(add_obj_lib _libname)
  add_library(${_libname} OBJECT ${_libname}.c ${_libname}.h)
  list(APPEND OBJECTLIBS $<TARGET_OBJECTS:${_libname}>)
endmacro(add_obj_lib)

add_obj_lib(miniweb_logging)
add_obj_lib(miniweb_request)
add_obj_lib(mw_mempool)
add_obj_lib(mw_buffer)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

#######################################################################
#                            Dependencies                             #
#######################################################################
find_package(ZLIB REQUIRED)

include_directories(${ZLIB_INCLUDE_DIRS})

add_executable(${PROGRAM} ${SOURCES})
target_link_libraries(${PROGRAM} ${ZLIB_LIBRARIES} system)

#######################################################################
#                         Feature Checks, Etc                         #
#######################################################################
include(CheckIncludeFile)
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CheckSymbolExists)
include(CheckCCompilerFlag)

check_include_file(stdbool.h HAVE_STDBOOL_H)
check_include_file(Block.h HAVE_BLOCK_H)
check_include_file(assert.h HAVE_ASSERT_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(unistd.h HAVE_UNISTD_H)

check_c_source_compiles(
  "int main() { __builtin_expect(0,0); return 0; }"
  HAVE___BUILTIN_EXPECT)
check_c_source_compiles("
  int f(void) __attribute__ (());
  int main() { return 0; }
" HAVE___ATTRIBUTE__)

check_c_compiler_flag(-fblocks HAVE_BLOCKS_RUNTIME)

configure_file(${PROJECT_SOURCE_DIR}/cmake/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/config.h)



#  vim: set ts=8 sw=2 tw=0 ft=cmake et :

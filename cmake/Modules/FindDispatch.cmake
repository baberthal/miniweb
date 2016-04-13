find_package(PkgConfig)
pkg_check_modules(PC_LIBDISPATCH QUIET dispatch)

find_path(DISPATCH_INCLUDE_DIR dispatch/dispatch.h
  HINTS ${PC_LIBDISPATCH_INCLUDE_DIR} ${PC_LIBDISPATCH_INCLUDE_DIRS}
)

find_library(DISPATCH_LIBRARY NAMES dispatch PATH_SUFFIXES "system")

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(DISPATCH
  DEFAULT_MSG DISPATCH_LIBRARY DISPATCH_INCLUDE_DIR
)

if(DISPATCH_LIBRARY)
  set(DISPATCH_LIBRARIES ${DISPATCH_LIBRARY})
endif(DISPATCH_LIBRARY)

include(CheckIncludeFile)
check_include_file(dispatch/dispatch.h HAVE_DISPATCH_H)

mark_as_advanced(
  DISPATCH_INCLUDE_DIR
  DISPATCH_LIBRARY
  DISPATCH_LIBRARIES
  HAVE_DISPATCH_H
)

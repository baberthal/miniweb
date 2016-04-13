# - Try to find the CHECK libraries
#  Once done this will define
#
#  CHECK_FOUND - system has check
#  CHECK_INCLUDE_DIR - the check include directory
#  CHECK_LIBRARIES - check library
#
#  This configuration file for finding libcheck is originally from
#  the opensync project. The originally was downloaded from here:
#  opensync.org/browser/branches/3rd-party-cmake-modules/modules/FindCheck.cmake
#
#  Copyright (c) 2007 Daniel Gollub <dgollub@suse.de>
#  Copyright (c) 2007 Bjoern Ricks  <b.ricks@fh-osnabrueck.de>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.

find_package(PkgConfig)
pkg_check_modules(PC_CHECK QUIET check)

find_path(CHECK_INCLUDE_DIR check.h
  PATHS ${CHECK_PREFIX}/include
  HINTS ${PC_CHECK_INCLUDE_DIR} ${PC_CHECK_INCLUDE_DIRS})

find_library(CHECK_LIBRARY
  NAMES check
  PATHS ${CHECK_PREFIX}/lib
  HINTS ${PC_CHECK_LIBDIR} ${PC_CHECK_LIBRARY_DIRS})

if("${CHECK_LIBRARY}" STREQUAL "CHECK_LIBRARY-NOTFOUND")

  find_library(COMPAT_LIBRARY
    NAMES compat
    PATHS ${CHECK_PREFIX}/lib /usr/local/lib /usr/lib
    HINTS ${PC_CHECK_LIBDIR} ${PC_CHECK_LIBRARY_DIRS})

endif("${CHECK_LIBRARY}" STREQUAL "CHECK_LIBRARY-NOTFOUND")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CHECK
  DEFAULT_MSG
  CHECK_LIBRARY CHECK_INCLUDE_DIR)

if(NOT CHECK_LIBRARY MATCHES "NOTFOUND")
  set(CHECK_LIBRARIES "${CHECK_LIBRARY}")
endif(NOT CHECK_LIBRARY MATCHES "NOTFOUND")

if(NOT "${COMPAT_LIBRARY}" MATCHES "NOTFOUND")
  set(CHECK_LIBRARIES "${CHECK_LIBRARIES}" ${COMPAT_LIBRARY})
endif(NOT "${COMPAT_LIBRARY}" MATCHES "NOTFOUND")

# Hide advanced variables from CMake GUIs
MARK_AS_ADVANCED( CHECK_INCLUDE_DIR CHECK_LIBRARIES )


function(CHECK_ADD_TESTS executable extra_args)
  if(NOT ARGN)
    message(
      FATAL_ERROR
      "Missing ARGN: Read the documentation for CMOCKA_ADD_TESTS"
    )
  endif(NOT ARGN)

  if(ARGN STREQUAL "AUTO")
    # obtain sources used for building that executable
    get_property(ARGN TARGET ${executable} PROPERTY SOURCES)
  endif()

  set(check_test_name_regex "START_TEST.([A-Za-z_0-9]+).")

  foreach(source ${ARGN})
    file(READ "${source}" contents)

    string(REGEX MATCHALL
      "${check_test_name_regex}"
      found_tests
      ${contents}
    )

    foreach(hit ${found_tests})
      string(REGEX REPLACE "${check_test_name_regex}" "\\1" test_func ${hit})

      if(NOT test_func)
        message(WARNING "Could not parse a test from ${hit} for some reason")
        continue()
      endif(NOT test_func)

      add_test(
        NAME ${test_func}
        COMMAND ${executable} ${extra_args}
      )

      set_tests_properties(${test_func}
        PROPERTIES
        ENVIRONMENT CK_RUN_CASE=${test_func}
      )

    endforeach(hit)
  endforeach(source)
endfunction(CHECK_ADD_TESTS)

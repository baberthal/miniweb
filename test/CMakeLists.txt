cmake_minimum_required(VERSION 3.2)

find_package(CMocka REQUIRED)
include_directories(
  ${CMOCKA_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_BINARY_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}
)

find_program(VALGRIND_EXE valgrind)

macro(jml_add_test _target_name _sources)
  add_executable(${_target_name} ${${_sources}})
  add_dependencies(${_target_name} ${_target_name})
  target_link_libraries(${_target_name} ${CMOCKA_LIBRARIES})
  target_compile_definitions(${_target_name} INTERFACE CSPEC_STATIC_DEFINE=1)
  target_compile_options(${_target_name} INTERFACE "-fprofile-arcs" "-ftest-coverage")
  add_test(${_target_name} ${_target_name})
  if(VALGRIND_EXE)
    add_test(${_target_name}_valgrind
      ${VALGRIND_EXE} --error-exitcode=1 --read-var-info=yes --leak-check=full
      --show-leak-kinds=all ./${_target_name}
    )
  endif(VALGRIND_EXE)
endmacro(jml_add_test)

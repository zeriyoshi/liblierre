# liblierre test

include(CTest)
include(FetchContent)
include(CheckIncludeFile)

enable_testing()

FetchContent_Declare(unity SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/unity)
FetchContent_MakeAvailable(unity)

set(LIERRE_TEST_ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
add_definitions(-DLIERRE_TEST_ASSETS_DIR="${LIERRE_TEST_ASSETS_DIR}")

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

set(LIERRE_TEST_LINK_LIBRARIES lierre unity)

if(LIERRE_ENABLE_COVERAGE)
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/coverage)
  add_custom_target(coverage
    COMMAND ${LCOV} --initial --directory ${CMAKE_BINARY_DIR} --capture --output-file ${CMAKE_BINARY_DIR}/coverage/base.info --ignore-errors negative
    COMMAND ${CMAKE_CTEST_COMMAND} -C ${CMAKE_BUILD_TYPE}
    COMMAND ${LCOV} --directory ${CMAKE_BINARY_DIR} --capture --output-file ${CMAKE_BINARY_DIR}/coverage/test.info --ignore-errors negative
    COMMAND ${LCOV} --add-tracefile ${CMAKE_BINARY_DIR}/coverage/base.info --add-tracefile ${CMAKE_BINARY_DIR}/coverage/test.info --output-file ${CMAKE_BINARY_DIR}/coverage/total.info --ignore-errors negative
    COMMAND ${LCOV} --remove ${CMAKE_BINARY_DIR}/coverage/total.info '/usr/*' --output-file ${CMAKE_BINARY_DIR}/coverage/filtered.info --ignore-errors negative
    COMMAND ${GENHTML} --demangle-cpp --legend --title "lierre Coverage Report" --output-directory ${CMAKE_BINARY_DIR}/coverage/html ${CMAKE_BINARY_DIR}/coverage/filtered.info --ignore-errors negative
    COMMENT "Generating coverage report with lcov/gcov"
  )

  message(STATUS "Coverage report generation target 'coverage' is now available")
endif()

file(GLOB TEST_SOURCES "tests/test_*.c")

foreach(TEST_SOURCE ${TEST_SOURCES})
  get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
  set(TEST_NAME "lierre_${TEST_NAME}")

  add_executable(${TEST_NAME} ${TEST_SOURCE})

  target_link_libraries(${TEST_NAME} PRIVATE lierre unity)

  target_include_directories(${TEST_NAME} PRIVATE ${unity_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include)

  set_target_properties(${TEST_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
  )

  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

  if(LIERRE_ENABLE_VALGRIND)
    add_test(
      NAME ${TEST_NAME}_valgrind
      COMMAND ${VALGRIND} 
        "--tool=memcheck"
        "--leak-check=full"
        "--show-leak-kinds=all"
        "--track-origins=yes"
        "--error-exitcode=1"
      $<TARGET_FILE:${TEST_NAME}>)
  endif()
endforeach()

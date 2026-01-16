# liblierre

cmake_minimum_required(VERSION 3.25)
project(lierre
  VERSION 1.0.5
  LANGUAGES C
  DESCRIPTION "QR code encoding and decoding library"
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(cmake/buildtime.cmake)

include(GNUInstallDirs)

option(LIERRE_USE_SIMD "Enable SIMD optimizations" ON)
option(LIERRE_USE_VALGRIND "Use Valgrind if available" OFF)
option(LIERRE_USE_COVERAGE "Use coverage if available" OFF)
option(LIERRE_USE_ASAN "Use AddressSanitizer" OFF)
option(LIERRE_USE_MSAN "Use MemorySanitizer" OFF)
option(LIERRE_USE_UBSAN "Use UndefinedBehaviorSanitizer" OFF)
option(LIERRE_USE_TESTS "Use tests" OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT EMSCRIPTEN AND NOT MSVC)
  message(STATUS "Enabled debug")

  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -fno-inline -fno-omit-frame-pointer")

  if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    if(LIERRE_USE_ASAN)
      message(STATUS "Enabling AddressSanitizer")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
      set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fsanitize=address")
    endif()

    if(LIERRE_USE_MSAN)
      message(STATUS "Enabling MemorySanitizer")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=memory")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=memory")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=memory")
      set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fsanitize=memory")
    endif()

    if(LIERRE_USE_UBSAN)
      message(STATUS "Enabling UndefinedBehaviorSanitizer")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=undefined")
      set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fsanitize=undefined")
    endif()
  endif()

  find_program(VALGRIND "valgrind")
  if(LIERRE_USE_VALGRIND AND VALGRIND)
    message(STATUS "Found Valgrind: ${VALGRIND}")
    set(LIERRE_ENABLE_VALGRIND ON)
  else()
    set(LIERRE_ENABLE_VALGRIND OFF)
  endif()

  if(LIERRE_USE_COVERAGE)
    find_program(LCOV lcov)
    find_program(GENHTML genhtml)
    
    if(CMAKE_C_COMPILER_ID MATCHES "GNU")
      if(LCOV AND GENHTML)
        set(LIERRE_ENABLE_COVERAGE ON)
        add_definitions(-DLIERRE_COVERAGE)

        if(CMAKE_C_COMPILER_ID MATCHES "GNU")
          link_libraries(gcov)
        endif()
      else()
        message(WARNING "lcov or genhtml not found, disabling coverage")
        set(LIERRE_ENABLE_COVERAGE OFF)
      endif()
    else()
      message(WARNING "Coverage is supported only with GCC compilers")
      set(LIERRE_ENABLE_COVERAGE OFF)
    endif()
  else()
    set(LIERRE_ENABLE_COVERAGE OFF)
  endif()
else()
  set(LIERRE_ENABLE_VALGRIND OFF)
  set(LIERRE_ENABLE_COVERAGE OFF)
endif()

if(EMSCRIPTEN)
  include(cmake/emscripten.cmake)
endif()

if(LIERRE_ENABLE_VALGRIND)
  message(STATUS "Valgrind enabled: forcing SIMD off")
  set(LIERRE_USE_SIMD OFF)
  set(POPORON_USE_SIMD OFF CACHE BOOL "Use SIMD optimizations" FORCE)
endif()

if(LIERRE_USE_SIMD)
  include(CheckCCompilerFlag)

  if(EMSCRIPTEN)
    check_c_compiler_flag("-msimd128" LIERRE_HAS_WASM_SIMD)
    if(LIERRE_HAS_WASM_SIMD)
      message(STATUS "Enabling WebAssembly SIMD128")
      set(LIERRE_SIMD_FLAGS "-msimd128")
    endif()
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|aarch64)")
    message(STATUS "Detected ARM architecture, NEON may be available")
    set(LIERRE_SIMD_FLAGS "")
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|i[3-6]86)")
    check_c_compiler_flag("-mavx2" LIERRE_HAS_AVX2)
    if(LIERRE_HAS_AVX2)
      message(STATUS "Enabling x86 AVX2")
      set(LIERRE_SIMD_FLAGS "-mavx2")
    endif()
  endif()
endif()

# Add libpoporon as a subdirectory
if(NOT TARGET poporon)
  add_subdirectory(third_party/libpoporon EXCLUDE_FROM_ALL)
endif()

file(GLOB_RECURSE SOURCES "src/*.c")
file(GLOB_RECURSE HEADERS "include/*.h")

add_library(lierre-obj OBJECT ${SOURCES})
target_include_directories(lierre-obj PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libpoporon/include
)

if(LIERRE_USE_SIMD)
  target_compile_definitions(lierre-obj PRIVATE LIERRE_USE_SIMD=1)
  if(LIERRE_SIMD_FLAGS)
    target_compile_options(lierre-obj PRIVATE ${LIERRE_SIMD_FLAGS})
  endif()
endif()

if(LIERRE_ENABLE_COVERAGE)
  target_compile_options(lierre-obj PRIVATE "--coverage")
  target_link_options(lierre-obj PRIVATE "--coverage")
endif()

add_library(lierre STATIC $<TARGET_OBJECTS:lierre-obj>)
target_link_libraries(lierre PUBLIC poporon)
if(UNIX AND NOT APPLE)
  target_link_libraries(lierre PUBLIC m)
endif()

target_include_directories(lierre PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/libpoporon/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(BUILD_SHARED_LIBS)
  add_library(lierre-shared SHARED $<TARGET_OBJECTS:lierre-obj>)
  target_link_libraries(lierre-shared PUBLIC poporon)
  target_include_directories(lierre-shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/libpoporon/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
endif()

if(LIERRE_ENABLE_COVERAGE)
  target_link_options(lierre PRIVATE "--coverage")
endif()

set_target_properties(lierre PROPERTIES
  OUTPUT_NAME "lierre"
)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h"
)

install(
  TARGETS lierre poporon
  EXPORT lierre-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
  EXPORT lierre-targets
  FILE LierreTargets.cmake
  NAMESPACE lierre::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lierre
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/LierreConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/LierreConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lierre
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/LierreConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/LierreConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/LierreConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lierre
)

if(LIERRE_USE_TESTS)
  include(cmake/test.cmake)
endif()

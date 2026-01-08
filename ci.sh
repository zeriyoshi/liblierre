#!/bin/sh -eu

cd "$(dirname "${0}")"

for BUILD_TYPE in "Debug" "Release"; do
  rm -rf "build"
  cmake -B "build" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" -DLIERRE_USE_TESTS=ON .
  cmake --build "build" --parallel
  ctest --build-config "${BUILD_TYPE}" --test-dir "build" --output-on-failure --parallel
done

if type "clang" > /dev/null 2>&1 && type "clang++" > /dev/null 2>&1; then
  rm -rf "build"
  cmake -B "build" \
    -DCMAKE_C_COMPILER="$(command -v "clang")" \
    -DCMAKE_CXX_COMPILER="$(command -v "clang++")" \
    -DCMAKE_BUILD_TYPE="Debug" \
    -DLIERRE_USE_TESTS=ON \
    .
  cmake --build "build" --parallel
  ctest --build-config "Debug" --test-dir "build" --output-on-failure --parallel

  for SANITIZER in "ASAN" "UBSAN" "MSAN"; do
    rm -rf "build"
    cmake -B "build" \
      -DCMAKE_C_COMPILER="$(command -v "clang")" \
      -DCMAKE_CXX_COMPILER="$(command -v "clang++")" \
      -DCMAKE_BUILD_TYPE="Debug" \
      -DLIERRE_USE_TESTS=ON \
      -DLIERRE_USE_${SANITIZER}=ON \
      .
    cmake --build "build" --parallel
    ctest --build-config "Debug" --test-dir "build" --output-on-failure --parallel
  done
fi

if type "emcc" > /dev/null 2>&1; then
  rm -rf "build"
  emcmake cmake -B "build" \
    -DCMAKE_BUILD_TYPE="Debug" \
    -DLIERRE_USE_TESTS=ON \
    .
  cmake --build "build" --parallel
  ctest --build-config "Debug" --test-dir "build" --output-on-failure --parallel
fi

if type "valgrind" > /dev/null 2>&1; then
  rm -rf "build"
  cmake -B "build" \
    -DCMAKE_BUILD_TYPE="Debug" \
    -DLIERRE_USE_TESTS=ON \
    -DLIERRE_USE_VALGRIND=ON \
    .
  cmake --build "build" --parallel
  ctest --build-config "Debug" --test-dir "build" --output-on-failure --parallel
fi

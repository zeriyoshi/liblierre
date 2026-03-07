# liblierre

[![CI](https://github.com/zeriyoshi/liblierre/actions/workflows/ci.yaml/badge.svg)](https://github.com/zeriyoshi/liblierre/actions/workflows/ci.yaml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**Lightweight Image Encoding & Reading for Resilient Encoded data**

[🇯🇵 日本語版 README はこちら](README_ja.md)

liblierre is a lightweight QR code encoding and decoding library written in C99. It provides QR code generation and reading capabilities with optional SIMD acceleration and a portable fallback path.

## Features

- **C99 Implementation** - Portable C code with bundled libpoporon for Reed-Solomon error correction
- **QR Code Generation** - Support for all QR versions (1-40), all error correction levels (L/M/Q/H), all mask patterns (0-7)
- **QR Code Reading** - Image preprocessing strategies and multi-code detection for RGB input images
- **SIMD Acceleration** - Optional AVX2 (x86_64), NEON (ARM/ARM64), or WASM SIMD128 code paths with scalar fallback
- **WebAssembly Support** - Can be compiled to WASM using Emscripten
- **Memory-Conscious API** - Explicit resource ownership with sanitizer and Valgrind-friendly test coverage
- **Extensive Testing** - Unit and round-trip tests for writer, reader, portable helpers, and codec behavior

## Quick Start

### Building

```bash
# Clone the repository with submodules
git clone --recursive https://github.com/zeriyoshi/liblierre.git
cd liblierre

# Build with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `LIERRE_USE_SIMD` | `ON` | Enable SIMD optimizations |
| `LIERRE_USE_TESTS` | `OFF` | Build test suite |
| `LIERRE_USE_VALGRIND` | `OFF` | Enable Valgrind memory checking |
| `LIERRE_USE_COVERAGE` | `OFF` | Enable code coverage |
| `LIERRE_USE_ASAN` | `OFF` | Enable AddressSanitizer |
| `LIERRE_USE_MSAN` | `OFF` | Enable MemorySanitizer |
| `LIERRE_USE_UBSAN` | `OFF` | Enable UndefinedBehaviorSanitizer |
| `BUILD_SHARED_LIBS` | `OFF` | Build shared library |

Notes:

- `LIERRE_USE_ASAN`, `LIERRE_USE_MSAN`, and `LIERRE_USE_UBSAN` are enabled only for Clang Debug builds.
- `LIERRE_USE_COVERAGE` is available only for GCC Debug builds with `lcov` and `genhtml` installed.
- `LIERRE_USE_VALGRIND` is effective only for Debug builds with `valgrind` available, and it forces `LIERRE_USE_SIMD=OFF`.

### Running Tests

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DLIERRE_USE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Usage Examples

### Generating a QR Code

```c
#include <lierre.h>
#include <lierre/writer.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    const char *text = "Hello, World!";
    lierre_writer_param_t param;
    lierre_rgba_t fill = {0, 0, 0, 255};      // Black
    lierre_rgba_t bg = {255, 255, 255, 255};  // White

    // Initialize parameters
    lierre_error_t err = lierre_writer_param_init(
        &param,
        (uint8_t *)text,
        strlen(text),
        4,           // scale (4x)
        2,           // margin (2 modules)
        ECC_MEDIUM,  // error correction level
        MASK_AUTO,   // auto-select mask pattern
        MODE_BYTE    // byte mode
    );
    if (err != LIERRE_ERROR_SUCCESS) {
        fprintf(stderr, "Failed to init params: %s\n", lierre_strerror(err));
        return 1;
    }

    // Get output resolution
    lierre_reso_t res;
    if (!lierre_writer_get_res(&param, &res)) {
        fprintf(stderr, "Failed to calculate output resolution\n");
        return 1;
    }
    printf("QR code size: %zux%zu\n", res.width, res.height);

    // Create writer and generate
    lierre_writer_t *writer = lierre_writer_create(&param, &fill, &bg);
    if (!writer) {
        fprintf(stderr, "Failed to create writer\n");
        return 1;
    }

    if (lierre_writer_write(writer) == LIERRE_ERROR_SUCCESS) {
        const uint8_t *rgba = lierre_writer_get_rgba_data(writer);
        size_t size = lierre_writer_get_rgba_data_size(writer);
        printf("Generated %zu bytes of RGBA data\n", size);
        // Use RGBA data (save to file, display, etc.)
    }

    lierre_writer_destroy(writer);
    return 0;
}
```

### Reading a QR Code

```c
#include <lierre.h>
#include <lierre/reader.h>
#include <stdio.h>

int main(void) {
    // Assume you have RGB image data
    uint8_t *rgb_data = /* your image data */;
    size_t width = 640, height = 480;

    // Create RGB data wrapper
    lierre_rgb_data_t *rgb = lierre_rgb_create(
        rgb_data, width * height * 3, width, height
    );
    if (!rgb) {
        fprintf(stderr, "Failed to create RGB container\n");
        return 1;
    }

    // Initialize reader parameters
    lierre_reader_param_t param;
    if (lierre_reader_param_init(&param) != LIERRE_ERROR_SUCCESS) {
        fprintf(stderr, "Failed to init reader params\n");
        lierre_rgb_destroy(rgb);
        return 1;
    }

    // Optional: Enable preprocessing strategies
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_GRAYSCALE);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_DENOISE);

    // Create reader and decode
    lierre_reader_t *reader = lierre_reader_create(&param);
    if (!reader) {
        fprintf(stderr, "Failed to create reader\n");
        lierre_rgb_destroy(rgb);
        return 1;
    }
    lierre_reader_set_data(reader, rgb);

    lierre_reader_result_t *result = NULL;
    if (lierre_reader_read(reader, &result) == LIERRE_ERROR_SUCCESS) {
        uint32_t count = lierre_reader_result_get_num_qr_codes(result);
        printf("Found %u QR code(s)\n", count);

        for (uint32_t i = 0; i < count; i++) {
            const uint8_t *data = lierre_reader_result_get_qr_code_data(result, i);
            size_t size = lierre_reader_result_get_qr_code_data_size(result, i);
            printf("QR[%u]: %.*s\n", i, (int)size, data);
        }
        lierre_reader_result_destroy(result);
    } else {
        fprintf(stderr, "Failed to decode QR code\n");
    }

    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb);
    return 0;
}
```

## API Reference

### Core Types

```c
// Error codes
typedef enum lierre_error {
    SUCCESS = 0,
    ERROR_INVALID_PARAMS,
    ERROR_INVALID_GRID_SIZE,
    ERROR_INVALID_VERSION,
    ERROR_FORMAT_ECC,
    ERROR_DATA_ECC,
    ERROR_UNKNOWN_DATA_TYPE,
    ERROR_DATA_OVERFLOW,
    ERROR_DATA_UNDERFLOW,
    ERROR_SIZE_EXCEEDED
} lierre_error_t;

// RGB image data container
typedef struct {
    uint8_t *data;
    size_t data_size;
    size_t width;
    size_t height;
} lierre_rgb_data_t;

// RGBA color
typedef struct {
    uint8_t r, g, b, a;
} lierre_rgba_t;

// 2D vector and rectangle
typedef struct { size_t x, y; } lierre_vec2_t;
typedef struct { size_t width, height; } lierre_reso_t;
typedef struct { lierre_vec2_t origin; lierre_reso_t size; } lierre_rect_t;
```

### Writer API

```c
// Error correction levels
ECC_LOW, ECC_MEDIUM, ECC_QUARTILE, ECC_HIGH

// Mask patterns
MASK_AUTO, MASK_0 through MASK_7

// Encoding modes
MODE_NUMERIC, MODE_ALPHANUMERIC, MODE_BYTE, MODE_KANJI, MODE_ECI

// Functions
lierre_error_t lierre_writer_param_init(lierre_writer_param_t *param, ...);
lierre_qr_version_t lierre_writer_qr_version(const lierre_writer_param_t *param);
bool lierre_writer_get_res(const lierre_writer_param_t *param, lierre_reso_t *res);
size_t lierre_writer_get_res_width(const lierre_writer_param_t *param);
size_t lierre_writer_get_res_height(const lierre_writer_param_t *param);
lierre_writer_t *lierre_writer_create(const lierre_writer_param_t *param,
                                      const lierre_rgba_t *fill_color,
                                      const lierre_rgba_t *bg_color);
lierre_error_t lierre_writer_write(lierre_writer_t *writer);
const uint8_t *lierre_writer_get_rgba_data(const lierre_writer_t *writer);
size_t lierre_writer_get_rgba_data_size(const lierre_writer_t *writer);
void lierre_writer_destroy(lierre_writer_t *writer);
```

### Reader API

```c
// Strategy flags (can be combined with |)
LIERRE_READER_STRATEGY_NONE
LIERRE_READER_STRATEGY_MINIMIZE          // Minimize image for easier detection
LIERRE_READER_STRATEGY_GRAYSCALE         // Convert to grayscale
LIERRE_READER_STRATEGY_USE_RECT          // Focus on specific area
LIERRE_READER_STRATEGY_DENOISE           // Apply denoising filter
LIERRE_READER_STRATEGY_BRIGHTNESS_NORMALIZE  // Normalize brightness
LIERRE_READER_STRATEGY_CONTRAST_NORMALIZE    // Normalize contrast
LIERRE_READER_STRATEGY_SHARPENING        // Apply sharpening filter
LIERRE_READER_STRATEGY_MT                // Enable multi-threading

// Functions
lierre_error_t lierre_reader_param_init(lierre_reader_param_t *param);
void lierre_reader_param_set_flag(lierre_reader_param_t *param, lierre_reader_strategy_flag_t flag);
void lierre_reader_param_set_rect(lierre_reader_param_t *param, const lierre_rect_t *rect);
lierre_reader_t *lierre_reader_create(const lierre_reader_param_t *param);
void lierre_reader_set_data(lierre_reader_t *reader, lierre_rgb_data_t *data);
lierre_error_t lierre_reader_read(lierre_reader_t *reader, lierre_reader_result_t **result);
uint32_t lierre_reader_result_get_num_qr_codes(const lierre_reader_result_t *result);
const uint8_t *lierre_reader_result_get_qr_code_data(const lierre_reader_result_t *result, uint32_t index);
size_t lierre_reader_result_get_qr_code_data_size(const lierre_reader_result_t *result, uint32_t index);
const lierre_rect_t *lierre_reader_result_get_qr_code_rect(const lierre_reader_result_t *result, uint32_t index);
void lierre_reader_result_destroy(lierre_reader_result_t *result);
void lierre_reader_destroy(lierre_reader_t *reader);
```

### Utility Functions

```c
const char *lierre_strerror(lierre_error_t err);   // Get error message
uint32_t lierre_version_id(void);                  // Get library version
lierre_buildtime_t lierre_buildtime(void);         // Get build timestamp
lierre_rgb_data_t *lierre_rgb_create(...);         // Create RGB container
void lierre_rgb_destroy(lierre_rgb_data_t *rgb);   // Destroy RGB container
```

### Portable API

```c
uint32_t lierre_get_cpu_count(void);                       // Get logical CPU count
int lierre_thread_create(lierre_thread_t *thread,
                         void *(*start_routine)(void *),
                         void *arg);                       // Create platform thread
int lierre_thread_join(lierre_thread_t thread,
                       void **retval);                    // Join platform thread
```

## SIMD Support

The library enables SIMD code paths only when both the target architecture and compiler flags support them. Scalar implementations remain available on every target.

| Target | SIMD path | Enablement |
|--------|-----------|------------|
| x86_64 / x86 | AVX2 | Enabled when the compiler accepts `-mavx2` |
| ARM / ARM64 | NEON | Uses NEON code paths when the toolchain exposes NEON intrinsics |
| WebAssembly | SIMD128 | Enabled when the compiler accepts `-msimd128` |
| Any target | Scalar fallback | Always available |

To disable SIMD optimizations:
```bash
cmake -B build -DLIERRE_USE_SIMD=OFF
```

When `LIERRE_USE_VALGRIND=ON` is active in a Debug build, CMake forces SIMD off to keep Valgrind runs reliable.

## Code Coverage

To generate coverage reports, use a GCC Debug build with `lcov` and `genhtml` installed:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
               -DLIERRE_USE_TESTS=ON \
               -DLIERRE_USE_COVERAGE=ON
cmake --build build
cmake --build build --target coverage
```

The HTML report will be generated at `build/coverage/html/index.html`.

If those tools are missing, or if a non-GCC compiler is selected, coverage is disabled automatically.

## Integration

### Using CMake `find_package`

After installation, use in your CMakeLists.txt:

```cmake
find_package(Lierre REQUIRED)
target_link_libraries(your_target PRIVATE lierre::lierre)
```

### Using CMake `add_subdirectory`

Add liblierre as a subdirectory in your project:

```cmake
add_subdirectory(path/to/liblierre)
target_link_libraries(your_target PRIVATE lierre)
```

### Integrating with Existing libpoporon

If your project already uses libpoporon, you can avoid duplicate symbol conflicts:

```cmake
# Your existing libpoporon setup
add_subdirectory(your/path/to/libpoporon)

# liblierre will detect the existing poporon target and skip its bundled version
add_subdirectory(path/to/liblierre)

target_link_libraries(your_target PRIVATE lierre)
```

## Project Structure

```
liblierre/
├── include/
│   ├── lierre.h           # Main header (types, errors, utilities)
│   └── lierre/
│       ├── reader.h       # QR reader API
│       ├── writer.h       # QR writer API
│       └── portable.h     # Cross-platform threading
├── src/
│   ├── lierre.c           # Core utilities
│   ├── image.c            # Image processing
│   ├── portable.c         # Platform abstraction
│   ├── decode/            # QR decoding implementation
│   │   ├── decode_qr.c    # QR decode logic
│   │   ├── decoder.c      # Main decoder
│   │   ├── decoder_detect.c   # QR detection
│   │   ├── decoder_grid.c     # Grid processing
│   │   └── reader.c       # Reader interface
│   ├── encode/            # QR encoding implementation
│   │   └── writer.c       # Writer interface
│   └── internal/          # Internal headers
│       ├── decoder.h      # Decoder internals
│       ├── image.h        # Image processing internals
│       ├── memory.h       # Memory management
│       ├── simd.h         # SIMD abstractions
│       └── structs.h      # Internal structures
├── tests/                 # Unit tests (Unity framework)
│   ├── test_lierre.c      # Core tests
│   ├── test_portable.c    # Threading tests
│   ├── test_qr_codec.c    # Encode/decode tests
│   ├── test_reader.c      # Reader tests
│   └── test_writer.c      # Writer tests
├── third_party/
│   ├── emsdk/             # Emscripten SDK (optional)
│   ├── libpoporon/        # Reed-Solomon library
│   ├── unity/             # Unity Test framework
│   └── valgrind/          # Valgrind (source, optional)
└── cmake/                 # CMake modules
    ├── buildtime.cmake    # Build timestamp
    ├── emscripten.cmake   # WebAssembly support
    ├── test.cmake         # Test configuration
    └── LierreConfig.cmake.in  # CMake package config
```

## Dependencies

- **[libpoporon](https://github.com/zeriyoshi/libpoporon)** - Reed-Solomon error correction library (submodule)
- **[Unity](https://github.com/ThrowTheSwitch/Unity)** - Unit testing framework (submodule, tests only)
- **[Emscripten SDK](https://emscripten.org/)** - WebAssembly build/test support (submodule, optional)
- **[Valgrind](https://valgrind.org/)** - Memory checking tool (submodule, optional)

## Acknowledgments

The QR code decoding implementation in this library is derived from [quirc](https://github.com/dlbeer/quirc).

> Copyright (C) 2010-2012 Daniel Beer \<dlbeer@gmail.com\>
>
> Licensed under the ISC License.

See [LICENSE](LICENSE) for the bundled license texts and [NOTICE](NOTICE) for the third-party notice summary.

## License

The liblierre code in this repository is licensed under the MIT License. Bundled third-party components remain under their own permissive licenses.

See [LICENSE](LICENSE) for the MIT license text together with bundled third-party license texts, and [NOTICE](NOTICE) for the third-party notice summary.

## Author

**Go Kudo** ([@zeriyoshi](https://github.com/zeriyoshi)) - [zeriyoshi@gmail.com](mailto:zeriyoshi@gmail.com)

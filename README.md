# liblierre

[![CI](https://github.com/zeriyoshi/liblierre/actions/workflows/ci.yaml/badge.svg)](https://github.com/zeriyoshi/liblierre/actions/workflows/ci.yaml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**Lightweight Image Encoding & Reading for Resilient Encoded data**

[🇯🇵 日本語版 README はこちら](README_ja.md)

liblierre is a lightweight, high-performance QR code encoding and decoding library written in C99. It provides complete QR code generation and reading capabilities with optional SIMD acceleration for maximum performance.

## Features

- **Pure C99 Implementation** - No external dependencies (except libpoporon for Reed-Solomon), portable across platforms
- **QR Code Generation** - Support for all QR versions (1-40), all error correction levels (L/M/Q/H), all mask patterns (0-7)
- **QR Code Reading** - Advanced image preprocessing with multiple detection strategies
- **SIMD Acceleration** - Automatic optimization using AVX2 (x86_64), NEON (ARM64), or WASM SIMD128
- **WebAssembly Support** - Can be compiled to WASM using Emscripten
- **Memory Safe** - Carefully designed API with proper resource management
- **Extensive Testing** - Comprehensive test suite with sanitizer and Valgrind support

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
    lierre_writer_param_init(
        &param,
        (uint8_t *)text,
        strlen(text),
        4,           // scale (4x)
        2,           // margin (2 modules)
        ECC_MEDIUM,  // error correction level
        MASK_AUTO,   // auto-select mask pattern
        MODE_BYTE    // byte mode
    );

    // Get output resolution
    lierre_reso_t res;
    lierre_writer_get_res(&param, &res);
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
    lierre_reader_param_init(&param);

    // Optional: Enable preprocessing strategies
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_GLAYSCALE);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_DENOISE);

    // Create reader and decode
    lierre_reader_t *reader = lierre_reader_create(&param);
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
LIERRE_READER_STRATEGY_GLAYSCALE         // Convert to grayscale
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

## SIMD Support

The library automatically detects and enables SIMD optimizations based on the target architecture:

| Platform | SIMD | Status |
|----------|------|--------|
| Linux x86_64 | AVX2 | ✅ Fully supported |
| Linux ARM64 | NEON | ✅ Fully supported |
| macOS x86_64 | AVX2 | ✅ Fully supported |
| macOS ARM64 | NEON | ✅ Fully supported |
| Windows x86_64 | AVX2 | ✅ Fully supported |
| WebAssembly | SIMD128 | ✅ Fully supported |

To disable SIMD optimizations:
```bash
cmake -B build -DLIERRE_USE_SIMD=OFF
```

## Code Coverage

To generate coverage reports (requires GCC, `lcov`, and `genhtml`):

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
               -DLIERRE_USE_TESTS=ON \
               -DLIERRE_USE_COVERAGE=ON
cmake --build build
cmake --build build --target coverage
```

The HTML report will be generated at `build/coverage/html/index.html`.

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
│   ├── libpoporon/        # Reed-Solomon library
│   ├── unity/             # Unity Test framework
│   └── valgrind/          # Valgrind headers
└── cmake/                 # CMake modules
    ├── buildtime.cmake    # Build timestamp
    ├── emscripten.cmake   # WebAssembly support
    ├── test.cmake         # Test configuration
    └── LierreConfig.cmake.in  # CMake package config
```

## Dependencies

- **[libpoporon](https://github.com/zeriyoshi/libpoporon)** - Reed-Solomon error correction library (bundled)
- **[Unity](https://github.com/ThrowTheSwitch/Unity)** - Unit testing framework (bundled, tests only)

## License

MIT License - see [LICENSE](LICENSE) for details.

## Author

**Go Kudo** ([@zeriyoshi](https://github.com/zeriyoshi)) - [zeriyoshi@gmail.com](mailto:zeriyoshi@gmail.com)

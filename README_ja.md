# liblierre

[![CI](https://github.com/zeriyoshi/liblierre/actions/workflows/ci.yaml/badge.svg)](https://github.com/zeriyoshi/liblierre/actions/workflows/ci.yaml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**Lightweight Image Encoding & Reading for Resilient Encoded data**

[🇺🇸 English README](README.md)

liblierre は、C99で書かれた軽量・高性能なQRコードエンコード・デコードライブラリです。完全なQRコード生成・読み取り機能を提供し、オプションのSIMD最適化により最大のパフォーマンスを実現します。

## 特徴

- **純粋なC99実装** - 外部依存なし（Reed-Solomon用のlibpoporonを除く）、プラットフォーム間で移植可能
- **QRコード生成** - 全QRバージョン（1-40）、全誤り訂正レベル（L/M/Q/H）、全マスクパターン（0-7）をサポート
- **QRコード読み取り** - 複数の検出戦略による高度な画像前処理
- **SIMD最適化** - AVX2（x86_64）、NEON（ARM64）、WASM SIMD128による自動最適化
- **WebAssemblyサポート** - Emscriptenを使用してWASMにコンパイル可能
- **メモリ安全** - 適切なリソース管理を備えた慎重に設計されたAPI
- **広範なテスト** - サニタイザとValgrindをサポートする包括的なテストスイート

## クイックスタート

### ビルド

```bash
# サブモジュールを含めてリポジトリをクローン
git clone --recursive https://github.com/zeriyoshi/liblierre.git
cd liblierre

# CMakeでビルド
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### ビルドオプション

| オプション | デフォルト | 説明 |
|-----------|-----------|------|
| `LIERRE_USE_SIMD` | `ON` | SIMD最適化を有効化 |
| `LIERRE_USE_TESTS` | `OFF` | テストスイートをビルド |
| `LIERRE_USE_VALGRIND` | `OFF` | Valgrindメモリチェックを有効化 |
| `LIERRE_USE_COVERAGE` | `OFF` | コードカバレッジを有効化 |
| `LIERRE_USE_ASAN` | `OFF` | AddressSanitizerを有効化 |
| `LIERRE_USE_MSAN` | `OFF` | MemorySanitizerを有効化 |
| `LIERRE_USE_UBSAN` | `OFF` | UndefinedBehaviorSanitizerを有効化 |
| `BUILD_SHARED_LIBS` | `OFF` | 共有ライブラリをビルド |

### テストの実行

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DLIERRE_USE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## 使用例

### QRコードの生成

```c
#include <lierre.h>
#include <lierre/writer.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    const char *text = "Hello, World!";
    lierre_writer_param_t param;
    lierre_rgba_t fill = {0, 0, 0, 255};      // 黒
    lierre_rgba_t bg = {255, 255, 255, 255};  // 白

    // パラメータを初期化
    lierre_writer_param_init(
        &param,
        (uint8_t *)text,
        strlen(text),
        4,           // スケール（4倍）
        2,           // マージン（2モジュール）
        ECC_MEDIUM,  // 誤り訂正レベル
        MASK_AUTO,   // マスクパターン自動選択
        MODE_BYTE    // バイトモード
    );

    // 出力解像度を取得
    lierre_reso_t res;
    lierre_writer_get_res(&param, &res);
    printf("QRコードサイズ: %zux%zu\n", res.width, res.height);

    // ライターを作成して生成
    lierre_writer_t *writer = lierre_writer_create(&param, &fill, &bg);
    if (!writer) {
        fprintf(stderr, "ライターの作成に失敗しました\n");
        return 1;
    }

    if (lierre_writer_write(writer) == LIERRE_ERROR_SUCCESS) {
        const uint8_t *rgba = lierre_writer_get_rgba_data(writer);
        size_t size = lierre_writer_get_rgba_data_size(writer);
        printf("%zuバイトのRGBAデータを生成しました\n", size);
        // RGBAデータを使用（ファイル保存、表示など）
    }

    lierre_writer_destroy(writer);
    return 0;
}
```

### QRコードの読み取り

```c
#include <lierre.h>
#include <lierre/reader.h>
#include <stdio.h>

int main(void) {
    // RGB画像データがあると仮定
    uint8_t *rgb_data = /* 画像データ */;
    size_t width = 640, height = 480;

    // RGBデータラッパーを作成
    lierre_rgb_data_t *rgb = lierre_rgb_create(
        rgb_data, width * height * 3, width, height
    );
    if (!rgb) {
        fprintf(stderr, "RGBコンテナの作成に失敗しました\n");
        return 1;
    }

    // リーダーパラメータを初期化
    lierre_reader_param_t param;
    lierre_reader_param_init(&param);

    // オプション: 前処理戦略を有効化
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_GLAYSCALE);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_DENOISE);

    // リーダーを作成してデコード
    lierre_reader_t *reader = lierre_reader_create(&param);
    lierre_reader_set_data(reader, rgb);

    lierre_reader_result_t *result = NULL;
    if (lierre_reader_read(reader, &result) == LIERRE_ERROR_SUCCESS) {
        uint32_t count = lierre_reader_result_get_num_qr_codes(result);
        printf("%u個のQRコードが見つかりました\n", count);

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

## APIリファレンス

### コア型

```c
// エラーコード
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

// RGB画像データコンテナ
typedef struct {
    uint8_t *data;
    size_t data_size;
    size_t width;
    size_t height;
} lierre_rgb_data_t;

// RGBAカラー
typedef struct {
    uint8_t r, g, b, a;
} lierre_rgba_t;

// 2Dベクトルと矩形
typedef struct { size_t x, y; } lierre_vec2_t;
typedef struct { size_t width, height; } lierre_reso_t;
typedef struct { lierre_vec2_t origin; lierre_reso_t size; } lierre_rect_t;
```

### Writer API

```c
// 誤り訂正レベル
ECC_LOW, ECC_MEDIUM, ECC_QUARTILE, ECC_HIGH

// マスクパターン
MASK_AUTO, MASK_0 〜 MASK_7

// エンコードモード
MODE_NUMERIC, MODE_ALPHANUMERIC, MODE_BYTE, MODE_KANJI, MODE_ECI

// 関数
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
// 戦略フラグ（| で組み合わせ可能）
LIERRE_READER_STRATEGY_NONE
LIERRE_READER_STRATEGY_MINIMIZE          // 検出を容易にするため画像を縮小
LIERRE_READER_STRATEGY_GLAYSCALE         // グレースケールに変換
LIERRE_READER_STRATEGY_USE_RECT          // 特定領域にフォーカス
LIERRE_READER_STRATEGY_DENOISE           // ノイズ除去フィルタを適用
LIERRE_READER_STRATEGY_BRIGHTNESS_NORMALIZE  // 輝度を正規化
LIERRE_READER_STRATEGY_CONTRAST_NORMALIZE    // コントラストを正規化
LIERRE_READER_STRATEGY_SHARPENING        // シャープニングフィルタを適用
LIERRE_READER_STRATEGY_MT                // マルチスレッドを有効化

// 関数
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

### ユーティリティ関数

```c
const char *lierre_strerror(lierre_error_t err);   // エラーメッセージを取得
uint32_t lierre_version_id(void);                  // ライブラリバージョンを取得
lierre_buildtime_t lierre_buildtime(void);         // ビルドタイムスタンプを取得
lierre_rgb_data_t *lierre_rgb_create(...);         // RGBコンテナを作成
void lierre_rgb_destroy(lierre_rgb_data_t *rgb);   // RGBコンテナを破棄
```

## SIMDサポート

ライブラリはターゲットアーキテクチャに基づいてSIMD最適化を自動的に検出し有効化します：

| プラットフォーム | SIMD | 状態 |
|-----------------|------|------|
| Linux x86_64 | AVX2 | ✅ 完全サポート |
| Linux ARM64 | NEON | ✅ 完全サポート |
| macOS x86_64 | AVX2 | ✅ 完全サポート |
| macOS ARM64 | NEON | ✅ 完全サポート |
| Windows x86_64 | AVX2 | ✅ 完全サポート |
| WebAssembly | SIMD128 | ✅ 完全サポート |

SIMD最適化を無効にするには：
```bash
cmake -B build -DLIERRE_USE_SIMD=OFF
```

## コードカバレッジ

カバレッジレポートを生成するには（GCC、`lcov`、`genhtml`が必要）：

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
               -DLIERRE_USE_TESTS=ON \
               -DLIERRE_USE_COVERAGE=ON
cmake --build build
cmake --build build --target coverage
```

HTMLレポートは `build/coverage/html/index.html` に生成されます。

## 統合

### CMake `find_package` を使用

インストール後、CMakeLists.txtで使用：

```cmake
find_package(Lierre REQUIRED)
target_link_libraries(your_target PRIVATE lierre::lierre)
```

### CMake `add_subdirectory` を使用

プロジェクトにliblierre をサブディレクトリとして追加：

```cmake
add_subdirectory(path/to/liblierre)
target_link_libraries(your_target PRIVATE lierre)
```

### 既存のlibpoporonとの統合

プロジェクトですでにlibpoporonを使用している場合、シンボルの重複を回避できます：

```cmake
# 既存のlibpoporon設定
add_subdirectory(your/path/to/libpoporon)

# liblierre は既存のpoporonターゲットを検出し、バンドル版をスキップします
add_subdirectory(path/to/liblierre)

target_link_libraries(your_target PRIVATE lierre)
```

## プロジェクト構造

```
liblierre/
├── include/
│   ├── lierre.h           # メインヘッダー（型、エラー、ユーティリティ）
│   └── lierre/
│       ├── reader.h       # QRリーダーAPI
│       ├── writer.h       # QRライターAPI
│       └── portable.h     # クロスプラットフォームスレッド
├── src/
│   ├── lierre.c           # コアユーティリティ
│   ├── image.c            # 画像処理
│   ├── portable.c         # プラットフォーム抽象化
│   ├── decode/            # QRデコード実装
│   │   ├── decode_qr.c    # QRデコードロジック
│   │   ├── decoder.c      # メインデコーダー
│   │   ├── decoder_detect.c   # QR検出
│   │   ├── decoder_grid.c     # グリッド処理
│   │   └── reader.c       # リーダーインターフェース
│   ├── encode/            # QRエンコード実装
│   │   └── writer.c       # ライターインターフェース
│   └── internal/          # 内部ヘッダー
│       ├── decoder.h      # デコーダー内部
│       ├── image.h        # 画像処理内部
│       ├── memory.h       # メモリ管理
│       ├── simd.h         # SIMD抽象化
│       └── structs.h      # 内部構造体
├── tests/                 # ユニットテスト（Unityフレームワーク）
│   ├── test_lierre.c      # コアテスト
│   ├── test_portable.c    # スレッドテスト
│   ├── test_qr_codec.c    # エンコード/デコードテスト
│   ├── test_reader.c      # リーダーテスト
│   └── test_writer.c      # ライターテスト
├── third_party/
│   ├── libpoporon/        # Reed-Solomonライブラリ
│   ├── unity/             # Unity Testフレームワーク
│   └── valgrind/          # Valgrindヘッダー
└── cmake/                 # CMakeモジュール
    ├── buildtime.cmake    # ビルドタイムスタンプ
    ├── emscripten.cmake   # WebAssemblyサポート
    ├── test.cmake         # テスト設定
    └── LierreConfig.cmake.in  # CMakeパッケージ設定
```

## 依存関係

- **[libpoporon](https://github.com/zeriyoshi/libpoporon)** - Reed-Solomon誤り訂正ライブラリ（バンドル）
- **[Unity](https://github.com/ThrowTheSwitch/Unity)** - ユニットテストフレームワーク（バンドル、テストのみ）

## ライセンス

MIT License - 詳細は [LICENSE](LICENSE) を参照してください。

## 著者

**Go Kudo** ([@zeriyoshi](https://github.com/zeriyoshi)) - [zeriyoshi@gmail.com](mailto:zeriyoshi@gmail.com)

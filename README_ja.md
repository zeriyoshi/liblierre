# liblierre

[![CI](https://github.com/zeriyoshi/liblierre/actions/workflows/ci.yaml/badge.svg)](https://github.com/zeriyoshi/liblierre/actions/workflows/ci.yaml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**Lightweight Image Encoding & Reading for Resilient Encoded data**

[🇺🇸 English README](README.md)

liblierre は、C99で書かれた軽量な QR コードのエンコード・デコードライブラリです。QR コード生成と読み取りを提供し、オプションの SIMD 最適化と移植性の高いフォールバック実装を備えています。

## 特徴

- **C99実装** - Reed-Solomon 誤り訂正のために同梱された libpoporon を用いる移植性の高い C 実装
- **QRコード生成** - 全QRバージョン（1-40）、全誤り訂正レベル（L/M/Q/H）、全マスクパターン（0-7）をサポート
- **QRコード読み取り** - RGB 入力画像に対する前処理戦略と複数コード検出をサポート
- **SIMD最適化** - AVX2（x86_64）、NEON（ARM/ARM64）、WASM SIMD128 の各コードパスを条件付きで利用し、常にスカラ実装へフォールバック可能
- **WebAssemblyサポート** - Emscriptenを使用してWASMにコンパイル可能
- **メモリ管理を意識したAPI** - リソース所有権が明示的で、Sanitizer や Valgrind で検証しやすい設計
- **広範なテスト** - writer、reader、portable helper、codec 挙動を対象にしたユニットテストと round-trip テスト

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

補足:

- `LIERRE_USE_ASAN`、`LIERRE_USE_MSAN`、`LIERRE_USE_UBSAN` は Clang の Debug ビルドでのみ有効です。
- `LIERRE_USE_COVERAGE` は GCC の Debug ビルドかつ `lcov` と `genhtml` が利用可能な場合のみ有効です。
- `LIERRE_USE_VALGRIND` は Debug ビルドで `valgrind` が見つかった場合のみ有効で、その際は `LIERRE_USE_SIMD=OFF` が強制されます。

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
    lierre_error_t err = lierre_writer_param_init(
        &param,
        (uint8_t *)text,
        strlen(text),
        4,           // スケール（4倍）
        2,           // マージン（2モジュール）
        ECC_MEDIUM,  // 誤り訂正レベル
        MASK_AUTO,   // マスクパターン自動選択
        MODE_BYTE    // バイトモード
    );
    if (err != LIERRE_ERROR_SUCCESS) {
        fprintf(stderr, "パラメータの初期化に失敗しました: %s\n", lierre_strerror(err));
        return 1;
    }

    // 出力解像度を取得
    lierre_reso_t res;
    if (!lierre_writer_get_res(&param, &res)) {
        fprintf(stderr, "出力解像度の計算に失敗しました\n");
        return 1;
    }
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
    if (lierre_reader_param_init(&param) != LIERRE_ERROR_SUCCESS) {
        fprintf(stderr, "リーダーパラメータの初期化に失敗しました\n");
        lierre_rgb_destroy(rgb);
        return 1;
    }

    // オプション: 前処理戦略を有効化
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_GRAYSCALE);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_DENOISE);

    // リーダーを作成してデコード
    lierre_reader_t *reader = lierre_reader_create(&param);
    if (!reader) {
        fprintf(stderr, "リーダーの作成に失敗しました\n");
        lierre_rgb_destroy(rgb);
        return 1;
    }
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
    } else {
        fprintf(stderr, "QRコードのデコードに失敗しました\n");
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
// 戦略フラグ（| で組み合わせ可能）
LIERRE_READER_STRATEGY_NONE
LIERRE_READER_STRATEGY_MINIMIZE          // 検出を容易にするため画像を縮小
LIERRE_READER_STRATEGY_GRAYSCALE         // グレースケールに変換
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

### Portable API

```c
uint32_t lierre_get_cpu_count(void);                       // 論理 CPU 数を取得
int lierre_thread_create(lierre_thread_t *thread,
                         void *(*start_routine)(void *),
                         void *arg);                       // プラットフォームスレッドを生成
int lierre_thread_join(lierre_thread_t thread,
                       void **retval);                    // プラットフォームスレッドを join
```

## SIMDサポート

ライブラリは、ターゲットアーキテクチャとコンパイラフラグの両方が対応している場合にのみ SIMD コードパスを有効化します。どの環境でもスカラ実装は利用可能です。

| ターゲット | SIMD パス | 有効化条件 |
|-----------|-----------|--------------|
| x86_64 / x86 | AVX2 | コンパイラが `-mavx2` を受け付ける場合に有効 |
| ARM / ARM64 | NEON | ツールチェーンが NEON intrinsic を公開している場合に利用 |
| WebAssembly | SIMD128 | コンパイラが `-msimd128` を受け付ける場合に有効 |
| 任意のターゲット | スカラ実装 | 常に利用可能 |

SIMD最適化を無効にするには：
```bash
cmake -B build -DLIERRE_USE_SIMD=OFF
```

Debug ビルドで `LIERRE_USE_VALGRIND=ON` を有効にした場合、Valgrind 実行を安定させるために CMake が SIMD を無効化します。

## コードカバレッジ

カバレッジレポートを生成するには、GCC の Debug ビルドで `lcov` と `genhtml` を利用可能にしてください：

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
               -DLIERRE_USE_TESTS=ON \
               -DLIERRE_USE_COVERAGE=ON
cmake --build build
cmake --build build --target coverage
```

HTMLレポートは `build/coverage/html/index.html` に生成されます。

これらのツールが見つからない場合、または GCC 以外のコンパイラを使う場合、coverage は自動的に無効化されます。

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
│   ├── emsdk/             # Emscripten SDK（オプション）
│   ├── libpoporon/        # Reed-Solomonライブラリ
│   ├── unity/             # Unity Testフレームワーク
│   └── valgrind/          # Valgrind（ソース、オプション）
└── cmake/                 # CMakeモジュール
    ├── buildtime.cmake    # ビルドタイムスタンプ
    ├── emscripten.cmake   # WebAssemblyサポート
    ├── test.cmake         # テスト設定
    └── LierreConfig.cmake.in  # CMakeパッケージ設定
```

## 依存関係

- **[libpoporon](https://github.com/zeriyoshi/libpoporon)** - Reed-Solomon誤り訂正ライブラリ（submodule）
- **[Unity](https://github.com/ThrowTheSwitch/Unity)** - ユニットテストフレームワーク（submodule、テストのみ）
- **[Emscripten SDK](https://emscripten.org/)** - WebAssemblyビルド/テストサポート（submodule、オプション）
- **[Valgrind](https://valgrind.org/)** - メモリチェックツール（submodule、オプション）

## 謝辞

本ライブラリの QR コードデコード実装は、[quirc](https://github.com/dlbeer/quirc) に基づいています。

> Copyright (C) 2010-2012 Daniel Beer \<dlbeer@gmail.com\>
>
> Licensed under the ISC License.

[LICENSE](LICENSE) に同梱ライセンス全文を、[NOTICE](NOTICE) に第三者通知の一覧を記載しています。

## ライセンス

このリポジトリ内の liblierre 本体コードは MIT License です。同梱されている第三者コンポーネントには、それぞれ固有の許諾条件が適用されます。

MIT 本文と同梱第三者ライセンス全文は [LICENSE](LICENSE) を、第三者通知の一覧は [NOTICE](NOTICE) を参照してください。

## 著者

**Go Kudo** ([@zeriyoshi](https://github.com/zeriyoshi)) - [zeriyoshi@gmail.com](mailto:zeriyoshi@gmail.com)

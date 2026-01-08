/*
 * liblierre - image.c
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "internal/image.h"

#include <string.h>

#include "internal/memory.h"

#if LIERRE_USE_SIMD
#include "internal/simd.h"
#endif

#define LIERRE_PIXEL_VALUE_MIN     0
#define LIERRE_PIXEL_VALUE_MAX     255
#define LIERRE_MIN_QR_SIZE         21
#define LIERRE_CONTRAST_FACTOR     128
#define LIERRE_CONTRAST_DIVISOR    100
#define LIERRE_FILTER_KERNEL_SIZE  3
#define LIERRE_FILTER_KERNEL_ELEMS 9
#define LIERRE_SHARPEN_CENTER_COEF 5

typedef struct {
    const uint8_t *src;
    uint8_t *dst;
    size_t src_width;
    size_t dst_width;
    size_t start_row;
    size_t end_row;
} lierre_image_mt_minimize_ctx_t;

typedef struct {
    uint8_t *image;
    uint8_t *temp;
    size_t width;
    size_t height;
    size_t start_row;
    size_t end_row;
} lierre_image_mt_filter_ctx_t;

#define LIERRE_IMAGE_MT_MAX_THREADS          64
#define LIERRE_IMAGE_MINIMIZE_MAX_ITERATIONS 4

static inline void lierre_minimize_row(const uint8_t *src_row0, const uint8_t *src_row1, uint8_t *dst, size_t dst_width)
{
#if LIERRE_USE_SIMD && defined(LIERRE_SIMD_AVX2)
    __m256i data0, data1, row0_even, row0_odd, row1_even, row1_odd, sum, avg;
    __m128i packed;
    size_t x, simd_count, sx;
    int32_t sum_scalar;

    simd_count = dst_width & ~15ULL;

    for (x = 0; x < simd_count; x += 16) {
        data0 = _mm256_loadu_si256((const __m256i *)(src_row0 + x * 2));
        data1 = _mm256_loadu_si256((const __m256i *)(src_row1 + x * 2));

        row0_even = _mm256_and_si256(data0, _mm256_set1_epi16(0x00FF));
        row0_odd = _mm256_srli_epi16(data0, 8);
        row1_even = _mm256_and_si256(data1, _mm256_set1_epi16(0x00FF));
        row1_odd = _mm256_srli_epi16(data1, 8);

        sum = _mm256_add_epi16(row0_even, row0_odd);
        sum = _mm256_add_epi16(sum, row1_even);
        sum = _mm256_add_epi16(sum, row1_odd);

        avg = _mm256_srli_epi16(sum, 2);

        packed = _mm_packus_epi16(_mm256_castsi256_si128(avg), _mm256_extracti128_si256(avg, 1));
        _mm_storeu_si128((__m128i *)(dst + x), packed);
    }

    for (x = simd_count; x < dst_width; x++) {
        sx = x * 2;
        sum_scalar =
            (int32_t)src_row0[sx] + (int32_t)src_row0[sx + 1] + (int32_t)src_row1[sx] + (int32_t)src_row1[sx + 1];
        dst[x] = (uint8_t)(sum_scalar >> 2);
    }
#elif LIERRE_USE_SIMD && defined(LIERRE_SIMD_NEON)
    uint8x16x2_t row0, row1;
    uint16x8_t sum_lo, sum_hi;
    uint8x8_t avg_lo, avg_hi;
    uint8x16_t result;
    size_t x, simd_count, sx;
    int32_t sum_scalar;

    simd_count = dst_width & ~15ULL;

    for (x = 0; x < simd_count; x += 16) {
        row0 = vld2q_u8(src_row0 + x * 2);
        row1 = vld2q_u8(src_row1 + x * 2);

        sum_lo = vaddl_u8(vget_low_u8(row0.val[0]), vget_low_u8(row0.val[1]));
        sum_lo = vaddw_u8(sum_lo, vget_low_u8(row1.val[0]));
        sum_lo = vaddw_u8(sum_lo, vget_low_u8(row1.val[1]));

        sum_hi = vaddl_u8(vget_high_u8(row0.val[0]), vget_high_u8(row0.val[1]));
        sum_hi = vaddw_u8(sum_hi, vget_high_u8(row1.val[0]));
        sum_hi = vaddw_u8(sum_hi, vget_high_u8(row1.val[1]));

        avg_lo = vshrn_n_u16(sum_lo, 2);
        avg_hi = vshrn_n_u16(sum_hi, 2);
        result = vcombine_u8(avg_lo, avg_hi);

        vst1q_u8(dst + x, result);
    }

    for (x = simd_count; x < dst_width; x++) {
        sx = x * 2;
        sum_scalar =
            (int32_t)src_row0[sx] + (int32_t)src_row0[sx + 1] + (int32_t)src_row1[sx] + (int32_t)src_row1[sx + 1];
        dst[x] = (uint8_t)(sum_scalar >> 2);
    }
#elif LIERRE_USE_SIMD && defined(LIERRE_SIMD_WASM)
    v128_t data0, data1, row0_even, row0_odd, row1_even, row1_odd, sum, avg, mask = wasm_i16x8_splat(0x00FF);
    size_t x, simd_count = dst_width & ~15ULL;

    for (x = 0; x < simd_count; x += 16) {
        data0 = wasm_v128_load(src_row0 + x * 2);
        data1 = wasm_v128_load(src_row1 + x * 2);

        row0_even = wasm_v128_and(data0, mask);
        row0_odd = wasm_u16x8_shr(data0, 8);
        row1_even = wasm_v128_and(data1, mask);
        row1_odd = wasm_u16x8_shr(data1, 8);

        sum = wasm_i16x8_add(row0_even, row0_odd);
        sum = wasm_i16x8_add(sum, row1_even);
        sum = wasm_i16x8_add(sum, row1_odd);

        avg = wasm_u16x8_shr(sum, 2);

        data0 = wasm_v128_load(src_row0 + x * 2 + 16);
        data1 = wasm_v128_load(src_row1 + x * 2 + 16);

        row0_even = wasm_v128_and(data0, mask);
        row0_odd = wasm_u16x8_shr(data0, 8);
        row1_even = wasm_v128_and(data1, mask);
        row1_odd = wasm_u16x8_shr(data1, 8);

        sum = wasm_i16x8_add(row0_even, row0_odd);
        sum = wasm_i16x8_add(sum, row1_even);
        sum = wasm_i16x8_add(sum, row1_odd);
        sum = wasm_u16x8_shr(sum, 2);

        wasm_v128_store(dst + x, wasm_u8x16_narrow_i16x8(avg, sum));
    }

    {
        size_t sx;
        int32_t sum_scalar;
        for (x = simd_count; x < dst_width; x++) {
            sx = x * 2;
            sum_scalar =
                (int32_t)src_row0[sx] + (int32_t)src_row0[sx + 1] + (int32_t)src_row1[sx] + (int32_t)src_row1[sx + 1];
            dst[x] = (uint8_t)(sum_scalar >> 2);
        }
    }
#else
    size_t x, sx;
    int32_t sum_scalar;

    for (x = 0; x < dst_width; x++) {
        sx = x * 2;
        sum_scalar =
            (int32_t)src_row0[sx] + (int32_t)src_row0[sx + 1] + (int32_t)src_row1[sx] + (int32_t)src_row1[sx + 1];
        dst[x] = (uint8_t)(sum_scalar >> 2);
    }
#endif
}

static inline void lierre_find_minmax(const uint8_t *image, size_t total, uint8_t *out_min, uint8_t *out_max)
{
#if LIERRE_USE_SIMD && defined(LIERRE_SIMD_AVX2)
    __m256i min_vec = _mm256_set1_epi8((char)255), max_vec = _mm256_setzero_si256(), data;
    __m128i min128, max128;
    uint8_t min_arr[32], max_arr[32], min_val, max_val;
    size_t i, simd_count = total & ~31ULL;

    for (i = 0; i < simd_count; i += 32) {
        data = _mm256_loadu_si256((const __m256i *)(image + i));
        min_vec = _mm256_min_epu8(min_vec, data);
        max_vec = _mm256_max_epu8(max_vec, data);
    }

    min128 = _mm_min_epu8(_mm256_castsi256_si128(min_vec), _mm256_extracti128_si256(min_vec, 1));
    max128 = _mm_max_epu8(_mm256_castsi256_si128(max_vec), _mm256_extracti128_si256(max_vec, 1));

    _mm_storeu_si128((__m128i *)min_arr, min128);
    _mm_storeu_si128((__m128i *)max_arr, max128);

    min_val = 255;
    max_val = 0;
    for (i = 0; i < 16; i++) {
        if (min_arr[i] < min_val) {
            min_val = min_arr[i];
        }
        if (max_arr[i] > max_val) {
            max_val = max_arr[i];
        }
    }

    for (i = simd_count; i < total; i++) {
        if (image[i] < min_val) {
            min_val = image[i];
        }
        if (image[i] > max_val) {
            max_val = image[i];
        }
    }

    *out_min = min_val;
    *out_max = max_val;
#elif LIERRE_USE_SIMD && defined(LIERRE_SIMD_NEON)
    uint8x16_t min_vec, max_vec, data;
    uint8_t min_val, max_val;
    size_t i, simd_count;

    min_vec = vdupq_n_u8(255);
    max_vec = vdupq_n_u8(0);

    simd_count = total & ~15ULL;

    for (i = 0; i < simd_count; i += 16) {
        data = vld1q_u8(image + i);
        min_vec = vminq_u8(min_vec, data);
        max_vec = vmaxq_u8(max_vec, data);
    }

    min_val = vminvq_u8(min_vec);
    max_val = vmaxvq_u8(max_vec);

    for (i = simd_count; i < total; i++) {
        if (image[i] < min_val) {
            min_val = image[i];
        }
        if (image[i] > max_val) {
            max_val = image[i];
        }
    }

    *out_min = min_val;
    *out_max = max_val;
#elif LIERRE_USE_SIMD && defined(LIERRE_SIMD_WASM)
    v128_t min_vec, max_vec, data;
    uint8_t min_arr[16], max_arr[16], min_val, max_val;
    size_t i, simd_count;

    min_vec = wasm_u8x16_splat(255);
    max_vec = wasm_u8x16_splat(0);

    simd_count = total & ~15ULL;

    for (i = 0; i < simd_count; i += 16) {
        data = wasm_v128_load(image + i);
        min_vec = wasm_u8x16_min(min_vec, data);
        max_vec = wasm_u8x16_max(max_vec, data);
    }

    wasm_v128_store(min_arr, min_vec);
    wasm_v128_store(max_arr, max_vec);

    min_val = 255;
    max_val = 0;
    for (i = 0; i < 16; i++) {
        if (min_arr[i] < min_val) {
            min_val = min_arr[i];
        }
        if (max_arr[i] > max_val) {
            max_val = max_arr[i];
        }
    }

    for (i = simd_count; i < total; i++) {
        if (image[i] < min_val) {
            min_val = image[i];
        }
        if (image[i] > max_val) {
            max_val = image[i];
        }
    }

    *out_min = min_val;
    *out_max = max_val;
#else
    uint8_t min_val, max_val;
    size_t i;

    min_val = 255;
    max_val = 0;

    for (i = 0; i < total; i++) {
        if (image[i] < min_val) {
            min_val = image[i];
        }
        if (image[i] > max_val) {
            max_val = image[i];
        }
    }

    *out_min = min_val;
    *out_max = max_val;
#endif
}

static inline int64_t lierre_sum(const uint8_t *image, size_t total)
{
#if LIERRE_USE_SIMD && defined(LIERRE_SIMD_AVX2)
    __m256i sum_vec, data, zero;
    int64_t sum;
    uint64_t partial[4];
    size_t i, simd_count;

    sum_vec = _mm256_setzero_si256();
    zero = _mm256_setzero_si256();

    simd_count = total & ~31ULL;

    for (i = 0; i < simd_count; i += 32) {
        data = _mm256_loadu_si256((const __m256i *)(image + i));
        sum_vec = _mm256_add_epi64(sum_vec, _mm256_sad_epu8(data, zero));
    }

    _mm256_storeu_si256((__m256i *)partial, sum_vec);
    sum = (int64_t)(partial[0] + partial[1] + partial[2] + partial[3]);

    for (i = simd_count; i < total; i++) {
        sum += image[i];
    }

    return sum;
#elif LIERRE_USE_SIMD && defined(LIERRE_SIMD_NEON)
    uint32x4_t sum_vec;
    uint8x16_t data;
    uint16x8_t sum16;
    int64_t sum;
    size_t i, simd_count;

    sum_vec = vdupq_n_u32(0);

    simd_count = total & ~15ULL;

    for (i = 0; i < simd_count; i += 16) {
        data = vld1q_u8(image + i);
        sum16 = vpaddlq_u8(data);
        sum_vec = vpadalq_u16(sum_vec, sum16);
    }

    sum = (int64_t)(vgetq_lane_u32(sum_vec, 0) + vgetq_lane_u32(sum_vec, 1) + vgetq_lane_u32(sum_vec, 2) +
                    vgetq_lane_u32(sum_vec, 3));

    for (i = simd_count; i < total; i++) {
        sum += image[i];
    }

    return sum;
#elif LIERRE_USE_SIMD && defined(LIERRE_SIMD_WASM)
    v128_t sum_vec = wasm_i64x2_splat(0), data, lo, hi, sum16, lo32, hi32, sum32, lo64, hi64;
    int64_t sum;
    uint64_t partial[2];
    size_t i, simd_count = total & ~15ULL;

    for (i = 0; i < simd_count; i += 16) {
        data = wasm_v128_load(image + i);
        lo = wasm_u16x8_extend_low_u8x16(data);
        hi = wasm_u16x8_extend_high_u8x16(data);
        sum16 = wasm_i16x8_add(lo, hi);
        lo32 = wasm_u32x4_extend_low_u16x8(sum16);
        hi32 = wasm_u32x4_extend_high_u16x8(sum16);
        sum32 = wasm_i32x4_add(lo32, hi32);
        lo64 = wasm_u64x2_extend_low_u32x4(sum32);
        hi64 = wasm_u64x2_extend_high_u32x4(sum32);
        sum_vec = wasm_i64x2_add(sum_vec, wasm_i64x2_add(lo64, hi64));
    }

    wasm_v128_store(partial, sum_vec);
    sum = (int64_t)(partial[0] + partial[1]);

    for (i = simd_count; i < total; i++) {
        sum += image[i];
    }

    return sum;
#else
    int64_t sum;
    size_t i;

    sum = 0;
    for (i = 0; i < total; i++) {
        sum += image[i];
    }

    return sum;
#endif
}

static inline void *minimize_thread(void *arg)
{
    lierre_image_mt_minimize_ctx_t *ctx;
    size_t y, src_y;

    ctx = (lierre_image_mt_minimize_ctx_t *)arg;

    for (y = ctx->start_row; y < ctx->end_row; y++) {
        src_y = y * 2;
        lierre_minimize_row(ctx->src + src_y * ctx->src_width, ctx->src + (src_y + 1) * ctx->src_width,
                            ctx->dst + y * ctx->dst_width, ctx->dst_width);
    }

    return NULL;
}

static inline uint8_t *apply_minimize_once(const uint8_t *image, size_t width, size_t height, size_t *out_width,
                                           size_t *out_height)
{
    uint8_t *result;
    size_t new_width, new_height, y, src_y;

    new_width = width >> 1;
    new_height = height >> 1;

    if (new_width < 21 || new_height < 21) {
        *out_width = width;
        *out_height = height;
        return NULL;
    }

    result = lmalloc(new_width * new_height);
    if (!result) {
        return NULL;
    }

    for (y = 0; y < new_height; y++) {
        src_y = y * 2;
        lierre_minimize_row(image + src_y * width, image + (src_y + 1) * width, result + y * new_width, new_width);
    }

    *out_width = new_width;
    *out_height = new_height;

    return result;
}

static inline uint8_t *apply_minimize_once_mt(const uint8_t *image, size_t width, size_t height, size_t *out_width,
                                              size_t *out_height, uint32_t num_threads)
{
    lierre_thread_t *threads;
    lierre_image_mt_minimize_ctx_t *contexts;
    uint32_t i;
    uint8_t *result;
    size_t new_width, new_height, rows_per_thread;

    new_width = width >> 1;
    new_height = height >> 1;

    if (new_width < 21 || new_height < 21) {
        *out_width = width;
        *out_height = height;
        return NULL;
    }

    if (num_threads > LIERRE_IMAGE_MT_MAX_THREADS) {
        num_threads = LIERRE_IMAGE_MT_MAX_THREADS;
    }
    if (num_threads > new_height) {
        num_threads = (uint32_t)new_height;
    }
    if (num_threads < 1) {
        num_threads = 1;
    }

    result = lmalloc(new_width * new_height);
    if (!result) {
        return NULL;
    }

    threads = lmalloc(sizeof(lierre_thread_t) * num_threads);
    contexts = lmalloc(sizeof(lierre_image_mt_minimize_ctx_t) * num_threads);
    if (!threads || !contexts) {
        lfree(result);
        lfree(threads);
        lfree(contexts);
        return NULL;
    }

    rows_per_thread = new_height / num_threads;

    for (i = 0; i < num_threads; i++) {
        contexts[i].src = image;
        contexts[i].dst = result;
        contexts[i].src_width = width;
        contexts[i].dst_width = new_width;
        contexts[i].start_row = i * rows_per_thread;
        contexts[i].end_row = (i == num_threads - 1) ? new_height : (i + 1) * rows_per_thread;
        lierre_thread_create(&threads[i], minimize_thread, &contexts[i]);
    }

    for (i = 0; i < num_threads; i++) {
        lierre_thread_join(threads[i], NULL);
    }

    lfree(threads);
    lfree(contexts);

    *out_width = new_width;
    *out_height = new_height;

    return result;
}

static inline uint8_t *lierre_image_minimize(const uint8_t *image, size_t width, size_t height, size_t *out_width,
                                             size_t *out_height, bool use_mt, uint32_t num_threads)
{
    uint32_t iter;
    uint8_t *current, *next;
    size_t cur_width, cur_height, new_width, new_height;

    current = lmalloc(width * height);
    if (!current) {
        return NULL;
    }

    lmemcpy(current, image, width * height);
    cur_width = width;
    cur_height = height;

    for (iter = 0; iter < LIERRE_IMAGE_MINIMIZE_MAX_ITERATIONS; iter++) {
        if (use_mt) {
            next = apply_minimize_once_mt(current, cur_width, cur_height, &new_width, &new_height, num_threads);
        } else {
            next = apply_minimize_once(current, cur_width, cur_height, &new_width, &new_height);
        }

        if (!next) {
            break;
        }

        lfree(current);
        current = next;
        cur_width = new_width;
        cur_height = new_height;
    }

    *out_width = cur_width;
    *out_height = cur_height;

    return current;
}

static inline void *denoise_filter_thread(void *arg)
{
    lierre_image_mt_filter_ctx_t *ctx;
    int32_t sum;
    size_t x, y, i, j, idx;

    ctx = (lierre_image_mt_filter_ctx_t *)arg;

    for (y = ctx->start_row; y < ctx->end_row; y++) {
        if (y == 0 || y >= ctx->height - 1) {
            continue;
        }
        for (x = 1; x < ctx->width - 1; x++) {
            sum = 0;
            for (j = y - 1; j <= y + 1; j++) {
                for (i = x - 1; i <= x + 1; i++) {
                    sum += ctx->temp[j * ctx->width + i];
                }
            }
            idx = y * ctx->width + x;
            ctx->image[idx] = (uint8_t)(sum / LIERRE_FILTER_KERNEL_ELEMS);
        }
    }

    return NULL;
}

static inline void *sharpening_filter_thread(void *arg)
{
    lierre_image_mt_filter_ctx_t *ctx;
    int32_t val;
    size_t x, y, idx;

    ctx = (lierre_image_mt_filter_ctx_t *)arg;

    for (y = ctx->start_row; y < ctx->end_row; y++) {
        if (y == 0 || y >= ctx->height - 1) {
            continue;
        }
        for (x = 1; x < ctx->width - 1; x++) {
            idx = y * ctx->width + x;
            val = LIERRE_SHARPEN_CENTER_COEF * (int32_t)ctx->temp[idx] - (int32_t)ctx->temp[(y - 1) * ctx->width + x] -
                  (int32_t)ctx->temp[(y + 1) * ctx->width + x] - (int32_t)ctx->temp[y * ctx->width + (x - 1)] -
                  (int32_t)ctx->temp[y * ctx->width + (x + 1)];
            if (val < LIERRE_PIXEL_VALUE_MIN) {
                val = LIERRE_PIXEL_VALUE_MIN;
            }
            if (val > LIERRE_PIXEL_VALUE_MAX) {
                val = LIERRE_PIXEL_VALUE_MAX;
            }
            ctx->image[idx] = (uint8_t)val;
        }
    }

    return NULL;
}

extern void lierre_image_brightness_normalize(uint8_t *image, size_t width, size_t height)
{
    size_t i, total;
    int32_t range;
    uint8_t min_val, max_val;

    total = width * height;
    if (total == 0) {
        return;
    }

    lierre_find_minmax(image, total, &min_val, &max_val);

    range = (int32_t)max_val - (int32_t)min_val;
    if (range <= 0) {
        return;
    }

    for (i = 0; i < total; i++) {
        image[i] = (uint8_t)(((int32_t)(image[i] - min_val) * LIERRE_PIXEL_VALUE_MAX) / range);
    }
}

extern void lierre_image_contrast_normalize(uint8_t *image, size_t width, size_t height)
{
    size_t i, total;
    int64_t sum64;
    int32_t mean, val, new_val, factor;

    total = width * height;
    if (total == 0) {
        return;
    }

    sum64 = lierre_sum(image, total);

    mean = (int32_t)(sum64 / (int64_t)total);
    factor = LIERRE_CONTRAST_FACTOR;

    for (i = 0; i < total; i++) {
        val = (int32_t)image[i];
        new_val = mean + ((val - mean) * factor) / LIERRE_CONTRAST_DIVISOR;

        if (new_val < LIERRE_PIXEL_VALUE_MIN) {
            new_val = LIERRE_PIXEL_VALUE_MIN;
        }

        if (new_val > LIERRE_PIXEL_VALUE_MAX) {
            new_val = LIERRE_PIXEL_VALUE_MAX;
        }

        image[i] = (uint8_t)new_val;
    }
}

extern void lierre_image_denoise_mt(uint8_t *image, size_t width, size_t height, uint32_t num_threads)
{
    lierre_thread_t *threads;
    lierre_image_mt_filter_ctx_t *contexts;
    uint32_t i;
    uint8_t *temp;
    size_t rows_per_thread;

    if (width < 3 || height < 3) {
        return;
    }

    if (num_threads > LIERRE_IMAGE_MT_MAX_THREADS) {
        num_threads = LIERRE_IMAGE_MT_MAX_THREADS;
    }
    if (num_threads > height) {
        num_threads = (uint32_t)height;
    }
    if (num_threads < 1) {
        num_threads = 1;
    }

    temp = lmalloc(width * height);
    if (!temp) {
        return;
    }

    threads = lmalloc(sizeof(lierre_thread_t) * num_threads);
    contexts = lmalloc(sizeof(lierre_image_mt_filter_ctx_t) * num_threads);
    if (!threads || !contexts) {
        lfree(temp);
        lfree(threads);
        lfree(contexts);
        return;
    }

    lmemcpy(temp, image, width * height);

    rows_per_thread = height / num_threads;

    for (i = 0; i < num_threads; i++) {
        contexts[i].image = image;
        contexts[i].temp = temp;
        contexts[i].width = width;
        contexts[i].height = height;
        contexts[i].start_row = i * rows_per_thread;
        contexts[i].end_row = (i == num_threads - 1) ? height : (i + 1) * rows_per_thread;
        lierre_thread_create(&threads[i], denoise_filter_thread, &contexts[i]);
    }

    for (i = 0; i < num_threads; i++) {
        lierre_thread_join(threads[i], NULL);
    }

    lfree(threads);
    lfree(contexts);
    lfree(temp);
}

extern void lierre_image_denoise(uint8_t *image, size_t width, size_t height)
{
    int32_t sum;
    uint8_t *temp;
    size_t x, y, i, j, idx;

    if (width < 3 || height < 3) {
        return;
    }

    temp = lmalloc(width * height);
    if (!temp) {
        return;
    }

    lmemcpy(temp, image, width * height);

    for (y = 1; y < height - 1; y++) {
        for (x = 1; x < width - 1; x++) {
            sum = 0;
            for (j = y - 1; j <= y + 1; j++) {
                for (i = x - 1; i <= x + 1; i++) {
                    sum += temp[j * width + i];
                }
            }
            idx = y * width + x;
            image[idx] = (uint8_t)(sum / LIERRE_FILTER_KERNEL_ELEMS);
        }
    }

    lfree(temp);
}

extern void lierre_image_sharpen_mt(uint8_t *image, size_t width, size_t height, uint32_t num_threads)
{
    lierre_thread_t *threads;
    lierre_image_mt_filter_ctx_t *contexts;
    uint32_t i;
    uint8_t *temp;
    size_t rows_per_thread;

    if (width < 3 || height < 3) {
        return;
    }

    if (num_threads > LIERRE_IMAGE_MT_MAX_THREADS) {
        num_threads = LIERRE_IMAGE_MT_MAX_THREADS;
    }
    if (num_threads > height) {
        num_threads = (uint32_t)height;
    }
    if (num_threads < 1) {
        num_threads = 1;
    }

    temp = lmalloc(width * height);
    if (!temp) {
        return;
    }

    threads = lmalloc(sizeof(lierre_thread_t) * num_threads);
    contexts = lmalloc(sizeof(lierre_image_mt_filter_ctx_t) * num_threads);
    if (!threads || !contexts) {
        lfree(temp);
        lfree(threads);
        lfree(contexts);
        return;
    }

    lmemcpy(temp, image, width * height);

    rows_per_thread = height / num_threads;

    for (i = 0; i < num_threads; i++) {
        contexts[i].image = image;
        contexts[i].temp = temp;
        contexts[i].width = width;
        contexts[i].height = height;
        contexts[i].start_row = i * rows_per_thread;
        contexts[i].end_row = (i == num_threads - 1) ? height : (i + 1) * rows_per_thread;
        lierre_thread_create(&threads[i], sharpening_filter_thread, &contexts[i]);
    }

    for (i = 0; i < num_threads; i++) {
        lierre_thread_join(threads[i], NULL);
    }

    lfree(threads);
    lfree(contexts);
    lfree(temp);
}

extern void lierre_image_sharpen(uint8_t *image, size_t width, size_t height)
{
    int32_t val;
    uint8_t *temp;
    size_t x, y, idx;

    if (width < 3 || height < 3) {
        return;
    }

    temp = lmalloc(width * height);
    if (!temp) {
        return;
    }

    lmemcpy(temp, image, width * height);

    for (y = 1; y < height - 1; y++) {
        for (x = 1; x < width - 1; x++) {
            idx = y * width + x;
            val = LIERRE_SHARPEN_CENTER_COEF * (int32_t)temp[idx] - (int32_t)temp[(y - 1) * width + x] -
                  (int32_t)temp[(y + 1) * width + x] - (int32_t)temp[y * width + (x - 1)] -
                  (int32_t)temp[y * width + (x + 1)];
            if (val < LIERRE_PIXEL_VALUE_MIN) {
                val = LIERRE_PIXEL_VALUE_MIN;
            }
            if (val > LIERRE_PIXEL_VALUE_MAX) {
                val = LIERRE_PIXEL_VALUE_MAX;
            }
            image[idx] = (uint8_t)val;
        }
    }

    lfree(temp);
}

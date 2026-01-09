/*
 * liblierre - decoder.c
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "../internal/decoder.h"

#if LIERRE_USE_SIMD
#include "../internal/simd.h"
#endif

#define LIERRE_HISTOGRAM_SIZE 256

static inline uint8_t compute_otsu_threshold(const lierre_decoder_t *decoder)
{
    const uint8_t *image_ptr;
    uint64_t total_sum_int;
    uint32_t histogram[LIERRE_HISTOGRAM_SIZE] = {0}, total_pixels, foreground_count, background_count,
             optimal_threshold, i;
    uint8_t pixel_value;
    size_t remaining;
    double total_sum, foreground_sum, foreground_mean, background_mean, between_class_variance, max_variance;

    total_pixels = (uint32_t)(decoder->w * decoder->h);

    image_ptr = decoder->image;
    remaining = (size_t)total_pixels;
    total_sum_int = 0;
    while (remaining--) {
        pixel_value = *image_ptr++;
        histogram[pixel_value]++;
        total_sum_int += pixel_value;
    }
    total_sum = (double)total_sum_int;

    foreground_sum = 0.0;
    foreground_count = 0;
    max_variance = 0.0;
    optimal_threshold = 0;

    for (i = 0; i < LIERRE_HISTOGRAM_SIZE; i++) {
        foreground_count += histogram[i];
        if (foreground_count == 0) {
            continue;
        }

        background_count = total_pixels - foreground_count;
        if (background_count == 0) {
            break;
        }

        foreground_sum += (double)i * histogram[i];
        foreground_mean = foreground_sum / foreground_count;
        background_mean = (total_sum - foreground_sum) / background_count;
        between_class_variance = (foreground_mean - background_mean) * (foreground_mean - background_mean) *
                                 foreground_count * background_count;

        if (between_class_variance >= max_variance) {
            optimal_threshold = i;
            max_variance = between_class_variance;
        }
    }

    return (uint8_t)optimal_threshold;
}

static inline void binarize_image(lierre_decoder_t *decoder, uint8_t threshold)
{
    int32_t x, y;
    uint8_t *row, pixel_value;
    size_t total;

    total = (size_t)decoder->w * (size_t)decoder->h;

#if LIERRE_USE_SIMD && defined(LIERRE_SIMD_AVX2)
    {
        __m256i thresh_vec, data;
        __m256i black_lo, white_lo, result_lo;
        __m128i data128;
        size_t i, simd_count;

        simd_count = total & ~15ULL;

        for (i = 0; i < simd_count; i += 16) {
            data128 = _mm_loadu_si128((const __m128i *)(decoder->image + i));
            data = _mm256_cvtepu8_epi16(data128);

            thresh_vec = _mm256_set1_epi16((int16_t)threshold);
            black_lo = _mm256_set1_epi16((int16_t)LIERRE_PIXEL_BLACK);
            white_lo = _mm256_set1_epi16((int16_t)LIERRE_PIXEL_WHITE);

            result_lo = _mm256_blendv_epi8(white_lo, black_lo, _mm256_cmpgt_epi16(thresh_vec, data));

            _mm256_storeu_si256((__m256i *)(decoder->pixels + i), result_lo);
        }

        for (i = simd_count; i < total; i++) {
            decoder->pixels[i] = (decoder->image[i] < threshold) ? LIERRE_PIXEL_BLACK : LIERRE_PIXEL_WHITE;
        }
    }
#elif LIERRE_USE_SIMD && defined(LIERRE_SIMD_NEON)
    {
        uint8x8_t data8, thresh_vec8;
        uint16x8_t data16, mask16, black_vec, white_vec, result;
        size_t i, simd_count;

        black_vec = vdupq_n_u16(LIERRE_PIXEL_BLACK);
        white_vec = vdupq_n_u16(LIERRE_PIXEL_WHITE);

        simd_count = total & ~7ULL;

        for (i = 0; i < simd_count; i += 8) {
            data8 = vld1_u8(decoder->image + i);
            data16 = vmovl_u8(data8);
            mask16 = vcltq_u16(data16, vdupq_n_u16(threshold));
            result = vbslq_u16(mask16, black_vec, white_vec);
            vst1q_u16(decoder->pixels + i, result);
        }

        for (i = simd_count; i < total; i++) {
            decoder->pixels[i] = (decoder->image[i] < threshold) ? LIERRE_PIXEL_BLACK : LIERRE_PIXEL_WHITE;
        }
    }
#elif LIERRE_USE_SIMD && defined(LIERRE_SIMD_WASM)
    {
        v128_t data8, data16_lo, data16_hi, thresh_vec, black_vec, white_vec, mask_lo, mask_hi, result_lo, result_hi;
        size_t i, simd_count;

        thresh_vec = wasm_u16x8_splat(threshold);
        black_vec = wasm_u16x8_splat(LIERRE_PIXEL_BLACK);
        white_vec = wasm_u16x8_splat(LIERRE_PIXEL_WHITE);

        simd_count = total & ~15ULL;

        for (i = 0; i < simd_count; i += 16) {
            data8 = wasm_v128_load(decoder->image + i);
            data16_lo = wasm_u16x8_extend_low_u8x16(data8);
            data16_hi = wasm_u16x8_extend_high_u8x16(data8);

            mask_lo = wasm_u16x8_lt(data16_lo, thresh_vec);
            mask_hi = wasm_u16x8_lt(data16_hi, thresh_vec);

            result_lo = wasm_v128_bitselect(black_vec, white_vec, mask_lo);
            result_hi = wasm_v128_bitselect(black_vec, white_vec, mask_hi);

            wasm_v128_store(decoder->pixels + i, result_lo);
            wasm_v128_store(decoder->pixels + i + 8, result_hi);
        }

        for (i = simd_count; i < total; i++) {
            decoder->pixels[i] = (decoder->image[i] < threshold) ? LIERRE_PIXEL_BLACK : LIERRE_PIXEL_WHITE;
        }
    }
#else
    for (y = 0; y < decoder->h; y++) {
        row = decoder->image + y * decoder->w;
        for (x = 0; x < decoder->w; x++) {
            pixel_value = row[x];
            decoder->pixels[y * decoder->w + x] = (pixel_value < threshold) ? LIERRE_PIXEL_BLACK : LIERRE_PIXEL_WHITE;
        }
    }

    (void)total;
#endif
}

static inline int32_t decoder_resize(lierre_decoder_t *decoder, int32_t width, int32_t height)
{
    lierre_pixel_t *pixels;
    lierre_flood_fill_vars_t *vars;
    uint8_t *image;
    size_t num_vars, vars_byte_size;

    image = NULL;
    pixels = NULL;
    vars = NULL;

    if (width < 0 || height < 0) {
        return -1;
    }

    image = lcalloc((size_t)width, (size_t)height);
    if (!image) {
        return -1;
    }

    pixels = lcalloc((size_t)width * (size_t)height, sizeof(lierre_pixel_t));
    if (!pixels) {
        lfree(image);

        return -1;
    }

    num_vars = (size_t)height * 2 / 3;
    if (num_vars == 0) {
        num_vars = 1;
    }

    vars_byte_size = sizeof(lierre_flood_fill_vars_t) * num_vars;
    vars = lmalloc(vars_byte_size);
    if (!vars) {
        lfree(image);
        lfree(pixels);

        return -1;
    }

    decoder->w = width;
    decoder->h = height;

    if (decoder->image) {
        lfree(decoder->image);
    }

    decoder->image = image;

    if (decoder->pixels) {
        lfree(decoder->pixels);
    }

    decoder->pixels = pixels;

    if (decoder->flood_fill_vars) {
        lfree(decoder->flood_fill_vars);
    }

    decoder->flood_fill_vars = vars;
    decoder->num_flood_fill_vars = num_vars;

    return 0;
}

static inline void *decode_qr_thread(void *arg)
{
    decode_thread_context_t *ctx = (decode_thread_context_t *)arg;

    extract_qr_code(ctx->decoder, ctx->grid_index, &ctx->code);
    ctx->err = decode_qr(&ctx->code, &ctx->data);

    return NULL;
}

extern lierre_decoder_t *lierre_decoder_create(void)
{
    lierre_decoder_t *decoder;

    decoder = lcalloc(1, sizeof(lierre_decoder_t));
    if (!decoder) {
        return NULL;
    }

    return decoder;
}

extern void lierre_decoder_destroy(lierre_decoder_t *decoder)
{
    if (!decoder) {
        return;
    }

    if (decoder->image) {
        lfree(decoder->image);
    }

    if (decoder->pixels) {
        lfree(decoder->pixels);
    }

    if (decoder->flood_fill_vars) {
        lfree(decoder->flood_fill_vars);
    }

    lfree(decoder);
}

extern lierre_error_t lierre_decoder_process(lierre_decoder_t *decoder, const uint8_t *gray_image, int32_t width,
                                             int32_t height, lierre_decoder_result_t *result)
{
    lierre_qr_code_t code;
    lierre_qr_data_t data;
    lierre_error_t err;
    uint8_t threshold;
    int32_t row, i;

    if (!decoder || !gray_image || !result || width <= 0 || height <= 0) {
        return LIERRE_ERROR_INVALID_PARAMS;
    }

    if (decoder_resize(decoder, width, height) < 0) {
        return LIERRE_ERROR_DATA_OVERFLOW;
    }

    lmemcpy(decoder->image, gray_image, (size_t)(width * height));

    decoder->num_regions = LIERRE_PIXEL_REGION;
    decoder->num_capstones = 0;
    decoder->num_grids = 0;

    threshold = compute_otsu_threshold(decoder);
    decoder->threshold = threshold;
    binarize_image(decoder, threshold);

    for (row = 0; row < height; row++) {
        scan_finder_patterns(decoder, (uint32_t)row);
    }

    for (i = 0; i < decoder->num_capstones; i++) {
        find_capstone_groups(decoder, i);
    }

    result->count = 0;

    for (i = 0; i < decoder->num_grids && result->count < LIERRE_DECODER_MAX_GRIDS; i++) {
        extract_qr_code(decoder, i, &code);
        err = decode_qr(&code, &data);

        if (err == LIERRE_ERROR_SUCCESS) {
            result->codes[result->count].corners[0] = code.corners[0];
            result->codes[result->count].corners[1] = code.corners[1];
            result->codes[result->count].corners[2] = code.corners[2];
            result->codes[result->count].corners[3] = code.corners[3];
            lmemcpy(result->codes[result->count].payload, data.payload, (size_t)data.payload_len);
            result->codes[result->count].payload_len = data.payload_len;
            result->count++;
        }
    }

    return LIERRE_ERROR_SUCCESS;
}

extern lierre_error_t lierre_decoder_process_mt(lierre_decoder_t *decoder, const uint8_t *gray_image, int32_t width,
                                                int32_t height, lierre_decoder_result_t *result, uint32_t num_threads)
{
    lierre_thread_t *threads;
    decode_thread_context_t *contexts;
    uint32_t active_threads, t;
    uint8_t threshold;
    int32_t row, i;

    if (!decoder || !gray_image || !result || width <= 0 || height <= 0) {
        return LIERRE_ERROR_INVALID_PARAMS;
    }

    if (num_threads < 1) {
        num_threads = 1;
    }
    if (num_threads > LIERRE_DECODER_MT_MAX_THREADS) {
        num_threads = LIERRE_DECODER_MT_MAX_THREADS;
    }

    if (decoder_resize(decoder, width, height) < 0) {
        return LIERRE_ERROR_DATA_OVERFLOW;
    }

    lmemcpy(decoder->image, gray_image, (size_t)(width * height));

    decoder->num_regions = LIERRE_PIXEL_REGION;
    decoder->num_capstones = 0;
    decoder->num_grids = 0;

    threshold = compute_otsu_threshold(decoder);
    decoder->threshold = threshold;
    binarize_image(decoder, threshold);

    for (row = 0; row < height; row++) {
        scan_finder_patterns(decoder, (uint32_t)row);
    }

    for (i = 0; i < decoder->num_capstones; i++) {
        find_capstone_groups(decoder, i);
    }

    result->count = 0;

    if (decoder->num_grids == 0) {
        return LIERRE_ERROR_SUCCESS;
    }

    threads = lmalloc(sizeof(lierre_thread_t) * num_threads);
    contexts = lmalloc(sizeof(decode_thread_context_t) * decoder->num_grids);
    if (!threads || !contexts) {
        lfree(threads);
        lfree(contexts);
        return LIERRE_ERROR_DATA_OVERFLOW;
    }

    for (i = 0; i < decoder->num_grids; i++) {
        contexts[i].decoder = decoder;
        contexts[i].grid_index = i;
        contexts[i].err = LIERRE_ERROR_INVALID_PARAMS;
    }

    i = 0;
    while (i < decoder->num_grids) {
        active_threads = 0;

        for (t = 0; t < num_threads && i < decoder->num_grids; t++, i++) {
            lierre_thread_create(&threads[t], decode_qr_thread, &contexts[i]);
            active_threads++;
        }

        for (t = 0; t < active_threads; t++) {
            lierre_thread_join(threads[t], NULL);
        }
    }

    for (i = 0; i < decoder->num_grids && result->count < LIERRE_DECODER_MAX_GRIDS; i++) {
        if (contexts[i].err == LIERRE_ERROR_SUCCESS) {
            result->codes[result->count].corners[0] = contexts[i].code.corners[0];
            result->codes[result->count].corners[1] = contexts[i].code.corners[1];
            result->codes[result->count].corners[2] = contexts[i].code.corners[2];
            result->codes[result->count].corners[3] = contexts[i].code.corners[3];
            lmemcpy(result->codes[result->count].payload, contexts[i].data.payload,
                    (size_t)contexts[i].data.payload_len);
            result->codes[result->count].payload_len = contexts[i].data.payload_len;
            result->count++;
        }
    }

    lfree(threads);
    lfree(contexts);

    return LIERRE_ERROR_SUCCESS;
}

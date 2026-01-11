/*
 * liblierre - reader.c
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <string.h>

#include <lierre.h>
#include <lierre/portable.h>
#include <lierre/reader.h>

#include "../internal/decoder.h"
#include "../internal/image.h"
#include "../internal/memory.h"

#if LIERRE_USE_SIMD
#include "../internal/simd.h"
#endif

#include "../internal/structs.h"

#define LIERRE_IMAGE_MINIMIZE_MAX_SCALE 16

#define LIERRE_GRAY_WEIGHT_R       77
#define LIERRE_GRAY_WEIGHT_G       150
#define LIERRE_GRAY_WEIGHT_B       29
#define LIERRE_GRAY_SHIFT          8
#define LIERRE_MIN_QR_SIZE         21
#define LIERRE_PIXEL_VALUE_DEFAULT 255

static inline void lierre_rgb_to_gray(const uint8_t *src, uint8_t *dst, size_t pixel_count)
{
#if LIERRE_USE_SIMD && defined(LIERRE_SIMD_AVX2)
    __m256i r_weight, g_weight, b_weight;
    __m128i r0, g0, b0, r1, g1, gray8;
    __m256i r16, g16, b16, sum;
    size_t i, simd_count;
    const uint8_t *p;

    r_weight = _mm256_set1_epi16(LIERRE_GRAY_WEIGHT_R);
    g_weight = _mm256_set1_epi16(LIERRE_GRAY_WEIGHT_G);
    b_weight = _mm256_set1_epi16(LIERRE_GRAY_WEIGHT_B);

    simd_count = pixel_count & ~15ULL;

    for (i = 0; i < simd_count; i += 16) {
        p = src + i * 3;

        r0 = _mm_setr_epi8(p[0], p[3], p[6], p[9], p[12], p[15], p[18], p[21], p[24], p[27], p[30], p[33], p[36], p[39],
                           p[42], p[45]);
        g0 = _mm_setr_epi8(p[1], p[4], p[7], p[10], p[13], p[16], p[19], p[22], p[25], p[28], p[31], p[34], p[37],
                           p[40], p[43], p[46]);
        b0 = _mm_setr_epi8(p[2], p[5], p[8], p[11], p[14], p[17], p[20], p[23], p[26], p[29], p[32], p[35], p[38],
                           p[41], p[44], p[47]);

        r16 = _mm256_cvtepu8_epi16(r0);
        g16 = _mm256_cvtepu8_epi16(g0);
        b16 = _mm256_cvtepu8_epi16(b0);

        sum = _mm256_add_epi16(_mm256_mullo_epi16(r16, r_weight),
                               _mm256_add_epi16(_mm256_mullo_epi16(g16, g_weight), _mm256_mullo_epi16(b16, b_weight)));
        sum = _mm256_srli_epi16(sum, LIERRE_GRAY_SHIFT);

        r1 = _mm256_castsi256_si128(sum);
        g1 = _mm256_extracti128_si256(sum, 1);
        gray8 = _mm_packus_epi16(r1, g1);

        _mm_storeu_si128((__m128i *)(dst + i), gray8);
    }

    for (i = simd_count; i < pixel_count; i++) {
        p = src + i * 3;
        dst[i] = (uint8_t)((p[0] * LIERRE_GRAY_WEIGHT_R + p[1] * LIERRE_GRAY_WEIGHT_G + p[2] * LIERRE_GRAY_WEIGHT_B) >>
                           LIERRE_GRAY_SHIFT);
    }
#elif LIERRE_USE_SIMD && defined(LIERRE_SIMD_NEON)
    uint8x16x3_t rgb;
    uint16x8_t sum_lo, sum_hi;
    uint8x8_t gray_lo, gray_hi;
    uint8x16_t gray;
    size_t i, simd_count;

    simd_count = pixel_count & ~15ULL;

    for (i = 0; i < simd_count; i += 16) {
        rgb = vld3q_u8(src + i * 3);

        sum_lo = vmull_u8(vget_low_u8(rgb.val[0]), vdup_n_u8(LIERRE_GRAY_WEIGHT_R));
        sum_lo = vmlal_u8(sum_lo, vget_low_u8(rgb.val[1]), vdup_n_u8(LIERRE_GRAY_WEIGHT_G));
        sum_lo = vmlal_u8(sum_lo, vget_low_u8(rgb.val[2]), vdup_n_u8(LIERRE_GRAY_WEIGHT_B));

        sum_hi = vmull_u8(vget_high_u8(rgb.val[0]), vdup_n_u8(LIERRE_GRAY_WEIGHT_R));
        sum_hi = vmlal_u8(sum_hi, vget_high_u8(rgb.val[1]), vdup_n_u8(LIERRE_GRAY_WEIGHT_G));
        sum_hi = vmlal_u8(sum_hi, vget_high_u8(rgb.val[2]), vdup_n_u8(LIERRE_GRAY_WEIGHT_B));

        gray_lo = vshrn_n_u16(sum_lo, LIERRE_GRAY_SHIFT);
        gray_hi = vshrn_n_u16(sum_hi, LIERRE_GRAY_SHIFT);
        gray = vcombine_u8(gray_lo, gray_hi);

        vst1q_u8(dst + i, gray);
    }

    for (i = simd_count; i < pixel_count; i++) {
        const uint8_t *p = src + i * 3;
        dst[i] = (uint8_t)((p[0] * LIERRE_GRAY_WEIGHT_R + p[1] * LIERRE_GRAY_WEIGHT_G + p[2] * LIERRE_GRAY_WEIGHT_B) >>
                           LIERRE_GRAY_SHIFT);
    }
#elif LIERRE_USE_SIMD && defined(LIERRE_SIMD_WASM)
    v128_t r_weight, g_weight, b_weight;
    v128_t r0, g0, b0, r_lo, r_hi, g_lo, g_hi, b_lo, b_hi;
    v128_t sum_lo, sum_hi, gray;
    size_t i, simd_count;
    const uint8_t *p;
    uint8_t r_buf[16], g_buf[16], b_buf[16];
    size_t j;

    r_weight = wasm_i16x8_splat(LIERRE_GRAY_WEIGHT_R);
    g_weight = wasm_i16x8_splat(LIERRE_GRAY_WEIGHT_G);
    b_weight = wasm_i16x8_splat(LIERRE_GRAY_WEIGHT_B);

    simd_count = pixel_count & ~15ULL;

    for (i = 0; i < simd_count; i += 16) {
        p = src + i * 3;

        for (j = 0; j < 16; j++) {
            r_buf[j] = p[j * 3];
            g_buf[j] = p[j * 3 + 1];
            b_buf[j] = p[j * 3 + 2];
        }
        r0 = wasm_v128_load(r_buf);
        g0 = wasm_v128_load(g_buf);
        b0 = wasm_v128_load(b_buf);

        r_lo = wasm_u16x8_extend_low_u8x16(r0);
        r_hi = wasm_u16x8_extend_high_u8x16(r0);
        g_lo = wasm_u16x8_extend_low_u8x16(g0);
        g_hi = wasm_u16x8_extend_high_u8x16(g0);
        b_lo = wasm_u16x8_extend_low_u8x16(b0);
        b_hi = wasm_u16x8_extend_high_u8x16(b0);

        sum_lo = wasm_i16x8_add(wasm_i16x8_mul(r_lo, r_weight),
                                wasm_i16x8_add(wasm_i16x8_mul(g_lo, g_weight), wasm_i16x8_mul(b_lo, b_weight)));
        sum_hi = wasm_i16x8_add(wasm_i16x8_mul(r_hi, r_weight),
                                wasm_i16x8_add(wasm_i16x8_mul(g_hi, g_weight), wasm_i16x8_mul(b_hi, b_weight)));

        sum_lo = wasm_u16x8_shr(sum_lo, LIERRE_GRAY_SHIFT);
        sum_hi = wasm_u16x8_shr(sum_hi, LIERRE_GRAY_SHIFT);

        gray = wasm_u8x16_narrow_i16x8(sum_lo, sum_hi);

        wasm_v128_store(dst + i, gray);
    }

    for (i = simd_count; i < pixel_count; i++) {
        p = src + i * 3;
        dst[i] = (uint8_t)((p[0] * LIERRE_GRAY_WEIGHT_R + p[1] * LIERRE_GRAY_WEIGHT_G + p[2] * LIERRE_GRAY_WEIGHT_B) >>
                           LIERRE_GRAY_SHIFT);
    }
#else
    size_t i;
    const uint8_t *p;

    for (i = 0; i < pixel_count; i++) {
        p = src + i * 3;
        dst[i] = (uint8_t)((p[0] * LIERRE_GRAY_WEIGHT_R + p[1] * LIERRE_GRAY_WEIGHT_G + p[2] * LIERRE_GRAY_WEIGHT_B) >>
                           LIERRE_GRAY_SHIFT);
    }
#endif
}

extern lierre_error_t lierre_reader_param_init(lierre_reader_param_t *param)
{
    if (!param) {
        return LIERRE_ERROR_INVALID_PARAMS;
    }

    param->strategy_flags = LIERRE_READER_STRATEGY_NONE;
    param->rect = NULL;

    return LIERRE_ERROR_SUCCESS;
}

extern void lierre_reader_param_set_flag(lierre_reader_param_t *param, lierre_reader_strategy_flag_t flag)
{
    if (!param) {
        return;
    }

    param->strategy_flags |= flag;
}

extern void lierre_reader_param_set_rect(lierre_reader_param_t *param, const lierre_rect_t *rect)
{
    if (!param) {
        return;
    }

    param->rect = rect;
}

extern lierre_reader_t *lierre_reader_create(const lierre_reader_param_t *param)
{
    lierre_reader_t *reader;

    if (!param) {
        return NULL;
    }

    reader = lmalloc(sizeof(lierre_reader_t));
    if (!reader) {
        return NULL;
    }

    reader->data = NULL;
    reader->param = lmalloc(sizeof(lierre_reader_param_t));
    if (!reader->param) {
        lfree(reader);
        return NULL;
    }

    lmemcpy(reader->param, param, sizeof(lierre_reader_param_t));

    return reader;
}

extern void lierre_reader_destroy(lierre_reader_t *reader)
{
    if (!reader) {
        return;
    }

    if (reader->param) {
        lfree(reader->param);
    }

    lfree(reader);
}

extern void lierre_reader_set_data(lierre_reader_t *reader, lierre_rgb_data_t *data)
{
    if (!reader) {
        return;
    }

    reader->data = data;
}

extern lierre_error_t lierre_reader_read(lierre_reader_t *reader, lierre_reader_result_t **result)
{
    const uint8_t *pixel;
    lierre_reader_result_t *res;
    decoder_t *decoder;
    decoder_result_t *dec_result;
    lierre_error_t err;
    uint32_t num_threads, scale, sum, dy, dx, scale_shift, temp;
    uint8_t *gray_data, *scaled_gray, r, g, b, gray;
    size_t i, j, start_x, start_y, width, height, src_x, src_y, src_idx, sw, sh, sy, sx, gx, gy;
    bool use_mt, use_quirc_grayscale;

    if (!reader || !result || !reader->data || !reader->data->data) {
        return LIERRE_ERROR_INVALID_PARAMS;
    }

    dec_result = lmalloc(sizeof(decoder_result_t));
    if (!dec_result) {
        return LIERRE_ERROR_DATA_OVERFLOW;
    }

    use_mt = (reader->param->strategy_flags & LIERRE_READER_STRATEGY_MT) != 0;
    num_threads = use_mt ? lierre_get_cpu_count() : 1;

    start_x = 0;
    start_y = 0;
    width = reader->data->width;
    height = reader->data->height;

    if ((reader->param->strategy_flags & LIERRE_READER_STRATEGY_USE_RECT) && reader->param->rect) {
        start_x = reader->param->rect->origin.x;
        start_y = reader->param->rect->origin.y;
        width = reader->param->rect->size.width;
        height = reader->param->rect->size.height;
    }

    gray_data = lmalloc(width * height);
    if (!gray_data) {
        lfree(dec_result);
        return LIERRE_ERROR_DATA_OVERFLOW;
    }

    if (start_x == 0 && start_y == 0 && width == reader->data->width && height == reader->data->height) {
        lierre_rgb_to_gray(reader->data->data, gray_data, width * height);
    } else {
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
                src_x = start_x + j;
                src_y = start_y + i;
                if (src_x >= reader->data->width || src_y >= reader->data->height) {
                    gray_data[i * width + j] = LIERRE_PIXEL_VALUE_DEFAULT;
                    continue;
                }
                src_idx = (src_y * reader->data->width + src_x) * 3;
                r = reader->data->data[src_idx];
                g = reader->data->data[src_idx + 1];
                b = reader->data->data[src_idx + 2];
                gray = (uint8_t)((r * LIERRE_GRAY_WEIGHT_R + g * LIERRE_GRAY_WEIGHT_G + b * LIERRE_GRAY_WEIGHT_B) >>
                                 LIERRE_GRAY_SHIFT);
                gray_data[i * width + j] = gray;
            }
        }
    }

    if (reader->param->strategy_flags & LIERRE_READER_STRATEGY_DENOISE) {
        if (use_mt) {
            image_denoise_mt(gray_data, width, height, num_threads);
        } else {
            image_denoise(gray_data, width, height);
        }
    }

    if (reader->param->strategy_flags & LIERRE_READER_STRATEGY_BRIGHTNESS_NORMALIZE) {
        image_brightness_normalize(gray_data, width, height);
    }

    if (reader->param->strategy_flags & LIERRE_READER_STRATEGY_CONTRAST_NORMALIZE) {
        image_contrast_normalize(gray_data, width, height);
    }

    if (reader->param->strategy_flags & LIERRE_READER_STRATEGY_SHARPENING) {
        if (use_mt) {
            image_sharpen_mt(gray_data, width, height, num_threads);
        } else {
            image_sharpen(gray_data, width, height);
        }
    }

    decoder = lierre_decoder_create();
    if (!decoder) {
        lfree(gray_data);
        lfree(dec_result);

        return LIERRE_ERROR_DATA_OVERFLOW;
    }

    dec_result->count = 0;

    if (reader->param->strategy_flags & LIERRE_READER_STRATEGY_MINIMIZE) {
        use_quirc_grayscale = (reader->param->strategy_flags & LIERRE_READER_STRATEGY_GLAYSCALE) != 0;

        for (scale = 1; scale <= LIERRE_IMAGE_MINIMIZE_MAX_SCALE; scale *= 2) {
            sw = width / scale;
            sh = height / scale;

            if (sw < LIERRE_MIN_QR_SIZE || sh < LIERRE_MIN_QR_SIZE) {
                break;
            }

            scale_shift = 0;
            temp = scale;
            while (temp > 1) {
                temp >>= 1;
                scale_shift++;
            }
            scale_shift *= 2;

            scaled_gray = lmalloc(sw * sh);
            if (!scaled_gray) {
                break;
            }

            if (use_quirc_grayscale) {
                for (sy = 0; sy < sh; sy++) {
                    for (sx = 0; sx < sw; sx++) {
                        sum = 0;
                        for (dy = 0; dy < scale; dy++) {
                            for (dx = 0; dx < scale; dx++) {
                                src_x = start_x + sx * scale + dx;
                                src_y = start_y + sy * scale + dy;
                                if (src_x >= reader->data->width || src_y >= reader->data->height) {
                                    sum += LIERRE_PIXEL_VALUE_DEFAULT;
                                    continue;
                                }
                                src_idx = (src_y * reader->data->width + src_x) * 3;
                                pixel = reader->data->data + src_idx;
                                sum += (pixel[0] * LIERRE_GRAY_WEIGHT_R + pixel[1] * LIERRE_GRAY_WEIGHT_G +
                                        pixel[2] * LIERRE_GRAY_WEIGHT_B) >>
                                       LIERRE_GRAY_SHIFT;
                            }
                        }
                        scaled_gray[sy * sw + sx] = (uint8_t)(sum >> scale_shift);
                    }
                }
            } else {
                for (sy = 0; sy < sh; sy++) {
                    for (sx = 0; sx < sw; sx++) {
                        sum = 0;
                        for (dy = 0; dy < scale; dy++) {
                            for (dx = 0; dx < scale; dx++) {
                                gx = sx * scale + dx;
                                gy = sy * scale + dy;
                                if (gx < width && gy < height) {
                                    sum += gray_data[gy * width + gx];
                                } else {
                                    sum += LIERRE_PIXEL_VALUE_DEFAULT;
                                }
                            }
                        }
                        scaled_gray[sy * sw + sx] = (uint8_t)(sum >> scale_shift);
                    }
                }
            }

            if (use_mt) {
                err =
                    lierre_decoder_process_mt(decoder, scaled_gray, (int32_t)sw, (int32_t)sh, dec_result, num_threads);
            } else {
                err = lierre_decoder_process(decoder, scaled_gray, (int32_t)sw, (int32_t)sh, dec_result);
            }

            lfree(scaled_gray);

            if (err == LIERRE_ERROR_SUCCESS && dec_result->count > 0) {
                break;
            }
        }

        lfree(gray_data);
    } else {
        if (use_mt) {
            err = lierre_decoder_process_mt(decoder, gray_data, (int32_t)width, (int32_t)height, dec_result,
                                            num_threads);
        } else {
            err = lierre_decoder_process(decoder, gray_data, (int32_t)width, (int32_t)height, dec_result);
        }
        lfree(gray_data);

        if (err != LIERRE_ERROR_SUCCESS) {
            lfree(dec_result);
            lierre_decoder_destroy(decoder);

            return err;
        }
    }

    res = lmalloc(sizeof(lierre_reader_result_t));
    if (!res) {
        lfree(dec_result);
        lierre_decoder_destroy(decoder);

        return LIERRE_ERROR_DATA_OVERFLOW;
    }

    res->num_qr_codes = dec_result->count;
    res->qr_code_rects = NULL;
    res->qr_code_datas = NULL;
    res->qr_code_data_sizes = NULL;

    if (dec_result->count > 0) {
        res->qr_code_rects = lcalloc(dec_result->count, sizeof(lierre_rect_t));
        res->qr_code_datas = lcalloc(dec_result->count, sizeof(uint8_t *));
        res->qr_code_data_sizes = lcalloc(dec_result->count, sizeof(size_t));

        if (!res->qr_code_rects || !res->qr_code_datas || !res->qr_code_data_sizes) {
            lfree(res->qr_code_rects);
            lfree(res->qr_code_datas);
            lfree(res->qr_code_data_sizes);
            lfree(res);
            lfree(dec_result);
            lierre_decoder_destroy(decoder);

            return LIERRE_ERROR_DATA_OVERFLOW;
        }

        for (i = 0; i < dec_result->count; i++) {
            res->qr_code_rects[i].origin.x = start_x + dec_result->codes[i].corners[0].x;
            res->qr_code_rects[i].origin.y = start_y + dec_result->codes[i].corners[0].y;
            res->qr_code_rects[i].size.width =
                (size_t)(dec_result->codes[i].corners[2].x - dec_result->codes[i].corners[0].x);
            res->qr_code_rects[i].size.height =
                (size_t)(dec_result->codes[i].corners[2].y - dec_result->codes[i].corners[0].y);

            res->qr_code_data_sizes[i] = (size_t)dec_result->codes[i].payload_len;
            res->qr_code_datas[i] = lmalloc(res->qr_code_data_sizes[i] + 1);
            if (res->qr_code_datas[i]) {
                lmemcpy(res->qr_code_datas[i], dec_result->codes[i].payload, res->qr_code_data_sizes[i]);
                res->qr_code_datas[i][res->qr_code_data_sizes[i]] = '\0';
            }
        }
    }

    lfree(dec_result);
    lierre_decoder_destroy(decoder);
    *result = res;

    return LIERRE_ERROR_SUCCESS;
}

extern void lierre_reader_result_destroy(lierre_reader_result_t *result)
{
    uint32_t i;

    if (!result) {
        return;
    }

    if (result->qr_code_datas) {
        for (i = 0; i < result->num_qr_codes; i++) {
            if (result->qr_code_datas[i]) {
                lfree(result->qr_code_datas[i]);
            }
        }
        lfree(result->qr_code_datas);
    }

    if (result->qr_code_rects) {
        lfree(result->qr_code_rects);
    }

    if (result->qr_code_data_sizes) {
        lfree(result->qr_code_data_sizes);
    }

    lfree(result);
}

extern uint32_t lierre_reader_result_get_num_qr_codes(const lierre_reader_result_t *result)
{
    if (!result) {
        return 0;
    }

    return result->num_qr_codes;
}

extern const lierre_rect_t *lierre_reader_result_get_qr_code_rect(const lierre_reader_result_t *result, uint32_t index)
{
    if (!result || index >= result->num_qr_codes || !result->qr_code_rects) {
        return NULL;
    }

    return &result->qr_code_rects[index];
}

extern const uint8_t *lierre_reader_result_get_qr_code_data(const lierre_reader_result_t *result, uint32_t index)
{
    if (!result || index >= result->num_qr_codes || !result->qr_code_datas) {
        return NULL;
    }

    return result->qr_code_datas[index];
}

extern size_t lierre_reader_result_get_qr_code_data_size(const lierre_reader_result_t *result, uint32_t index)
{
    if (!result || index >= result->num_qr_codes || !result->qr_code_data_sizes) {
        return 0;
    }

    return result->qr_code_data_sizes[index];
}

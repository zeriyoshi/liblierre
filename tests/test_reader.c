/*
 * liblierre - test_reader.c
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <string.h>

#include <lierre.h>
#include <lierre/reader.h>
#include <lierre/writer.h>

#include "unity.h"
#include "utils.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static inline uint8_t *generate_qr_rgb(const char *text, size_t *out_width, size_t *out_height)
{
    lierre_writer_param_t param;
    lierre_rgba_t fill = {0, 0, 0, 255}, bg = {255, 255, 255, 255};
    lierre_reso_t res;
    lierre_writer_t *writer;
    uint8_t *rgb_data;
    size_t i;

    lierre_writer_param_init(&param, (uint8_t *)text, strlen(text), 8, 4, ECC_MEDIUM, MASK_AUTO, MODE_BYTE);
    if (!lierre_writer_get_res(&param, &res)) {
        return NULL;
    }

    writer = lierre_writer_create(&param, &fill, &bg);
    if (!writer) {
        return NULL;
    }

    if (lierre_writer_write(writer) != LIERRE_ERROR_SUCCESS) {
        lierre_writer_destroy(writer);
        return NULL;
    }

    rgb_data = (uint8_t *)malloc(res.width * res.height * 3);
    if (!rgb_data) {
        lierre_writer_destroy(writer);
        return NULL;
    }

    for (i = 0; i < res.width * res.height; i++) {
        rgb_data[i * 3 + 0] = 255;
        rgb_data[i * 3 + 1] = 255;
        rgb_data[i * 3 + 2] = 255;
    }

    *out_width = res.width;
    *out_height = res.height;

    lierre_writer_destroy(writer);
    return rgb_data;
}

static inline lierre_rgb_data_t *generate_four_qr_image(const char *texts[4], lierre_rect_t positions[4])
{
    const uint8_t *rgba_data[4];
    lierre_writer_param_t param;
    lierre_rgba_t fill = {0, 0, 0, 255}, bg = {255, 255, 255, 255};
    lierre_reso_t res;
    lierre_writer_t *writers[4] = {NULL};
    lierre_rgb_data_t *result;
    uint8_t *rgb_data;
    size_t qr_sizes[4], canvas_width, canvas_height, margin = 20, max_qr_size = 0, i, x, y, grid_x, grid_y, offset_x,
                                                     offset_y, src_idx, dst_idx;

    for (i = 0; i < 4; i++) {
        lierre_writer_param_init(&param, (uint8_t *)texts[i], strlen(texts[i]), 4, 2, ECC_MEDIUM, MASK_AUTO, MODE_BYTE);
        if (!lierre_writer_get_res(&param, &res)) {
            goto bailout;
        }
        qr_sizes[i] = res.width;
        if (res.width > max_qr_size) {
            max_qr_size = res.width;
        }

        writers[i] = lierre_writer_create(&param, &fill, &bg);
        if (!writers[i]) {
            goto bailout;
        }

        if (lierre_writer_write(writers[i]) != LIERRE_ERROR_SUCCESS) {
            goto bailout;
        }

        rgba_data[i] = lierre_writer_get_rgba_data(writers[i]);
        if (!rgba_data[i]) {
            goto bailout;
        }
    }

    canvas_width = margin * 3 + max_qr_size * 2;
    canvas_height = margin * 3 + max_qr_size * 2;

    rgb_data = (uint8_t *)malloc(canvas_width * canvas_height * 3);
    if (!rgb_data) {
        goto bailout;
    }
    memset(rgb_data, 255, canvas_width * canvas_height * 3);

    for (i = 0; i < 4; i++) {
        grid_x = i % 2;
        grid_y = i / 2;
        offset_x = margin + grid_x * (max_qr_size + margin);
        offset_y = margin + grid_y * (max_qr_size + margin);

        positions[i].origin.x = offset_x;
        positions[i].origin.y = offset_y;
        positions[i].size.width = qr_sizes[i];
        positions[i].size.height = qr_sizes[i];

        for (y = 0; y < qr_sizes[i]; y++) {
            for (x = 0; x < qr_sizes[i]; x++) {
                src_idx = (y * qr_sizes[i] + x) * 4;
                dst_idx = ((offset_y + y) * canvas_width + (offset_x + x)) * 3;
                rgb_data[dst_idx + 0] = rgba_data[i][src_idx + 0];
                rgb_data[dst_idx + 1] = rgba_data[i][src_idx + 1];
                rgb_data[dst_idx + 2] = rgba_data[i][src_idx + 2];
            }
        }
    }

    result = lierre_rgb_create(rgb_data, canvas_width * canvas_height * 3, canvas_width, canvas_height);
    free(rgb_data);

    for (i = 0; i < 4; i++) {
        if (writers[i]) {
            lierre_writer_destroy(writers[i]);
        }
    }

    return result;

bailout:
    for (i = 0; i < 4; i++) {
        if (writers[i]) {
            lierre_writer_destroy(writers[i]);
        }
    }

    return NULL;
}

void test_reader_param_init_basic(void)
{
    lierre_reader_param_t param;
    lierre_error_t err;

    err = lierre_reader_param_init(&param);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_EQUAL(LIERRE_READER_STRATEGY_NONE, param.strategy_flags);
    TEST_ASSERT_NULL(param.rect);
}

void test_reader_param_init_null(void)
{
    lierre_error_t err;

    err = lierre_reader_param_init(NULL);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_INVALID_PARAMS, err);
}

void test_reader_param_set_flag_single(void)
{
    lierre_reader_param_t param;

    lierre_reader_param_init(&param);

    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_MINIMIZE);
    TEST_ASSERT_BITS_HIGH(LIERRE_READER_STRATEGY_MINIMIZE, param.strategy_flags);
}

void test_reader_param_set_flag_multiple(void)
{
    lierre_reader_param_t param;

    lierre_reader_param_init(&param);

    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_MINIMIZE);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_USE_RECT);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_DENOISE);

    TEST_ASSERT_BITS_HIGH(LIERRE_READER_STRATEGY_MINIMIZE, param.strategy_flags);
    TEST_ASSERT_BITS_HIGH(LIERRE_READER_STRATEGY_USE_RECT, param.strategy_flags);
    TEST_ASSERT_BITS_HIGH(LIERRE_READER_STRATEGY_DENOISE, param.strategy_flags);
}

void test_reader_param_set_flag_null(void)
{
    lierre_reader_param_set_flag(NULL, LIERRE_READER_STRATEGY_MINIMIZE);
    TEST_PASS();
}

void test_reader_param_set_flag_all(void)
{
    lierre_reader_param_t param;

    lierre_reader_param_init(&param);

    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_MINIMIZE);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_USE_RECT);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_DENOISE);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_BRIGHTNESS_NORMALIZE);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_CONTRAST_NORMALIZE);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_SHARPENING);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_MT);

    TEST_ASSERT_NOT_EQUAL(LIERRE_READER_STRATEGY_NONE, param.strategy_flags);
}

void test_reader_param_set_rect_basic(void)
{
    lierre_reader_param_t param;
    lierre_rect_t rect = {{10, 20}, {100, 200}};

    lierre_reader_param_init(&param);
    lierre_reader_param_set_rect(&param, &rect);
    TEST_ASSERT_EQUAL_PTR(&rect, param.rect);
}

void test_reader_param_set_rect_null_param(void)
{
    lierre_rect_t rect = {{0, 0}, {100, 100}};

    lierre_reader_param_set_rect(NULL, &rect);
    TEST_PASS();
}

void test_reader_param_set_rect_null_rect(void)
{
    lierre_reader_param_t param;

    lierre_reader_param_init(&param);

    lierre_reader_param_set_rect(&param, NULL);
    TEST_ASSERT_NULL(param.rect);
}

void test_reader_create_basic(void)
{
    lierre_reader_param_t param;
    lierre_reader_t *reader;

    lierre_reader_param_init(&param);
    reader = lierre_reader_create(&param);
    TEST_ASSERT_NOT_NULL(reader);
    lierre_reader_destroy(reader);
}

void test_reader_create_null(void)
{
    lierre_reader_t *reader;

    reader = lierre_reader_create(NULL);
    TEST_ASSERT_NULL(reader);
}

void test_reader_create_with_flags(void)
{
    lierre_reader_param_t param;
    lierre_reader_t *reader;

    lierre_reader_param_init(&param);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_MINIMIZE);
    reader = lierre_reader_create(&param);
    TEST_ASSERT_NOT_NULL(reader);
    lierre_reader_destroy(reader);
}

void test_reader_destroy_null(void)
{
    lierre_reader_destroy(NULL);
    TEST_PASS();
}

void test_reader_read_null_reader(void)
{
    lierre_reader_result_t *result = NULL;
    lierre_error_t err;

    err = lierre_reader_read(NULL, &result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_INVALID_PARAMS, err);
}

void test_reader_read_null_result(void)
{
    lierre_reader_param_t param;
    lierre_reader_t *reader;
    lierre_error_t err;

    lierre_reader_param_init(&param);
    reader = lierre_reader_create(&param);
    TEST_ASSERT_NOT_NULL(reader);
    err = lierre_reader_read(reader, NULL);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_INVALID_PARAMS, err);
    lierre_reader_destroy(reader);
}

void test_reader_read_no_data(void)
{
    lierre_reader_param_t param;
    lierre_reader_t *reader;
    lierre_reader_result_t *result = NULL;
    lierre_error_t err;

    lierre_reader_param_init(&param);
    reader = lierre_reader_create(&param);
    TEST_ASSERT_NOT_NULL(reader);
    err = lierre_reader_read(reader, &result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_INVALID_PARAMS, err);
    lierre_reader_destroy(reader);
}

void test_reader_read_with_valid_data(void)
{
    lierre_rgb_data_t *rgb;
    lierre_reader_param_t param;
    lierre_reader_t *reader;
    lierre_reader_result_t *result = NULL;
    lierre_error_t err;
    uint8_t *rgb_data;
    size_t width = 200, height = 200;

    rgb_data = (uint8_t *)malloc(width * height * 3);
    TEST_ASSERT_NOT_NULL(rgb_data);
    memset(rgb_data, 255, width * height * 3);

    rgb = lierre_rgb_create(rgb_data, width * height * 3, width, height);
    TEST_ASSERT_NOT_NULL(rgb);

    lierre_reader_param_init(&param);
    reader = lierre_reader_create(&param);
    TEST_ASSERT_NOT_NULL(reader);

    lierre_reader_set_data(reader, rgb);

    err = lierre_reader_read(reader, &result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(0, lierre_reader_result_get_num_qr_codes(result));

    lierre_reader_result_destroy(result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb);
    free(rgb_data);
}

void test_reader_read_with_all_strategies(void)
{
    lierre_rgb_data_t *rgb;
    lierre_reader_param_t param;
    lierre_reader_t *reader;
    lierre_reader_result_t *result = NULL;
    lierre_error_t err;
    uint8_t *rgb_data;
    size_t width = 200, height = 200;

    rgb_data = (uint8_t *)malloc(width * height * 3);
    TEST_ASSERT_NOT_NULL(rgb_data);
    memset(rgb_data, 128, width * height * 3);

    rgb = lierre_rgb_create(rgb_data, width * height * 3, width, height);
    TEST_ASSERT_NOT_NULL(rgb);

    lierre_reader_param_init(&param);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_DENOISE);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_BRIGHTNESS_NORMALIZE);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_CONTRAST_NORMALIZE);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_SHARPENING);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_MINIMIZE);

    reader = lierre_reader_create(&param);
    TEST_ASSERT_NOT_NULL(reader);

    lierre_reader_set_data(reader, rgb);

    err = lierre_reader_read(reader, &result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(result);

    lierre_reader_result_destroy(result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb);
    free(rgb_data);
}

void test_reader_read_with_mt_strategy(void)
{
    lierre_rgb_data_t *rgb;
    lierre_reader_param_t param;
    lierre_reader_t *reader;
    lierre_reader_result_t *result = NULL;
    lierre_error_t err;
    uint8_t *rgb_data;
    size_t width = 200, height = 200;

    rgb_data = (uint8_t *)malloc(width * height * 3);
    TEST_ASSERT_NOT_NULL(rgb_data);
    memset(rgb_data, 128, width * height * 3);

    rgb = lierre_rgb_create(rgb_data, width * height * 3, width, height);
    TEST_ASSERT_NOT_NULL(rgb);

    lierre_reader_param_init(&param);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_MT);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_DENOISE);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_SHARPENING);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_MINIMIZE);

    reader = lierre_reader_create(&param);
    TEST_ASSERT_NOT_NULL(reader);

    lierre_reader_set_data(reader, rgb);

    err = lierre_reader_read(reader, &result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(result);

    lierre_reader_result_destroy(result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb);
    free(rgb_data);
}

void test_reader_read_with_rect_data(void)
{
    lierre_rgb_data_t *rgb;
    lierre_rect_t rect = {{10, 10}, {100, 100}};
    lierre_reader_param_t param;
    lierre_reader_t *reader;
    lierre_reader_result_t *result = NULL;
    lierre_error_t err;
    uint8_t *rgb_data;
    size_t width = 200, height = 200;

    rgb_data = (uint8_t *)malloc(width * height * 3);
    TEST_ASSERT_NOT_NULL(rgb_data);
    memset(rgb_data, 255, width * height * 3);

    rgb = lierre_rgb_create(rgb_data, width * height * 3, width, height);
    TEST_ASSERT_NOT_NULL(rgb);

    lierre_reader_param_init(&param);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_USE_RECT);
    lierre_reader_param_set_rect(&param, &rect);

    reader = lierre_reader_create(&param);
    TEST_ASSERT_NOT_NULL(reader);

    lierre_reader_set_data(reader, rgb);

    err = lierre_reader_read(reader, &result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(result);

    lierre_reader_result_destroy(result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb);
    free(rgb_data);
}

void test_reader_set_data_null(void)
{
    lierre_reader_set_data(NULL, NULL);
    TEST_PASS();
}

void test_reader_result_destroy_null(void)
{
    lierre_reader_result_destroy(NULL);
    TEST_PASS();
}

void test_reader_result_get_num_qr_codes_null(void)
{
    uint32_t count;

    count = lierre_reader_result_get_num_qr_codes(NULL);
    TEST_ASSERT_EQUAL(0, count);
}

void test_reader_result_get_qr_code_rect_null(void)
{
    const lierre_rect_t *rect;

    rect = lierre_reader_result_get_qr_code_rect(NULL, 0);
    TEST_ASSERT_NULL(rect);
}

void test_reader_result_get_qr_code_data_null(void)
{
    const uint8_t *data;

    data = lierre_reader_result_get_qr_code_data(NULL, 0);
    TEST_ASSERT_NULL(data);
}

void test_reader_result_get_qr_code_data_size_null(void)
{
    size_t size;

    size = lierre_reader_result_get_qr_code_data_size(NULL, 0);
    TEST_ASSERT_EQUAL(0, size);
}

void test_reader_with_simple_rgb_data(void)
{
    lierre_rgb_data_t *rgb;
    uint8_t *rgb_data;
    size_t width = 100, height = 100;

    rgb_data = (uint8_t *)malloc(width * height * 3);
    TEST_ASSERT_NOT_NULL(rgb_data);
    memset(rgb_data, 255, width * height * 3);
    rgb = lierre_rgb_create(rgb_data, width * height * 3, width, height);
    TEST_ASSERT_NOT_NULL(rgb);
    lierre_rgb_destroy(rgb);
    free(rgb_data);
}

void test_reader_read_with_rect_strategy(void)
{
    lierre_reader_param_t param;
    lierre_rect_t rect = {{0, 0}, {50, 50}};
    lierre_reader_t *reader;

    lierre_reader_param_init(&param);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_USE_RECT);
    lierre_reader_param_set_rect(&param, &rect);
    reader = lierre_reader_create(&param);
    TEST_ASSERT_NOT_NULL(reader);
    lierre_reader_destroy(reader);
}

void test_reader_multiple_strategies(void)
{
    lierre_reader_param_t param;
    lierre_reader_t *reader;

    lierre_reader_param_init(&param);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_MINIMIZE);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_DENOISE);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_BRIGHTNESS_NORMALIZE);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_CONTRAST_NORMALIZE);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_SHARPENING);
    reader = lierre_reader_create(&param);
    TEST_ASSERT_NOT_NULL(reader);
    lierre_reader_destroy(reader);
}

void test_reader_create_destroy_cycle(void)
{
    lierre_reader_param_t param;
    lierre_reader_t *reader;
    int i;

    for (i = 0; i < 10; i++) {
        lierre_reader_param_init(&param);
        reader = lierre_reader_create(&param);
        TEST_ASSERT_NOT_NULL(reader);
        lierre_reader_destroy(reader);
    }
}

void test_reader_load_bmp_noise_normal(void)
{
    lierre_rgb_data_t *rgb;

    rgb = test_load_bmp("../assets/noise_normal_1920_1080.bmp");
    TEST_ASSERT_NOT_NULL(rgb);
    TEST_ASSERT_EQUAL(1920, rgb->width);
    TEST_ASSERT_EQUAL(1080, rgb->height);
    TEST_ASSERT_EQUAL(1920 * 1080 * 3, rgb->data_size);
    lierre_rgb_destroy(rgb);
}

void test_reader_load_bmp_noise_min(void)
{
    lierre_rgb_data_t *rgb;

    rgb = test_load_bmp("../assets/noise_min_1920_1080.bmp");
    TEST_ASSERT_NOT_NULL(rgb);
    TEST_ASSERT_EQUAL(1920, rgb->width);
    TEST_ASSERT_EQUAL(1080, rgb->height);
    lierre_rgb_destroy(rgb);
}

void test_reader_load_bmp_invalid_path(void)
{
    lierre_rgb_data_t *rgb;

    rgb = test_load_bmp("nonexistent.bmp");
    TEST_ASSERT_NULL(rgb);
}

void test_reader_noise_normal_fails_without_strategy(void)
{
    lierre_rgb_data_t *rgb;
    lierre_reader_param_t param;
    lierre_reader_t *reader;
    lierre_reader_result_t *result = NULL;
    lierre_error_t err;

    rgb = test_load_bmp("../assets/noise_normal_1920_1080.bmp");
    TEST_ASSERT_NOT_NULL(rgb);

    lierre_reader_param_init(&param);

    reader = lierre_reader_create(&param);
    TEST_ASSERT_NOT_NULL(reader);
    lierre_reader_set_data(reader, rgb);

    err = lierre_reader_read(reader, &result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(result);

    TEST_ASSERT_EQUAL_UINT32(0, lierre_reader_result_get_num_qr_codes(result));

    lierre_reader_result_destroy(result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb);
}

void test_reader_noise_min_fails_without_strategy(void)
{
    lierre_rgb_data_t *rgb;
    lierre_reader_param_t param;
    lierre_reader_t *reader;
    lierre_reader_result_t *result = NULL;
    lierre_error_t err;

    rgb = test_load_bmp("../assets/noise_min_1920_1080.bmp");
    TEST_ASSERT_NOT_NULL(rgb);

    lierre_reader_param_init(&param);

    reader = lierre_reader_create(&param);
    TEST_ASSERT_NOT_NULL(reader);
    lierre_reader_set_data(reader, rgb);

    err = lierre_reader_read(reader, &result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(result);

    TEST_ASSERT_EQUAL_UINT32(0, lierre_reader_result_get_num_qr_codes(result));

    lierre_reader_result_destroy(result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb);
}

void test_reader_read_noisy_image_with_strategies(void)
{
    lierre_rgb_data_t *rgb;
    lierre_reader_param_t param;
    lierre_reader_t *reader;
    lierre_reader_result_t *result = NULL;
    lierre_error_t err;

    rgb = test_load_bmp("../assets/noise_normal_1920_1080.bmp");
    TEST_ASSERT_NOT_NULL(rgb);

    lierre_reader_param_init(&param);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_MINIMIZE);

    reader = lierre_reader_create(&param);
    TEST_ASSERT_NOT_NULL(reader);
    lierre_reader_set_data(reader, rgb);

    err = lierre_reader_read(reader, &result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(result);

    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(1, lierre_reader_result_get_num_qr_codes(result));

    lierre_reader_result_destroy(result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb);
}

void test_reader_read_min_image_with_minimize(void)
{
    lierre_rgb_data_t *rgb;
    lierre_reader_param_t param;
    lierre_reader_t *reader;
    lierre_reader_result_t *result = NULL;
    lierre_error_t err;

    rgb = test_load_bmp("../assets/noise_min_1920_1080.bmp");
    TEST_ASSERT_NOT_NULL(rgb);

    lierre_reader_param_init(&param);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_MINIMIZE);

    reader = lierre_reader_create(&param);
    TEST_ASSERT_NOT_NULL(reader);
    lierre_reader_set_data(reader, rgb);

    err = lierre_reader_read(reader, &result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(result);

    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(1, lierre_reader_result_get_num_qr_codes(result));

    lierre_reader_result_destroy(result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb);
}

void test_reader_read_with_mt_on_real_image(void)
{
    lierre_rgb_data_t *rgb;
    lierre_reader_param_t param;
    lierre_reader_t *reader;
    lierre_reader_result_t *result = NULL;
    lierre_error_t err;

    rgb = test_load_bmp("../assets/noise_normal_1920_1080.bmp");
    TEST_ASSERT_NOT_NULL(rgb);

    lierre_reader_param_init(&param);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_MT);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_DENOISE);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_SHARPENING);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_MINIMIZE);

    reader = lierre_reader_create(&param);
    TEST_ASSERT_NOT_NULL(reader);
    lierre_reader_set_data(reader, rgb);

    err = lierre_reader_read(reader, &result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(result);

    lierre_reader_result_destroy(result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb);
}

void test_reader_read_with_rect_on_real_image(void)
{
    lierre_rgb_data_t *rgb;
    lierre_rect_t rect = {{100, 100}, {800, 600}};
    lierre_reader_param_t param;
    lierre_reader_t *reader;
    lierre_reader_result_t *result = NULL;
    lierre_error_t err;

    rgb = test_load_bmp("../assets/noise_normal_1920_1080.bmp");
    TEST_ASSERT_NOT_NULL(rgb);

    lierre_reader_param_init(&param);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_USE_RECT);
    lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_DENOISE);
    lierre_reader_param_set_rect(&param, &rect);

    reader = lierre_reader_create(&param);
    TEST_ASSERT_NOT_NULL(reader);
    lierre_reader_set_data(reader, rgb);

    err = lierre_reader_read(reader, &result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(result);

    lierre_reader_result_destroy(result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb);
}

void test_reader_four_qr_read_single_with_rect(void)
{
    const char *texts[4] = {"QR_CODE_1", "QR_CODE_2", "QR_CODE_3", "QR_CODE_4"};
    const uint8_t *data;
    lierre_rect_t positions[4];
    lierre_rgb_data_t *rgb;
    lierre_reader_param_t param;
    lierre_reader_t *reader;
    lierre_reader_result_t *result = NULL;
    lierre_error_t err;
    size_t data_size;
    int i;

    rgb = generate_four_qr_image(texts, positions);
    TEST_ASSERT_NOT_NULL(rgb);

    for (i = 0; i < 4; i++) {
        lierre_reader_param_init(&param);
        lierre_reader_param_set_flag(&param, LIERRE_READER_STRATEGY_USE_RECT);
        lierre_reader_param_set_rect(&param, &positions[i]);

        reader = lierre_reader_create(&param);
        TEST_ASSERT_NOT_NULL(reader);
        lierre_reader_set_data(reader, rgb);

        err = lierre_reader_read(reader, &result);
        TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
        TEST_ASSERT_NOT_NULL(result);
        TEST_ASSERT_EQUAL_UINT32(1, lierre_reader_result_get_num_qr_codes(result));

        data = lierre_reader_result_get_qr_code_data(result, 0);
        data_size = lierre_reader_result_get_qr_code_data_size(result, 0);
        TEST_ASSERT_NOT_NULL(data);
        TEST_ASSERT_EQUAL(strlen(texts[i]), data_size);
        TEST_ASSERT_EQUAL_MEMORY(texts[i], data, data_size);

        lierre_reader_result_destroy(result);
        result = NULL;
        lierre_reader_destroy(reader);
    }

    lierre_rgb_destroy(rgb);
}

void test_reader_four_qr_read_all_without_rect(void)
{
    const char *texts[4] = {"QR_CODE_A", "QR_CODE_B", "QR_CODE_C", "QR_CODE_D"};
    const uint8_t *data, *data2;
    lierre_rect_t positions[4];
    lierre_rgb_data_t *rgb;
    lierre_reader_param_t param;
    lierre_reader_t *reader;
    lierre_reader_result_t *result = NULL;
    lierre_error_t err;
    uint32_t num_qr_codes, i;
    size_t data2_size;
    int found[4] = {0}, j;

    rgb = generate_four_qr_image(texts, positions);
    TEST_ASSERT_NOT_NULL(rgb);

    lierre_reader_param_init(&param);

    reader = lierre_reader_create(&param);
    TEST_ASSERT_NOT_NULL(reader);
    lierre_reader_set_data(reader, rgb);

    err = lierre_reader_read(reader, &result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(result);

    num_qr_codes = lierre_reader_result_get_num_qr_codes(result);
    TEST_ASSERT_EQUAL_UINT32(4, num_qr_codes);

    for (i = 0; i < num_qr_codes; i++) {
        data2 = lierre_reader_result_get_qr_code_data(result, i);
        data2_size = lierre_reader_result_get_qr_code_data_size(result, i);

        TEST_ASSERT_NOT_NULL(data2);
        TEST_ASSERT_GREATER_THAN(0, data2_size);

        for (j = 0; j < 4; j++) {
            if (data2_size == strlen(texts[j]) && memcmp(data2, texts[j], data2_size) == 0) {
                found[j] = 1;
                break;
            }
        }
    }

    TEST_ASSERT_EQUAL(1, found[0]);
    TEST_ASSERT_EQUAL(1, found[1]);
    TEST_ASSERT_EQUAL(1, found[2]);
    TEST_ASSERT_EQUAL(1, found[3]);

    lierre_reader_result_destroy(result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_reader_param_init_basic);
    RUN_TEST(test_reader_param_init_null);

    RUN_TEST(test_reader_param_set_flag_single);
    RUN_TEST(test_reader_param_set_flag_multiple);
    RUN_TEST(test_reader_param_set_flag_null);
    RUN_TEST(test_reader_param_set_flag_all);

    RUN_TEST(test_reader_param_set_rect_basic);
    RUN_TEST(test_reader_param_set_rect_null_param);
    RUN_TEST(test_reader_param_set_rect_null_rect);

    RUN_TEST(test_reader_create_basic);
    RUN_TEST(test_reader_create_null);
    RUN_TEST(test_reader_create_with_flags);

    RUN_TEST(test_reader_destroy_null);

    RUN_TEST(test_reader_read_null_reader);
    RUN_TEST(test_reader_read_null_result);
    RUN_TEST(test_reader_read_no_data);
    RUN_TEST(test_reader_read_with_valid_data);
    RUN_TEST(test_reader_read_with_all_strategies);
    RUN_TEST(test_reader_read_with_mt_strategy);
    RUN_TEST(test_reader_read_with_rect_data);
    RUN_TEST(test_reader_set_data_null);

    RUN_TEST(test_reader_result_destroy_null);

    RUN_TEST(test_reader_result_get_num_qr_codes_null);
    RUN_TEST(test_reader_result_get_qr_code_rect_null);
    RUN_TEST(test_reader_result_get_qr_code_data_null);
    RUN_TEST(test_reader_result_get_qr_code_data_size_null);

    RUN_TEST(test_reader_with_simple_rgb_data);
    RUN_TEST(test_reader_read_with_rect_strategy);
    RUN_TEST(test_reader_multiple_strategies);
    RUN_TEST(test_reader_create_destroy_cycle);

    RUN_TEST(test_reader_load_bmp_noise_normal);
    RUN_TEST(test_reader_load_bmp_noise_min);
    RUN_TEST(test_reader_load_bmp_invalid_path);

    RUN_TEST(test_reader_noise_normal_fails_without_strategy);
    RUN_TEST(test_reader_noise_min_fails_without_strategy);

    RUN_TEST(test_reader_read_noisy_image_with_strategies);
    RUN_TEST(test_reader_read_min_image_with_minimize);
    RUN_TEST(test_reader_read_with_mt_on_real_image);
    RUN_TEST(test_reader_read_with_rect_on_real_image);

    RUN_TEST(test_reader_four_qr_read_single_with_rect);
    RUN_TEST(test_reader_four_qr_read_all_without_rect);

    return UNITY_END();
}

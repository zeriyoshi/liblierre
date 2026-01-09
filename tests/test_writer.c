/*
 * liblierre - test_writer.c
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <string.h>

#include <lierre.h>
#include <lierre/writer.h>

#include "unity.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_writer_param_init_basic(void)
{
    lierre_writer_param_t param;
    lierre_error_t err;
    uint8_t data[] = "Hello";

    err = lierre_writer_param_init(&param, data, 5, 4, 2, ECC_LOW, MASK_AUTO, MODE_BYTE);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_EQUAL_PTR(data, param.data);
    TEST_ASSERT_EQUAL(5, param.data_size);
    TEST_ASSERT_EQUAL(4, param.scale);
    TEST_ASSERT_EQUAL(2, param.margin);
    TEST_ASSERT_EQUAL(ECC_LOW, param.ecc_level);
    TEST_ASSERT_EQUAL(MASK_AUTO, param.mask_pattern);
}

void test_writer_param_init_null_param(void)
{
    lierre_error_t err;
    uint8_t data[] = "Hello";

    err = lierre_writer_param_init(NULL, data, 5, 4, 2, ECC_LOW, MASK_AUTO, MODE_BYTE);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_INVALID_PARAMS, err);
}

void test_writer_param_init_null_data(void)
{
    lierre_writer_param_t param;
    lierre_error_t err;

    err = lierre_writer_param_init(&param, NULL, 5, 4, 2, ECC_LOW, MASK_AUTO, MODE_BYTE);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_INVALID_PARAMS, err);
}

void test_writer_param_init_zero_data_size(void)
{
    lierre_writer_param_t param;
    lierre_error_t err;
    uint8_t data[] = "Hello";

    err = lierre_writer_param_init(&param, data, 0, 4, 2, ECC_LOW, MASK_AUTO, MODE_BYTE);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_INVALID_PARAMS, err);
}

void test_writer_param_init_zero_scale(void)
{
    lierre_writer_param_t param;
    lierre_error_t err;
    uint8_t data[] = "Hello";

    err = lierre_writer_param_init(&param, data, 5, 0, 2, ECC_LOW, MASK_AUTO, MODE_BYTE);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_INVALID_PARAMS, err);
}

void test_writer_param_init_all_ecc_levels(void)
{
    lierre_writer_param_t param;
    uint8_t data[] = "Test";

    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS,
                      lierre_writer_param_init(&param, data, 4, 1, 1, ECC_LOW, MASK_AUTO, MODE_BYTE));
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS,
                      lierre_writer_param_init(&param, data, 4, 1, 1, ECC_MEDIUM, MASK_AUTO, MODE_BYTE));
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS,
                      lierre_writer_param_init(&param, data, 4, 1, 1, ECC_QUARTILE, MASK_AUTO, MODE_BYTE));
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS,
                      lierre_writer_param_init(&param, data, 4, 1, 1, ECC_HIGH, MASK_AUTO, MODE_BYTE));
}

void test_writer_param_init_all_mask_patterns(void)
{
    lierre_writer_param_t param;
    uint8_t data[] = "Test";

    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS,
                      lierre_writer_param_init(&param, data, 4, 1, 1, ECC_LOW, MASK_AUTO, MODE_BYTE));
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS,
                      lierre_writer_param_init(&param, data, 4, 1, 1, ECC_LOW, MASK_0, MODE_BYTE));
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS,
                      lierre_writer_param_init(&param, data, 4, 1, 1, ECC_LOW, MASK_1, MODE_BYTE));
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS,
                      lierre_writer_param_init(&param, data, 4, 1, 1, ECC_LOW, MASK_2, MODE_BYTE));
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS,
                      lierre_writer_param_init(&param, data, 4, 1, 1, ECC_LOW, MASK_3, MODE_BYTE));
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS,
                      lierre_writer_param_init(&param, data, 4, 1, 1, ECC_LOW, MASK_4, MODE_BYTE));
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS,
                      lierre_writer_param_init(&param, data, 4, 1, 1, ECC_LOW, MASK_5, MODE_BYTE));
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS,
                      lierre_writer_param_init(&param, data, 4, 1, 1, ECC_LOW, MASK_6, MODE_BYTE));
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS,
                      lierre_writer_param_init(&param, data, 4, 1, 1, ECC_LOW, MASK_7, MODE_BYTE));
}

void test_writer_param_init_various_scales(void)
{
    lierre_writer_param_t param;
    uint8_t data[] = "Test";
    size_t scale;

    for (scale = 1; scale <= 10; scale++) {
        TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS,
                          lierre_writer_param_init(&param, data, 4, scale, 0, ECC_LOW, MASK_AUTO, MODE_BYTE));
        TEST_ASSERT_EQUAL(scale, param.scale);
    }
}

void test_writer_param_init_various_margins(void)
{
    lierre_writer_param_t param;
    uint8_t data[] = "Test";
    size_t margin;

    for (margin = 0; margin <= 10; margin++) {
        TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS,
                          lierre_writer_param_init(&param, data, 4, 1, margin, ECC_LOW, MASK_AUTO, MODE_BYTE));
        TEST_ASSERT_EQUAL(margin, param.margin);
    }
}

void test_writer_qr_version_small_data(void)
{
    lierre_writer_param_t param;
    lierre_qr_version_t ver;
    uint8_t data[] = "Hi";

    lierre_writer_param_init(&param, data, 2, 1, 0, ECC_LOW, MASK_AUTO, MODE_BYTE);
    ver = lierre_writer_qr_version(&param);
    TEST_ASSERT_EQUAL(QR_VERSION_1, ver);
}

void test_writer_qr_version_null_param(void)
{
    lierre_qr_version_t ver;

    ver = lierre_writer_qr_version(NULL);
    TEST_ASSERT_EQUAL(QR_VERSION_ERR, ver);
}

void test_writer_qr_version_null_data(void)
{
    lierre_writer_param_t param;
    lierre_qr_version_t ver;

    memset(&param, 0, sizeof(param));
    param.data = NULL;
    param.data_size = 5;
    ver = lierre_writer_qr_version(&param);
    TEST_ASSERT_EQUAL(QR_VERSION_ERR, ver);
}

void test_writer_qr_version_zero_data_size(void)
{
    lierre_writer_param_t param;
    lierre_qr_version_t ver;
    uint8_t data[] = "Test";

    memset(&param, 0, sizeof(param));
    param.data = data;
    param.data_size = 0;
    ver = lierre_writer_qr_version(&param);
    TEST_ASSERT_EQUAL(QR_VERSION_ERR, ver);
}

void test_writer_qr_version_data_too_large(void)
{
    lierre_writer_param_t param;
    lierre_qr_version_t ver;
    uint8_t *data;

    data = (uint8_t *)malloc(5000);
    TEST_ASSERT_NOT_NULL(data);
    memset(data, 'A', 5000);
    lierre_writer_param_init(&param, data, 5000, 1, 0, ECC_HIGH, MASK_AUTO, MODE_BYTE);
    ver = lierre_writer_qr_version(&param);
    TEST_ASSERT_EQUAL(QR_VERSION_ERR, ver);
    free(data);
}

void test_writer_qr_version_all_ecc_levels(void)
{
    lierre_writer_param_t param;
    uint8_t data[100];

    memset(data, 'A', sizeof(data));

    lierre_writer_param_init(&param, data, 50, 1, 0, ECC_LOW, MASK_AUTO, MODE_BYTE);
    TEST_ASSERT_NOT_EQUAL(QR_VERSION_ERR, lierre_writer_qr_version(&param));

    lierre_writer_param_init(&param, data, 50, 1, 0, ECC_MEDIUM, MASK_AUTO, MODE_BYTE);
    TEST_ASSERT_NOT_EQUAL(QR_VERSION_ERR, lierre_writer_qr_version(&param));

    lierre_writer_param_init(&param, data, 50, 1, 0, ECC_QUARTILE, MASK_AUTO, MODE_BYTE);
    TEST_ASSERT_NOT_EQUAL(QR_VERSION_ERR, lierre_writer_qr_version(&param));

    lierre_writer_param_init(&param, data, 50, 1, 0, ECC_HIGH, MASK_AUTO, MODE_BYTE);
    TEST_ASSERT_NOT_EQUAL(QR_VERSION_ERR, lierre_writer_qr_version(&param));
}

void test_writer_qr_version_invalid_ecc_fallback(void)
{
    lierre_writer_param_t param;
    lierre_qr_version_t ver;
    uint8_t data[] = "Test";

    memset(&param, 0, sizeof(param));
    param.data = data;
    param.data_size = 4;
    param.ecc_level = (lierre_writer_ecc_t)99;
    ver = lierre_writer_qr_version(&param);
    TEST_ASSERT_NOT_EQUAL(QR_VERSION_ERR, ver);
}

void test_writer_get_res_basic(void)
{
    lierre_writer_param_t param;
    lierre_reso_t res;
    uint8_t data[] = "Hello";
    bool success;

    lierre_writer_param_init(&param, data, 5, 4, 2, ECC_LOW, MASK_AUTO, MODE_BYTE);
    success = lierre_writer_get_res(&param, &res);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_GREATER_THAN(0, res.width);
    TEST_ASSERT_GREATER_THAN(0, res.height);
    TEST_ASSERT_EQUAL(res.width, res.height);
}

void test_writer_get_res_null_param(void)
{
    lierre_reso_t res;
    bool success;

    success = lierre_writer_get_res(NULL, &res);
    TEST_ASSERT_FALSE(success);
}

void test_writer_get_res_null_res(void)
{
    lierre_writer_param_t param;
    uint8_t data[] = "Hello";
    bool success;

    lierre_writer_param_init(&param, data, 5, 4, 2, ECC_LOW, MASK_AUTO, MODE_BYTE);
    success = lierre_writer_get_res(&param, NULL);
    TEST_ASSERT_FALSE(success);
}

void test_writer_get_res_data_too_large(void)
{
    lierre_writer_param_t param;
    lierre_reso_t res;
    uint8_t *data;
    bool success;

    data = (uint8_t *)malloc(5000);
    TEST_ASSERT_NOT_NULL(data);
    memset(data, 'A', 5000);
    lierre_writer_param_init(&param, data, 5000, 1, 0, ECC_HIGH, MASK_AUTO, MODE_BYTE);
    success = lierre_writer_get_res(&param, &res);
    TEST_ASSERT_FALSE(success);
    free(data);
}

void test_writer_get_res_scale_effect(void)
{
    lierre_writer_param_t param;
    lierre_reso_t res1, res2;
    uint8_t data[] = "Test";

    lierre_writer_param_init(&param, data, 4, 1, 0, ECC_LOW, MASK_AUTO, MODE_BYTE);
    lierre_writer_get_res(&param, &res1);

    lierre_writer_param_init(&param, data, 4, 2, 0, ECC_LOW, MASK_AUTO, MODE_BYTE);
    lierre_writer_get_res(&param, &res2);

    TEST_ASSERT_EQUAL(res1.width * 2, res2.width);
}

void test_writer_get_res_margin_effect(void)
{
    lierre_writer_param_t param;
    lierre_reso_t res1, res2;
    uint8_t data[] = "Test";

    lierre_writer_param_init(&param, data, 4, 1, 0, ECC_LOW, MASK_AUTO, MODE_BYTE);
    lierre_writer_get_res(&param, &res1);

    lierre_writer_param_init(&param, data, 4, 1, 2, ECC_LOW, MASK_AUTO, MODE_BYTE);
    lierre_writer_get_res(&param, &res2);

    TEST_ASSERT_EQUAL(res1.width + 4, res2.width);
}

void test_writer_get_res_width_basic(void)
{
    lierre_writer_param_t param;
    uint8_t data[] = "Hello";
    size_t width;

    lierre_writer_param_init(&param, data, 5, 4, 2, ECC_LOW, MASK_AUTO, MODE_BYTE);
    width = lierre_writer_get_res_width(&param);
    TEST_ASSERT_GREATER_THAN(0, width);
}

void test_writer_get_res_width_null(void)
{
    size_t width;

    width = lierre_writer_get_res_width(NULL);
    TEST_ASSERT_EQUAL(0, width);
}

void test_writer_get_res_width_invalid(void)
{
    lierre_writer_param_t param;
    uint8_t *data;
    size_t width;

    data = (uint8_t *)malloc(5000);
    TEST_ASSERT_NOT_NULL(data);
    memset(data, 'A', 5000);
    lierre_writer_param_init(&param, data, 5000, 1, 0, ECC_HIGH, MASK_AUTO, MODE_BYTE);
    width = lierre_writer_get_res_width(&param);
    TEST_ASSERT_EQUAL(0, width);
    free(data);
}

void test_writer_get_res_height_basic(void)
{
    lierre_writer_param_t param;
    uint8_t data[] = "Hello";
    size_t height;

    lierre_writer_param_init(&param, data, 5, 4, 2, ECC_LOW, MASK_AUTO, MODE_BYTE);
    height = lierre_writer_get_res_height(&param);
    TEST_ASSERT_GREATER_THAN(0, height);
}

void test_writer_get_res_height_null(void)
{
    size_t height;

    height = lierre_writer_get_res_height(NULL);
    TEST_ASSERT_EQUAL(0, height);
}

void test_writer_get_res_height_matches_width(void)
{
    lierre_writer_param_t param;
    uint8_t data[] = "Hello";
    size_t width, height;

    lierre_writer_param_init(&param, data, 5, 4, 2, ECC_LOW, MASK_AUTO, MODE_BYTE);
    width = lierre_writer_get_res_width(&param);
    height = lierre_writer_get_res_height(&param);
    TEST_ASSERT_EQUAL(width, height);
}

void test_writer_create_basic(void)
{
    lierre_writer_param_t param;
    lierre_rgba_t fill = {0, 0, 0, 255}, bg = {255, 255, 255, 255};
    lierre_writer_t *writer;
    uint8_t data[] = "Hello";

    lierre_writer_param_init(&param, data, 5, 4, 2, ECC_LOW, MASK_AUTO, MODE_BYTE);
    writer = lierre_writer_create(&param, &fill, &bg);
    TEST_ASSERT_NOT_NULL(writer);
    lierre_writer_destroy(writer);
}

void test_writer_create_null_param(void)
{
    lierre_rgba_t fill = {0, 0, 0, 255}, bg = {255, 255, 255, 255};
    lierre_writer_t *writer;

    writer = lierre_writer_create(NULL, &fill, &bg);
    TEST_ASSERT_NULL(writer);
}

void test_writer_create_null_fill(void)
{
    lierre_writer_param_t param;
    lierre_rgba_t bg = {255, 255, 255, 255};
    lierre_writer_t *writer;
    uint8_t data[] = "Hello";

    lierre_writer_param_init(&param, data, 5, 4, 2, ECC_LOW, MASK_AUTO, MODE_BYTE);
    writer = lierre_writer_create(&param, NULL, &bg);
    TEST_ASSERT_NULL(writer);
}

void test_writer_create_null_bg(void)
{
    lierre_writer_param_t param;
    lierre_rgba_t fill = {0, 0, 0, 255};
    lierre_writer_t *writer;
    uint8_t data[] = "Hello";

    lierre_writer_param_init(&param, data, 5, 4, 2, ECC_LOW, MASK_AUTO, MODE_BYTE);
    writer = lierre_writer_create(&param, &fill, NULL);
    TEST_ASSERT_NULL(writer);
}

void test_writer_create_data_too_large(void)
{
    lierre_writer_param_t param;
    lierre_rgba_t fill = {0, 0, 0, 255}, bg = {255, 255, 255, 255};
    lierre_writer_t *writer;
    uint8_t *data;

    data = (uint8_t *)malloc(5000);
    TEST_ASSERT_NOT_NULL(data);
    memset(data, 'A', 5000);
    lierre_writer_param_init(&param, data, 5000, 1, 0, ECC_HIGH, MASK_AUTO, MODE_BYTE);
    writer = lierre_writer_create(&param, &fill, &bg);
    TEST_ASSERT_NULL(writer);
    free(data);
}

void test_writer_destroy_null(void)
{
    lierre_writer_destroy(NULL);
    TEST_PASS();
}

void test_writer_write_basic(void)
{
    lierre_writer_param_t param;
    lierre_rgba_t fill = {0, 0, 0, 255}, bg = {255, 255, 255, 255};
    lierre_writer_t *writer;
    lierre_error_t err;
    uint8_t data[] = "Hello, World!";

    lierre_writer_param_init(&param, data, strlen((char *)data), 4, 2, ECC_MEDIUM, MASK_AUTO, MODE_BYTE);
    writer = lierre_writer_create(&param, &fill, &bg);
    TEST_ASSERT_NOT_NULL(writer);
    err = lierre_writer_write(writer);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    lierre_writer_destroy(writer);
}

void test_writer_write_null(void)
{
    lierre_error_t err;

    err = lierre_writer_write(NULL);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_INVALID_PARAMS, err);
}

void test_writer_write_all_ecc_levels(void)
{
    lierre_rgba_t fill = {0, 0, 0, 255}, bg = {255, 255, 255, 255};
    lierre_writer_param_t param;
    lierre_writer_t *writer;
    lierre_error_t err;
    lierre_writer_ecc_t ecc_levels[] = {ECC_LOW, ECC_MEDIUM, ECC_QUARTILE, ECC_HIGH};
    uint8_t data[] = "Test ECC";
    size_t i;

    for (i = 0; i < sizeof(ecc_levels) / sizeof(ecc_levels[0]); i++) {
        lierre_writer_param_init(&param, data, strlen((char *)data), 2, 1, ecc_levels[i], MASK_AUTO, MODE_BYTE);
        writer = lierre_writer_create(&param, &fill, &bg);
        TEST_ASSERT_NOT_NULL(writer);
        err = lierre_writer_write(writer);
        TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
        lierre_writer_destroy(writer);
    }
}

void test_writer_write_all_mask_patterns(void)
{
    lierre_rgba_t fill = {0, 0, 0, 255}, bg = {255, 255, 255, 255};
    lierre_writer_param_t param;
    lierre_writer_t *writer;
    lierre_error_t err;
    lierre_writer_mask_t masks[] = {MASK_AUTO, MASK_0, MASK_1, MASK_2, MASK_3, MASK_4, MASK_5, MASK_6, MASK_7};
    uint8_t data[] = "Test Mask";
    size_t i;

    for (i = 0; i < sizeof(masks) / sizeof(masks[0]); i++) {
        lierre_writer_param_init(&param, data, strlen((char *)data), 2, 1, ECC_LOW, masks[i], MODE_BYTE);
        writer = lierre_writer_create(&param, &fill, &bg);
        TEST_ASSERT_NOT_NULL(writer);
        err = lierre_writer_write(writer);
        TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
        lierre_writer_destroy(writer);
    }
}

void test_writer_write_various_scales(void)
{
    lierre_rgba_t fill = {0, 0, 0, 255}, bg = {255, 255, 255, 255};
    lierre_writer_param_t param;
    lierre_writer_t *writer;
    lierre_error_t err;
    uint8_t data[] = "Scale";
    size_t scale;

    for (scale = 1; scale <= 8; scale++) {
        lierre_writer_param_init(&param, data, strlen((char *)data), scale, 0, ECC_LOW, MASK_AUTO, MODE_BYTE);
        writer = lierre_writer_create(&param, &fill, &bg);
        TEST_ASSERT_NOT_NULL(writer);
        err = lierre_writer_write(writer);
        TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
        lierre_writer_destroy(writer);
    }
}

void test_writer_write_various_margins(void)
{
    lierre_rgba_t fill = {0, 0, 0, 255}, bg = {255, 255, 255, 255};
    lierre_writer_param_t param;
    lierre_writer_t *writer;
    lierre_error_t err;
    uint8_t data[] = "Margin";
    size_t margin;

    for (margin = 0; margin <= 5; margin++) {
        lierre_writer_param_init(&param, data, strlen((char *)data), 2, margin, ECC_LOW, MASK_AUTO, MODE_BYTE);
        writer = lierre_writer_create(&param, &fill, &bg);
        TEST_ASSERT_NOT_NULL(writer);
        err = lierre_writer_write(writer);
        TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
        lierre_writer_destroy(writer);
    }
}

void test_writer_write_different_colors(void)
{
    lierre_rgba_t fill = {255, 0, 0, 255}, bg = {0, 0, 255, 128};
    lierre_writer_param_t param;
    lierre_writer_t *writer;
    lierre_error_t err;
    uint8_t data[] = "Color";

    lierre_writer_param_init(&param, data, strlen((char *)data), 2, 1, ECC_LOW, MASK_AUTO, MODE_BYTE);
    writer = lierre_writer_create(&param, &fill, &bg);
    TEST_ASSERT_NOT_NULL(writer);
    err = lierre_writer_write(writer);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    lierre_writer_destroy(writer);
}

void test_writer_write_binary_data(void)
{
    lierre_rgba_t fill = {0, 0, 0, 255}, bg = {255, 255, 255, 255};
    lierre_writer_param_t param;
    lierre_writer_t *writer;
    lierre_error_t err;
    uint8_t data[] = {0x00, 0x01, 0x02, 0xFF, 0xFE, 0x80, 0x7F};

    lierre_writer_param_init(&param, data, sizeof(data), 2, 1, ECC_LOW, MASK_AUTO, MODE_BYTE);
    writer = lierre_writer_create(&param, &fill, &bg);
    TEST_ASSERT_NOT_NULL(writer);
    err = lierre_writer_write(writer);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    lierre_writer_destroy(writer);
}

void test_writer_write_max_version_1_data(void)
{
    lierre_rgba_t fill = {0, 0, 0, 255}, bg = {255, 255, 255, 255};
    lierre_writer_param_t param;
    lierre_writer_t *writer;
    lierre_qr_version_t ver;
    lierre_error_t err;
    uint8_t data[17];

    memset(data, 'A', 17);
    lierre_writer_param_init(&param, data, 17, 1, 0, ECC_LOW, MASK_AUTO, MODE_BYTE);
    ver = lierre_writer_qr_version(&param);
    TEST_ASSERT_EQUAL(QR_VERSION_1, ver);
    writer = lierre_writer_create(&param, &fill, &bg);
    TEST_ASSERT_NOT_NULL(writer);
    err = lierre_writer_write(writer);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    lierre_writer_destroy(writer);
}

void test_writer_write_version_2_data(void)
{
    lierre_rgba_t fill = {0, 0, 0, 255}, bg = {255, 255, 255, 255};
    lierre_writer_param_t param;
    lierre_writer_t *writer;
    lierre_qr_version_t ver;
    lierre_error_t err;
    uint8_t data[25];

    memset(data, 'B', 25);
    lierre_writer_param_init(&param, data, 25, 1, 0, ECC_LOW, MASK_AUTO, MODE_BYTE);
    ver = lierre_writer_qr_version(&param);
    TEST_ASSERT_EQUAL(QR_VERSION_2, ver);
    writer = lierre_writer_create(&param, &fill, &bg);
    TEST_ASSERT_NOT_NULL(writer);
    err = lierre_writer_write(writer);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    lierre_writer_destroy(writer);
}

void test_writer_write_larger_version(void)
{
    lierre_rgba_t fill = {0, 0, 0, 255}, bg = {255, 255, 255, 255};
    lierre_writer_param_t param;
    lierre_writer_t *writer;
    lierre_qr_version_t ver;
    lierre_error_t err;
    uint8_t data[500];

    memset(data, 'X', 500);
    lierre_writer_param_init(&param, data, 500, 1, 0, ECC_LOW, MASK_AUTO, MODE_BYTE);
    ver = lierre_writer_qr_version(&param);
    TEST_ASSERT_GREATER_THAN(QR_VERSION_10, ver);
    writer = lierre_writer_create(&param, &fill, &bg);
    TEST_ASSERT_NOT_NULL(writer);
    err = lierre_writer_write(writer);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    lierre_writer_destroy(writer);
}

void test_writer_write_combined_params(void)
{
    lierre_rgba_t fill = {32, 64, 128, 200}, bg = {240, 230, 220, 255};
    lierre_writer_param_t param;
    lierre_writer_t *writer;
    lierre_error_t err;
    uint8_t data[] = "Combined test with various parameters";
    lierre_writer_ecc_t ecc_levels[] = {ECC_LOW, ECC_MEDIUM, ECC_QUARTILE, ECC_HIGH};
    lierre_writer_mask_t masks[] = {MASK_0, MASK_2, MASK_4, MASK_6};
    size_t scales[] = {1, 2, 4};
    size_t margins[] = {0, 1, 4};
    size_t ei, mi, si, mgi;

    for (ei = 0; ei < sizeof(ecc_levels) / sizeof(ecc_levels[0]); ei++) {
        for (mi = 0; mi < sizeof(masks) / sizeof(masks[0]); mi++) {
            for (si = 0; si < sizeof(scales) / sizeof(scales[0]); si++) {
                for (mgi = 0; mgi < sizeof(margins) / sizeof(margins[0]); mgi++) {
                    lierre_writer_param_init(&param, data, strlen((char *)data), scales[si], margins[mgi],
                                             ecc_levels[ei], masks[mi], MODE_BYTE);
                    writer = lierre_writer_create(&param, &fill, &bg);
                    TEST_ASSERT_NOT_NULL(writer);
                    err = lierre_writer_write(writer);
                    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
                    lierre_writer_destroy(writer);
                }
            }
        }
    }
}

void test_writer_version_boundary_byte_mode(void)
{
    const uint8_t *rgba_data;
    lierre_rgba_t fill = {0, 0, 0, 255}, bg = {255, 255, 255, 255};
    lierre_writer_param_t param;
    lierre_writer_t *writer;
    lierre_reso_t res;
    lierre_error_t err;
    uint8_t *data;
    size_t len, x, y, margin, scale, expected_white, offset;

    scale = 4;
    margin = 2;

    for (len = 75; len <= 85; len++) {
        data = (uint8_t *)malloc(len);
        TEST_ASSERT_NOT_NULL(data);
        memset(data, 'A', len);

        lierre_writer_param_init(&param, data, len, scale, margin, ECC_LOW, MASK_AUTO, MODE_BYTE);

        TEST_ASSERT_TRUE(lierre_writer_get_res(&param, &res));
        TEST_ASSERT_GREATER_THAN(0, res.width);
        TEST_ASSERT_EQUAL(res.width, res.height);

        writer = lierre_writer_create(&param, &fill, &bg);
        TEST_ASSERT_NOT_NULL(writer);

        err = lierre_writer_write(writer);
        TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

        rgba_data = lierre_writer_get_rgba_data(writer);
        TEST_ASSERT_NOT_NULL(rgba_data);

        expected_white = margin * scale;
        for (y = 0; y < expected_white; y++) {
            for (x = 0; x < res.width; x++) {
                offset = (y * res.width + x) * 4;
                TEST_ASSERT_EQUAL_MESSAGE(255, rgba_data[offset + 0], "Top margin R should be white");
                TEST_ASSERT_EQUAL_MESSAGE(255, rgba_data[offset + 1], "Top margin G should be white");
                TEST_ASSERT_EQUAL_MESSAGE(255, rgba_data[offset + 2], "Top margin B should be white");
            }
        }

        for (y = res.height - expected_white; y < res.height; y++) {
            for (x = 0; x < res.width; x++) {
                offset = (y * res.width + x) * 4;
                TEST_ASSERT_EQUAL_MESSAGE(255, rgba_data[offset + 0], "Bottom margin R should be white");
                TEST_ASSERT_EQUAL_MESSAGE(255, rgba_data[offset + 1], "Bottom margin G should be white");
                TEST_ASSERT_EQUAL_MESSAGE(255, rgba_data[offset + 2], "Bottom margin B should be white");
            }
        }

        for (y = 0; y < res.height; y++) {
            for (x = 0; x < expected_white; x++) {
                offset = (y * res.width + x) * 4;
                TEST_ASSERT_EQUAL_MESSAGE(255, rgba_data[offset + 0], "Left margin R should be white");
                TEST_ASSERT_EQUAL_MESSAGE(255, rgba_data[offset + 1], "Left margin G should be white");
                TEST_ASSERT_EQUAL_MESSAGE(255, rgba_data[offset + 2], "Left margin B should be white");
            }
        }

        for (y = 0; y < res.height; y++) {
            for (x = res.width - expected_white; x < res.width; x++) {
                offset = (y * res.width + x) * 4;
                TEST_ASSERT_EQUAL_MESSAGE(255, rgba_data[offset + 0], "Right margin R should be white");
                TEST_ASSERT_EQUAL_MESSAGE(255, rgba_data[offset + 1], "Right margin G should be white");
                TEST_ASSERT_EQUAL_MESSAGE(255, rgba_data[offset + 2], "Right margin B should be white");
            }
        }

        lierre_writer_destroy(writer);
        free(data);
    }
}

void test_writer_version_boundary_all_ecc(void)
{
    lierre_rgba_t fill = {0, 0, 0, 255}, bg = {255, 255, 255, 255};
    lierre_writer_param_t param;
    lierre_writer_t *writer;
    lierre_reso_t res;
    lierre_error_t err;
    lierre_writer_ecc_t ecc_levels[] = {ECC_LOW, ECC_MEDIUM, ECC_QUARTILE, ECC_HIGH};
    uint8_t *data;
    size_t len, ei;

    for (ei = 0; ei < sizeof(ecc_levels) / sizeof(ecc_levels[0]); ei++) {
        for (len = 10; len <= 100; len++) {
            data = (uint8_t *)malloc(len);
            TEST_ASSERT_NOT_NULL(data);
            memset(data, 'X', len);

            lierre_writer_param_init(&param, data, len, 4, 2, ecc_levels[ei], MASK_AUTO, MODE_BYTE);

            if (!lierre_writer_get_res(&param, &res)) {
                free(data);
                continue;
            }

            writer = lierre_writer_create(&param, &fill, &bg);
            if (writer == NULL) {
                free(data);
                continue;
            }

            err = lierre_writer_write(writer);
            TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

            lierre_writer_destroy(writer);
            free(data);
        }
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_writer_param_init_basic);
    RUN_TEST(test_writer_param_init_null_param);
    RUN_TEST(test_writer_param_init_null_data);
    RUN_TEST(test_writer_param_init_zero_data_size);
    RUN_TEST(test_writer_param_init_zero_scale);
    RUN_TEST(test_writer_param_init_all_ecc_levels);
    RUN_TEST(test_writer_param_init_all_mask_patterns);
    RUN_TEST(test_writer_param_init_various_scales);
    RUN_TEST(test_writer_param_init_various_margins);

    RUN_TEST(test_writer_qr_version_small_data);
    RUN_TEST(test_writer_qr_version_null_param);
    RUN_TEST(test_writer_qr_version_null_data);
    RUN_TEST(test_writer_qr_version_zero_data_size);
    RUN_TEST(test_writer_qr_version_data_too_large);
    RUN_TEST(test_writer_qr_version_all_ecc_levels);
    RUN_TEST(test_writer_qr_version_invalid_ecc_fallback);

    RUN_TEST(test_writer_get_res_basic);
    RUN_TEST(test_writer_get_res_null_param);
    RUN_TEST(test_writer_get_res_null_res);
    RUN_TEST(test_writer_get_res_data_too_large);
    RUN_TEST(test_writer_get_res_scale_effect);
    RUN_TEST(test_writer_get_res_margin_effect);

    RUN_TEST(test_writer_get_res_width_basic);
    RUN_TEST(test_writer_get_res_width_null);
    RUN_TEST(test_writer_get_res_width_invalid);

    RUN_TEST(test_writer_get_res_height_basic);
    RUN_TEST(test_writer_get_res_height_null);
    RUN_TEST(test_writer_get_res_height_matches_width);

    RUN_TEST(test_writer_create_basic);
    RUN_TEST(test_writer_create_null_param);
    RUN_TEST(test_writer_create_null_fill);
    RUN_TEST(test_writer_create_null_bg);
    RUN_TEST(test_writer_create_data_too_large);

    RUN_TEST(test_writer_destroy_null);

    RUN_TEST(test_writer_write_basic);
    RUN_TEST(test_writer_write_null);
    RUN_TEST(test_writer_write_all_ecc_levels);
    RUN_TEST(test_writer_write_all_mask_patterns);
    RUN_TEST(test_writer_write_various_scales);
    RUN_TEST(test_writer_write_various_margins);
    RUN_TEST(test_writer_write_different_colors);
    RUN_TEST(test_writer_write_binary_data);
    RUN_TEST(test_writer_write_max_version_1_data);
    RUN_TEST(test_writer_write_version_2_data);
    RUN_TEST(test_writer_write_larger_version);
    RUN_TEST(test_writer_write_combined_params);
    RUN_TEST(test_writer_version_boundary_byte_mode);
    RUN_TEST(test_writer_version_boundary_all_ecc);

    return UNITY_END();
}

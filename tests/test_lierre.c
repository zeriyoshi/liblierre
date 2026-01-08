/*
 * liblierre - test_lierre.c
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

#include <unity.h>

void setUp(void)
{
}

void tearDown(void)
{
}

static const size_t version_data_capacity_m[41] = {
    0,    /* v0: invalid */
    16,   /* v1 */
    28,   /* v2 */
    44,   /* v3 */
    64,   /* v4 */
    86,   /* v5 */
    108,  /* v6 */
    124,  /* v7 */
    154,  /* v8 */
    182,  /* v9 */
    216,  /* v10 */
    254,  /* v11 */
    290,  /* v12 */
    334,  /* v13 */
    365,  /* v14 */
    415,  /* v15 */
    453,  /* v16 */
    507,  /* v17 */
    563,  /* v18 */
    627,  /* v19 */
    669,  /* v20 */
    714,  /* v21 */
    782,  /* v22 */
    860,  /* v23 */
    914,  /* v24 */
    1000, /* v25 */
    1062, /* v26 */
    1128, /* v27 */
    1193, /* v28 */
    1267, /* v29 */
    1373, /* v30 */
    1455, /* v31 */
    1541, /* v32 */
    1631, /* v33 */
    1725, /* v34 */
    1812, /* v35 */
    1914, /* v36 */
    1992, /* v37 */
    2102, /* v38 */
    2216, /* v39 */
    2334  /* v40 */
};

static inline lierre_rgb_data_t *convert_rgba_to_rgb(const uint8_t *rgba_data, size_t width, size_t height)
{
    size_t rgb_size, i, j;
    uint8_t *rgb_data;
    lierre_rgb_data_t *result;

    rgb_size = width * height * 3;
    rgb_data = (uint8_t *)malloc(rgb_size);
    if (!rgb_data) {
        return NULL;
    }

    for (i = 0, j = 0; i < width * height * 4; i += 4, j += 3) {
        rgb_data[j] = rgba_data[i];
        rgb_data[j + 1] = rgba_data[i + 1];
        rgb_data[j + 2] = rgba_data[i + 2];
    }

    result = lierre_rgb_create(rgb_data, rgb_size, width, height);
    free(rgb_data);

    return result;
}

static inline int test_encode_decode_version_impl(int version, bool use_mt)
{
    uint8_t *test_data;
    size_t data_size, i;
    lierre_writer_param_t writer_param;
    lierre_writer_t *writer;
    lierre_reader_param_t reader_param;
    lierre_reader_t *reader;
    lierre_reader_result_t *reader_result;
    lierre_rgb_data_t *rgb_data;
    lierre_reso_t res;
    lierre_rgba_t fill_color, bg_color;
    lierre_error_t err;
    uint32_t num_codes, scale;
    const uint8_t *decoded_data;
    size_t decoded_size;
    int result;

    if (version < 1 || version > 40) {
        return 0;
    }

    if (version == 1) {
        data_size = 1;
    } else {
        data_size = version_data_capacity_m[version - 1] + 1;
    }

    if (data_size > version_data_capacity_m[version]) {
        data_size = version_data_capacity_m[version];
    }

    test_data = (uint8_t *)malloc(data_size);
    if (!test_data) {
        return 0;
    }

    for (i = 0; i < data_size; i++) {
        test_data[i] = (uint8_t)((i * 7 + version) & 0xFF);
    }

    if (version <= 5) {
        scale = 4;
    } else if (version <= 10) {
        scale = 5;
    } else if (version <= 15) {
        scale = 6;
    } else if (version <= 20) {
        scale = 7;
    } else if (version <= 25) {
        scale = 9;
    } else if (version <= 30) {
        scale = 10;
    } else if (version <= 35) {
        scale = 11;
    } else if (version == 39) {
        scale = 14;
    } else {
        scale = 12;
    }

    fill_color.r = 0;
    fill_color.g = 0;
    fill_color.b = 0;
    fill_color.a = 255;
    bg_color.r = 255;
    bg_color.g = 255;
    bg_color.b = 255;
    bg_color.a = 255;

    result = 0;

    err = lierre_writer_param_init(&writer_param, test_data, data_size, scale, 2, ECC_MEDIUM, MASK_AUTO, MODE_BYTE);
    if (err != LIERRE_ERROR_SUCCESS) {
        fprintf(stderr, "v%d: writer_param_init failed: %d\n", version, err);
        goto cleanup_data;
    }

    if (!lierre_writer_get_res(&writer_param, &res)) {
        fprintf(stderr, "v%d: writer_get_res failed\n", version);
        goto cleanup_data;
    }

    writer = lierre_writer_create(&writer_param, &fill_color, &bg_color);
    if (!writer) {
        goto cleanup_data;
    }

    err = lierre_writer_write(writer);
    if (err != LIERRE_ERROR_SUCCESS) {
        fprintf(stderr, "v%d: writer_write failed: %d\n", version, err);
        goto cleanup_writer;
    }

    rgb_data = convert_rgba_to_rgb(lierre_writer_get_rgba_data(writer), res.width, res.height);
    if (!rgb_data) {
        fprintf(stderr, "v%d: convert_rgba_to_rgb failed\n", version);
        goto cleanup_writer;
    }

    err = lierre_reader_param_init(&reader_param);
    if (err != LIERRE_ERROR_SUCCESS) {
        fprintf(stderr, "v%d: reader_param_init failed: %d\n", version, err);
        goto cleanup_rgb;
    }

    if (use_mt) {
        lierre_reader_param_set_flag(&reader_param, LIERRE_READER_STRATEGY_MT);
    }

    reader = lierre_reader_create(&reader_param);
    if (!reader) {
        fprintf(stderr, "v%d: reader_create failed\n", version);
        goto cleanup_rgb;
    }

    lierre_reader_set_data(reader, rgb_data);

    err = lierre_reader_read(reader, &reader_result);
    if (err != LIERRE_ERROR_SUCCESS) {
        fprintf(stderr, "v%d: reader_read failed: %d\n", version, err);
        goto cleanup_reader;
    }

    num_codes = lierre_reader_result_get_num_qr_codes(reader_result);
    if (num_codes != 1) {
        fprintf(stderr, "v%d: num_codes=%u (expected 1)\n", version, num_codes);
        goto cleanup_result;
    }

    decoded_data = lierre_reader_result_get_qr_code_data(reader_result, 0);
    decoded_size = lierre_reader_result_get_qr_code_data_size(reader_result, 0);

    if (!decoded_data || decoded_size != data_size) {
        fprintf(stderr, "v%d: decoded_size=%zu (expected %zu)\n", version, decoded_size, data_size);
        goto cleanup_result;
    }

    if (memcmp(test_data, decoded_data, data_size) == 0) {
        result = 1;
    }

cleanup_result:
    lierre_reader_result_destroy(reader_result);
cleanup_reader:
    lierre_reader_destroy(reader);
cleanup_rgb:
    lierre_rgb_destroy(rgb_data);
cleanup_writer:
    lierre_writer_destroy(writer);
cleanup_data:
    free(test_data);

    return result;
}

void test_lierre_strerror_success(void)
{
    const char *str;

    str = lierre_strerror(SUCCESS);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING("Success", str);
}

void test_lierre_strerror_invalid_params(void)
{
    const char *str;

    str = lierre_strerror(ERROR_INVALID_PARAMS);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING("Invalid parameters", str);
}

void test_lierre_strerror_invalid_grid_size(void)
{
    const char *str;

    str = lierre_strerror(ERROR_INVALID_GRID_SIZE);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING("Invalid grid size", str);
}

void test_lierre_strerror_invalid_version(void)
{
    const char *str;

    str = lierre_strerror(ERROR_INVALID_VERSION);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING("Invalid version", str);
}

void test_lierre_strerror_format_ecc(void)
{
    const char *str;

    str = lierre_strerror(ERROR_FORMAT_ECC);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING("Format ECC error", str);
}

void test_lierre_strerror_data_ecc(void)
{
    const char *str;

    str = lierre_strerror(ERROR_DATA_ECC);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING("Data ECC error", str);
}

void test_lierre_strerror_unknown_data_type(void)
{
    const char *str;

    str = lierre_strerror(ERROR_UNKNOWN_DATA_TYPE);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING("Unknown data type", str);
}

void test_lierre_strerror_data_overflow(void)
{
    const char *str;

    str = lierre_strerror(ERROR_DATA_OVERFLOW);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING("Data overflow", str);
}

void test_lierre_strerror_data_underflow(void)
{
    const char *str;

    str = lierre_strerror(ERROR_DATA_UNDERFLOW);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING("Data underflow", str);
}

void test_lierre_strerror_size_exceeded(void)
{
    const char *str;

    str = lierre_strerror(ERROR_SIZE_EXCEEDED);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING("Size exceeded", str);
}

void test_lierre_strerror_unknown_error(void)
{
    const char *str;

    str = lierre_strerror((lierre_error_t)999);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING("Unknown error", str);
}

void test_lierre_strerror_error_max(void)
{
    const char *str;

    str = lierre_strerror(ERROR_MAX);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING("Unknown error", str);
}

void test_lierre_version_id_returns_value(void)
{
    uint32_t version;

    version = lierre_version_id();
    TEST_ASSERT_NOT_EQUAL(0, version);
}

void test_lierre_version_id_consistent(void)
{
    uint32_t v1, v2;

    v1 = lierre_version_id();
    v2 = lierre_version_id();
    TEST_ASSERT_EQUAL_UINT32(v1, v2);
}

void test_lierre_buildtime_returns_value(void)
{
    lierre_buildtime_t bt;

    bt = lierre_buildtime();
    (void)bt;
    TEST_PASS();
}

void test_lierre_buildtime_consistent(void)
{
    lierre_buildtime_t bt1, bt2;

    bt1 = lierre_buildtime();
    bt2 = lierre_buildtime();
    TEST_ASSERT_EQUAL_UINT32(bt1, bt2);
}

void test_lierre_rgb_create_basic(void)
{
    lierre_rgb_data_t *rgb;
    uint8_t data[12] = {0};

    rgb = lierre_rgb_create(data, sizeof(data), 2, 2);
    TEST_ASSERT_NOT_NULL(rgb);
    lierre_rgb_destroy(rgb);
}

void test_lierre_rgb_create_null_data(void)
{
    lierre_rgb_data_t *rgb;

    rgb = lierre_rgb_create(NULL, 12, 2, 2);
    TEST_ASSERT_NULL(rgb);
}

void test_lierre_rgb_create_zero_size(void)
{
    lierre_rgb_data_t *rgb;
    uint8_t data[12] = {0};

    rgb = lierre_rgb_create(data, 0, 2, 2);
    TEST_ASSERT_NULL(rgb);
}

void test_lierre_rgb_create_zero_width(void)
{
    lierre_rgb_data_t *rgb;
    uint8_t data[12] = {0};

    rgb = lierre_rgb_create(data, sizeof(data), 0, 2);
    TEST_ASSERT_NULL(rgb);
}

void test_lierre_rgb_create_zero_height(void)
{
    lierre_rgb_data_t *rgb;
    uint8_t data[12] = {0};

    rgb = lierre_rgb_create(data, sizeof(data), 2, 0);
    TEST_ASSERT_NULL(rgb);
}

void test_lierre_rgb_create_large_image(void)
{
    size_t w = 100, h = 100, size;
    lierre_rgb_data_t *rgb;
    uint8_t *data;

    size = w * h * 3;
    data = (uint8_t *)malloc(size);
    TEST_ASSERT_NOT_NULL(data);
    memset(data, 128, size);
    rgb = lierre_rgb_create(data, size, w, h);
    TEST_ASSERT_NOT_NULL(rgb);
    lierre_rgb_destroy(rgb);
    free(data);
}

void test_lierre_rgb_destroy_null(void)
{
    lierre_rgb_destroy(NULL);
    TEST_PASS();
}

void test_encode_decode_simple_text(void)
{
    const char *test_data = "Hello, liblierre!";
    lierre_writer_param_t writer_param;
    lierre_writer_t *writer;
    lierre_reader_param_t reader_param;
    lierre_reader_t *reader;
    lierre_reader_result_t *reader_result;
    lierre_rgb_data_t *rgb_data;
    lierre_reso_t res;
    lierre_rgba_t fill_color, bg_color;
    lierre_error_t err;
    uint32_t num_codes;
    const uint8_t *decoded_data;
    size_t decoded_size;

    err = lierre_writer_param_init(&writer_param, (uint8_t *)test_data, strlen(test_data), 4, 2, ECC_MEDIUM, MASK_AUTO,
                                   MODE_BYTE);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    TEST_ASSERT_TRUE(lierre_writer_get_res(&writer_param, &res));
    TEST_ASSERT_GREATER_THAN(0, res.width);
    TEST_ASSERT_GREATER_THAN(0, res.height);

    fill_color.r = 0;
    fill_color.g = 0;
    fill_color.b = 0;
    fill_color.a = 255;
    bg_color.r = 255;
    bg_color.g = 255;
    bg_color.b = 255;
    bg_color.a = 255;

    writer = lierre_writer_create(&writer_param, &fill_color, &bg_color);
    TEST_ASSERT_NOT_NULL(writer);

    err = lierre_writer_write(writer);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    rgb_data = convert_rgba_to_rgb(lierre_writer_get_rgba_data(writer), res.width, res.height);
    TEST_ASSERT_NOT_NULL(rgb_data);

    err = lierre_reader_param_init(&reader_param);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    reader = lierre_reader_create(&reader_param);
    TEST_ASSERT_NOT_NULL(reader);

    lierre_reader_set_data(reader, rgb_data);

    err = lierre_reader_read(reader, &reader_result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(reader_result);

    num_codes = lierre_reader_result_get_num_qr_codes(reader_result);
    TEST_ASSERT_EQUAL_UINT32(1, num_codes);

    decoded_data = lierre_reader_result_get_qr_code_data(reader_result, 0);
    decoded_size = lierre_reader_result_get_qr_code_data_size(reader_result, 0);

    TEST_ASSERT_NOT_NULL(decoded_data);
    TEST_ASSERT_EQUAL(strlen(test_data), decoded_size);
    TEST_ASSERT_EQUAL_STRING(test_data, (const char *)decoded_data);

    lierre_reader_result_destroy(reader_result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb_data);
    lierre_writer_destroy(writer);
}

void test_encode_decode_binary_data(void)
{
    uint8_t test_data[64];
    size_t i;
    lierre_writer_param_t writer_param;
    lierre_writer_t *writer;
    lierre_reader_param_t reader_param;
    lierre_reader_t *reader;
    lierre_reader_result_t *reader_result;
    lierre_rgb_data_t *rgb_data;
    lierre_reso_t res;
    lierre_rgba_t fill_color, bg_color;
    lierre_error_t err;
    uint32_t num_codes;
    const uint8_t *decoded_data;
    size_t decoded_size;

    for (i = 0; i < sizeof(test_data); i++) {
        test_data[i] = (uint8_t)(i * 7 + 13);
    }

    err = lierre_writer_param_init(&writer_param, test_data, sizeof(test_data), 5, 3, ECC_LOW, MASK_AUTO, MODE_BYTE);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    TEST_ASSERT_TRUE(lierre_writer_get_res(&writer_param, &res));

    fill_color.r = 0;
    fill_color.g = 0;
    fill_color.b = 0;
    fill_color.a = 255;
    bg_color.r = 255;
    bg_color.g = 255;
    bg_color.b = 255;
    bg_color.a = 255;

    writer = lierre_writer_create(&writer_param, &fill_color, &bg_color);
    TEST_ASSERT_NOT_NULL(writer);

    err = lierre_writer_write(writer);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    rgb_data = convert_rgba_to_rgb(lierre_writer_get_rgba_data(writer), res.width, res.height);
    TEST_ASSERT_NOT_NULL(rgb_data);

    err = lierre_reader_param_init(&reader_param);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    reader = lierre_reader_create(&reader_param);
    TEST_ASSERT_NOT_NULL(reader);

    lierre_reader_set_data(reader, rgb_data);

    err = lierre_reader_read(reader, &reader_result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(reader_result);

    num_codes = lierre_reader_result_get_num_qr_codes(reader_result);
    TEST_ASSERT_EQUAL_UINT32(1, num_codes);

    decoded_data = lierre_reader_result_get_qr_code_data(reader_result, 0);
    decoded_size = lierre_reader_result_get_qr_code_data_size(reader_result, 0);

    TEST_ASSERT_NOT_NULL(decoded_data);
    TEST_ASSERT_EQUAL(sizeof(test_data), decoded_size);
    TEST_ASSERT_EQUAL_MEMORY(test_data, decoded_data, sizeof(test_data));

    lierre_reader_result_destroy(reader_result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb_data);
    lierre_writer_destroy(writer);
}

void test_encode_decode_high_ecc(void)
{
    const char *test_data = "QR code with HIGH error correction";
    lierre_writer_param_t writer_param;
    lierre_writer_t *writer;
    lierre_reader_param_t reader_param;
    lierre_reader_t *reader;
    lierre_reader_result_t *reader_result;
    lierre_rgb_data_t *rgb_data;
    lierre_reso_t res;
    lierre_rgba_t fill_color, bg_color;
    lierre_error_t err;
    uint32_t num_codes;
    const uint8_t *decoded_data;
    size_t decoded_size;

    err = lierre_writer_param_init(&writer_param, (uint8_t *)test_data, strlen(test_data), 6, 4, ECC_HIGH, MASK_AUTO,
                                   MODE_BYTE);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    TEST_ASSERT_TRUE(lierre_writer_get_res(&writer_param, &res));

    fill_color.r = 0;
    fill_color.g = 0;
    fill_color.b = 0;
    fill_color.a = 255;
    bg_color.r = 255;
    bg_color.g = 255;
    bg_color.b = 255;
    bg_color.a = 255;

    writer = lierre_writer_create(&writer_param, &fill_color, &bg_color);
    TEST_ASSERT_NOT_NULL(writer);

    err = lierre_writer_write(writer);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    rgb_data = convert_rgba_to_rgb(lierre_writer_get_rgba_data(writer), res.width, res.height);
    TEST_ASSERT_NOT_NULL(rgb_data);

    err = lierre_reader_param_init(&reader_param);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    reader = lierre_reader_create(&reader_param);
    TEST_ASSERT_NOT_NULL(reader);

    lierre_reader_set_data(reader, rgb_data);

    err = lierre_reader_read(reader, &reader_result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(reader_result);

    num_codes = lierre_reader_result_get_num_qr_codes(reader_result);
    TEST_ASSERT_EQUAL_UINT32(1, num_codes);

    decoded_data = lierre_reader_result_get_qr_code_data(reader_result, 0);
    decoded_size = lierre_reader_result_get_qr_code_data_size(reader_result, 0);

    TEST_ASSERT_NOT_NULL(decoded_data);
    TEST_ASSERT_EQUAL(strlen(test_data), decoded_size);
    TEST_ASSERT_EQUAL_STRING(test_data, (const char *)decoded_data);

    lierre_reader_result_destroy(reader_result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb_data);
    lierre_writer_destroy(writer);
}

void test_encode_decode_large_data(void)
{
    uint8_t test_data[256];
    size_t i;
    lierre_writer_param_t writer_param;
    lierre_writer_t *writer;
    lierre_reader_param_t reader_param;
    lierre_reader_t *reader;
    lierre_reader_result_t *reader_result;
    lierre_rgb_data_t *rgb_data;
    lierre_reso_t res;
    lierre_rgba_t fill_color, bg_color;
    lierre_error_t err;
    uint32_t num_codes;
    const uint8_t *decoded_data;
    size_t decoded_size;

    for (i = 0; i < sizeof(test_data); i++) {
        test_data[i] = (uint8_t)i;
    }

    err = lierre_writer_param_init(&writer_param, test_data, sizeof(test_data), 4, 2, ECC_LOW, MASK_AUTO, MODE_BYTE);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    TEST_ASSERT_TRUE(lierre_writer_get_res(&writer_param, &res));

    fill_color.r = 0;
    fill_color.g = 0;
    fill_color.b = 0;
    fill_color.a = 255;
    bg_color.r = 255;
    bg_color.g = 255;
    bg_color.b = 255;
    bg_color.a = 255;

    writer = lierre_writer_create(&writer_param, &fill_color, &bg_color);
    TEST_ASSERT_NOT_NULL(writer);

    err = lierre_writer_write(writer);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    rgb_data = convert_rgba_to_rgb(lierre_writer_get_rgba_data(writer), res.width, res.height);
    TEST_ASSERT_NOT_NULL(rgb_data);

    err = lierre_reader_param_init(&reader_param);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    reader = lierre_reader_create(&reader_param);
    TEST_ASSERT_NOT_NULL(reader);

    lierre_reader_set_data(reader, rgb_data);

    err = lierre_reader_read(reader, &reader_result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(reader_result);

    num_codes = lierre_reader_result_get_num_qr_codes(reader_result);
    TEST_ASSERT_EQUAL_UINT32(1, num_codes);

    decoded_data = lierre_reader_result_get_qr_code_data(reader_result, 0);
    decoded_size = lierre_reader_result_get_qr_code_data_size(reader_result, 0);

    TEST_ASSERT_NOT_NULL(decoded_data);
    TEST_ASSERT_EQUAL(sizeof(test_data), decoded_size);
    TEST_ASSERT_EQUAL_MEMORY(test_data, decoded_data, sizeof(test_data));

    lierre_reader_result_destroy(reader_result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb_data);
    lierre_writer_destroy(writer);
}

static void test_encode_decode_multithread(void)
{
    uint8_t test_data[200];
    size_t i;
    lierre_writer_param_t writer_param;
    lierre_writer_t *writer;
    lierre_reader_param_t reader_param;
    lierre_reader_t *reader;
    lierre_reader_result_t *reader_result;
    lierre_rgb_data_t *rgb_data;
    lierre_reso_t res;
    lierre_rgba_t fill_color, bg_color;
    lierre_error_t err;
    uint32_t num_codes;
    const uint8_t *decoded_data;
    size_t decoded_size;

    for (i = 0; i < sizeof(test_data); i++) {
        test_data[i] = (uint8_t)(i * 3 + 7);
    }

    err = lierre_writer_param_init(&writer_param, test_data, sizeof(test_data), 6, 2, ECC_MEDIUM, MASK_AUTO, MODE_BYTE);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    TEST_ASSERT_TRUE(lierre_writer_get_res(&writer_param, &res));

    fill_color.r = 0;
    fill_color.g = 0;
    fill_color.b = 0;
    fill_color.a = 255;
    bg_color.r = 255;
    bg_color.g = 255;
    bg_color.b = 255;
    bg_color.a = 255;

    writer = lierre_writer_create(&writer_param, &fill_color, &bg_color);
    TEST_ASSERT_NOT_NULL(writer);

    err = lierre_writer_write(writer);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    rgb_data = convert_rgba_to_rgb(lierre_writer_get_rgba_data(writer), res.width, res.height);
    TEST_ASSERT_NOT_NULL(rgb_data);

    err = lierre_reader_param_init(&reader_param);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    lierre_reader_param_set_flag(&reader_param, LIERRE_READER_STRATEGY_MT);

    reader = lierre_reader_create(&reader_param);
    TEST_ASSERT_NOT_NULL(reader);

    lierre_reader_set_data(reader, rgb_data);

    err = lierre_reader_read(reader, &reader_result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(reader_result);

    num_codes = lierre_reader_result_get_num_qr_codes(reader_result);
    TEST_ASSERT_EQUAL_UINT32(1, num_codes);

    decoded_data = lierre_reader_result_get_qr_code_data(reader_result, 0);
    decoded_size = lierre_reader_result_get_qr_code_data_size(reader_result, 0);

    TEST_ASSERT_NOT_NULL(decoded_data);
    TEST_ASSERT_EQUAL(sizeof(test_data), decoded_size);
    TEST_ASSERT_EQUAL_MEMORY(test_data, decoded_data, sizeof(test_data));

    lierre_reader_result_destroy(reader_result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb_data);
    lierre_writer_destroy(writer);
}

static void test_encode_decode_custom_colors(void)
{
    const char *test_data = "Custom color test";
    lierre_writer_param_t writer_param;
    lierre_writer_t *writer;
    lierre_reader_param_t reader_param;
    lierre_reader_t *reader;
    lierre_reader_result_t *reader_result;
    lierre_rgb_data_t *rgb_data;
    lierre_reso_t res;
    lierre_rgba_t fill_color, bg_color;
    lierre_error_t err;
    uint32_t num_codes;
    const uint8_t *decoded_data, *rgba_data;
    size_t decoded_size;

    err = lierre_writer_param_init(&writer_param, (uint8_t *)test_data, strlen(test_data), 5, 2, ECC_MEDIUM, MASK_AUTO,
                                   MODE_BYTE);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    TEST_ASSERT_TRUE(lierre_writer_get_res(&writer_param, &res));

    fill_color.r = 0;
    fill_color.g = 0;
    fill_color.b = 128;
    fill_color.a = 255;
    bg_color.r = 255;
    bg_color.g = 255;
    bg_color.b = 200;
    bg_color.a = 255;

    writer = lierre_writer_create(&writer_param, &fill_color, &bg_color);
    TEST_ASSERT_NOT_NULL(writer);

    err = lierre_writer_write(writer);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    rgba_data = lierre_writer_get_rgba_data(writer);
    TEST_ASSERT_NOT_NULL(rgba_data);
    TEST_ASSERT_GREATER_THAN(0, lierre_writer_get_rgba_data_size(writer));

    rgb_data = convert_rgba_to_rgb(rgba_data, res.width, res.height);
    TEST_ASSERT_NOT_NULL(rgb_data);

    err = lierre_reader_param_init(&reader_param);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    reader = lierre_reader_create(&reader_param);
    TEST_ASSERT_NOT_NULL(reader);

    lierre_reader_set_data(reader, rgb_data);

    err = lierre_reader_read(reader, &reader_result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(reader_result);

    num_codes = lierre_reader_result_get_num_qr_codes(reader_result);
    TEST_ASSERT_EQUAL_UINT32(1, num_codes);

    decoded_data = lierre_reader_result_get_qr_code_data(reader_result, 0);
    decoded_size = lierre_reader_result_get_qr_code_data_size(reader_result, 0);

    TEST_ASSERT_NOT_NULL(decoded_data);
    TEST_ASSERT_EQUAL(strlen(test_data), decoded_size);
    TEST_ASSERT_EQUAL_STRING(test_data, (const char *)decoded_data);

    lierre_reader_result_destroy(reader_result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb_data);
    lierre_writer_destroy(writer);
}

static void test_encode_decode_inverted_colors(void)
{
    const char *test_data = "Inverted color test";
    lierre_writer_param_t writer_param;
    lierre_writer_t *writer;
    lierre_reader_param_t reader_param;
    lierre_reader_t *reader;
    lierre_reader_result_t *reader_result;
    lierre_rgb_data_t *rgb_data;
    lierre_reso_t res;
    lierre_rgba_t fill_color, bg_color;
    lierre_error_t err;
    uint32_t num_codes;

    err = lierre_writer_param_init(&writer_param, (uint8_t *)test_data, strlen(test_data), 5, 2, ECC_MEDIUM, MASK_AUTO,
                                   MODE_BYTE);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    TEST_ASSERT_TRUE(lierre_writer_get_res(&writer_param, &res));

    fill_color.r = 255;
    fill_color.g = 255;
    fill_color.b = 255;
    fill_color.a = 255;
    bg_color.r = 0;
    bg_color.g = 0;
    bg_color.b = 0;
    bg_color.a = 255;

    writer = lierre_writer_create(&writer_param, &fill_color, &bg_color);
    TEST_ASSERT_NOT_NULL(writer);

    err = lierre_writer_write(writer);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    rgb_data = convert_rgba_to_rgb(lierre_writer_get_rgba_data(writer), res.width, res.height);
    TEST_ASSERT_NOT_NULL(rgb_data);

    err = lierre_reader_param_init(&reader_param);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    reader = lierre_reader_create(&reader_param);
    TEST_ASSERT_NOT_NULL(reader);

    lierre_reader_set_data(reader, rgb_data);

    err = lierre_reader_read(reader, &reader_result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(reader_result);

    num_codes = lierre_reader_result_get_num_qr_codes(reader_result);
    TEST_ASSERT_EQUAL_UINT32(0, num_codes);

    lierre_reader_result_destroy(reader_result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb_data);
    lierre_writer_destroy(writer);
}

static void test_encode_decode_all_versions(void)
{
    int version, failed_count;

    failed_count = 0;
    for (version = 1; version <= 40; version++) {
        if (!test_encode_decode_version_impl(version, false)) {
            fprintf(stderr, "Version %d failed (single-thread)\n", version);
            failed_count++;
        }
    }
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, failed_count, "Some versions failed in single-thread mode");
}

static void test_encode_decode_all_versions_mt(void)
{
    int version, failed_count;

    failed_count = 0;
    for (version = 1; version <= 40; version++) {
        if (!test_encode_decode_version_impl(version, true)) {
            fprintf(stderr, "Version %d failed (multi-thread)\n", version);
            failed_count++;
        }
    }
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, failed_count, "Some versions failed in multi-thread mode");
}

static void test_encode_decode_numeric_mode_simple(void)
{
    const char *test_data = "0123456789";
    const uint8_t *decoded_data;
    lierre_writer_param_t writer_param;
    lierre_writer_t *writer;
    lierre_reader_param_t reader_param;
    lierre_reader_t *reader;
    lierre_reader_result_t *reader_result;
    lierre_rgb_data_t *rgb_data;
    lierre_reso_t res;
    lierre_rgba_t fill_color, bg_color;
    lierre_error_t err;
    uint32_t num_codes;
    size_t decoded_size;

    err = lierre_writer_param_init(&writer_param, (uint8_t *)test_data, strlen(test_data), 4, 2, ECC_MEDIUM, MASK_AUTO,
                                   MODE_NUMERIC);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    TEST_ASSERT_TRUE(lierre_writer_get_res(&writer_param, &res));

    fill_color.r = 0;
    fill_color.g = 0;
    fill_color.b = 0;
    fill_color.a = 255;
    bg_color.r = 255;
    bg_color.g = 255;
    bg_color.b = 255;
    bg_color.a = 255;

    writer = lierre_writer_create(&writer_param, &fill_color, &bg_color);
    TEST_ASSERT_NOT_NULL(writer);

    err = lierre_writer_write(writer);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    rgb_data = convert_rgba_to_rgb(lierre_writer_get_rgba_data(writer), res.width, res.height);
    TEST_ASSERT_NOT_NULL(rgb_data);

    err = lierre_reader_param_init(&reader_param);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    reader = lierre_reader_create(&reader_param);
    TEST_ASSERT_NOT_NULL(reader);

    lierre_reader_set_data(reader, rgb_data);

    err = lierre_reader_read(reader, &reader_result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(reader_result);

    num_codes = lierre_reader_result_get_num_qr_codes(reader_result);
    TEST_ASSERT_EQUAL_UINT32(1, num_codes);

    decoded_data = lierre_reader_result_get_qr_code_data(reader_result, 0);
    decoded_size = lierre_reader_result_get_qr_code_data_size(reader_result, 0);

    TEST_ASSERT_NOT_NULL(decoded_data);
    TEST_ASSERT_EQUAL(strlen(test_data), decoded_size);
    TEST_ASSERT_EQUAL_STRING(test_data, (const char *)decoded_data);

    lierre_reader_result_destroy(reader_result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb_data);
    lierre_writer_destroy(writer);
}

static void test_encode_decode_numeric_mode_long(void)
{
    const char *test_data = "12345678901234567890123456789012345678901234567890";
    const uint8_t *decoded_data;
    lierre_writer_param_t writer_param;
    lierre_writer_t *writer;
    lierre_reader_param_t reader_param;
    lierre_reader_t *reader;
    lierre_reader_result_t *reader_result;
    lierre_rgb_data_t *rgb_data;
    lierre_reso_t res;
    lierre_rgba_t fill_color, bg_color;
    lierre_error_t err;
    uint32_t num_codes;
    size_t decoded_size;

    err = lierre_writer_param_init(&writer_param, (uint8_t *)test_data, strlen(test_data), 4, 2, ECC_LOW, MASK_AUTO,
                                   MODE_NUMERIC);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    TEST_ASSERT_TRUE(lierre_writer_get_res(&writer_param, &res));

    fill_color.r = 0;
    fill_color.g = 0;
    fill_color.b = 0;
    fill_color.a = 255;
    bg_color.r = 255;
    bg_color.g = 255;
    bg_color.b = 255;
    bg_color.a = 255;

    writer = lierre_writer_create(&writer_param, &fill_color, &bg_color);
    TEST_ASSERT_NOT_NULL(writer);

    err = lierre_writer_write(writer);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    rgb_data = convert_rgba_to_rgb(lierre_writer_get_rgba_data(writer), res.width, res.height);
    TEST_ASSERT_NOT_NULL(rgb_data);

    err = lierre_reader_param_init(&reader_param);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    reader = lierre_reader_create(&reader_param);
    TEST_ASSERT_NOT_NULL(reader);

    lierre_reader_set_data(reader, rgb_data);

    err = lierre_reader_read(reader, &reader_result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(reader_result);

    num_codes = lierre_reader_result_get_num_qr_codes(reader_result);
    TEST_ASSERT_EQUAL_UINT32(1, num_codes);

    decoded_data = lierre_reader_result_get_qr_code_data(reader_result, 0);
    decoded_size = lierre_reader_result_get_qr_code_data_size(reader_result, 0);

    TEST_ASSERT_NOT_NULL(decoded_data);
    TEST_ASSERT_EQUAL(strlen(test_data), decoded_size);
    TEST_ASSERT_EQUAL_STRING(test_data, (const char *)decoded_data);

    lierre_reader_result_destroy(reader_result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb_data);
    lierre_writer_destroy(writer);
}

static void test_encode_decode_alphanumeric_mode_simple(void)
{
    const char *test_data = "HELLO WORLD";
    const uint8_t *decoded_data;
    lierre_writer_param_t writer_param;
    lierre_writer_t *writer;
    lierre_reader_param_t reader_param;
    lierre_reader_t *reader;
    lierre_reader_result_t *reader_result;
    lierre_rgb_data_t *rgb_data;
    lierre_reso_t res;
    lierre_rgba_t fill_color, bg_color;
    lierre_error_t err;
    uint32_t num_codes;
    size_t decoded_size;

    err = lierre_writer_param_init(&writer_param, (uint8_t *)test_data, strlen(test_data), 4, 2, ECC_MEDIUM, MASK_AUTO,
                                   MODE_ALPHANUMERIC);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    TEST_ASSERT_TRUE(lierre_writer_get_res(&writer_param, &res));

    fill_color.r = 0;
    fill_color.g = 0;
    fill_color.b = 0;
    fill_color.a = 255;
    bg_color.r = 255;
    bg_color.g = 255;
    bg_color.b = 255;
    bg_color.a = 255;

    writer = lierre_writer_create(&writer_param, &fill_color, &bg_color);
    TEST_ASSERT_NOT_NULL(writer);

    err = lierre_writer_write(writer);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    rgb_data = convert_rgba_to_rgb(lierre_writer_get_rgba_data(writer), res.width, res.height);
    TEST_ASSERT_NOT_NULL(rgb_data);

    err = lierre_reader_param_init(&reader_param);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    reader = lierre_reader_create(&reader_param);
    TEST_ASSERT_NOT_NULL(reader);

    lierre_reader_set_data(reader, rgb_data);

    err = lierre_reader_read(reader, &reader_result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(reader_result);

    num_codes = lierre_reader_result_get_num_qr_codes(reader_result);
    TEST_ASSERT_EQUAL_UINT32(1, num_codes);

    decoded_data = lierre_reader_result_get_qr_code_data(reader_result, 0);
    decoded_size = lierre_reader_result_get_qr_code_data_size(reader_result, 0);

    TEST_ASSERT_NOT_NULL(decoded_data);
    TEST_ASSERT_EQUAL(strlen(test_data), decoded_size);
    TEST_ASSERT_EQUAL_STRING(test_data, (const char *)decoded_data);

    lierre_reader_result_destroy(reader_result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb_data);
    lierre_writer_destroy(writer);
}

static void test_encode_decode_alphanumeric_mode_with_special(void)
{
    const char *test_data = "ABC123 $%*+-./:";
    const uint8_t *decoded_data;
    lierre_writer_param_t writer_param;
    lierre_writer_t *writer;
    lierre_reader_param_t reader_param;
    lierre_reader_t *reader;
    lierre_reader_result_t *reader_result;
    lierre_rgb_data_t *rgb_data;
    lierre_reso_t res;
    lierre_rgba_t fill_color, bg_color;
    lierre_error_t err;
    uint32_t num_codes;
    size_t decoded_size;

    err = lierre_writer_param_init(&writer_param, (uint8_t *)test_data, strlen(test_data), 4, 2, ECC_LOW, MASK_AUTO,
                                   MODE_ALPHANUMERIC);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    TEST_ASSERT_TRUE(lierre_writer_get_res(&writer_param, &res));

    fill_color.r = 0;
    fill_color.g = 0;
    fill_color.b = 0;
    fill_color.a = 255;
    bg_color.r = 255;
    bg_color.g = 255;
    bg_color.b = 255;
    bg_color.a = 255;

    writer = lierre_writer_create(&writer_param, &fill_color, &bg_color);
    TEST_ASSERT_NOT_NULL(writer);

    err = lierre_writer_write(writer);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    rgb_data = convert_rgba_to_rgb(lierre_writer_get_rgba_data(writer), res.width, res.height);
    TEST_ASSERT_NOT_NULL(rgb_data);

    err = lierre_reader_param_init(&reader_param);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    reader = lierre_reader_create(&reader_param);
    TEST_ASSERT_NOT_NULL(reader);

    lierre_reader_set_data(reader, rgb_data);

    err = lierre_reader_read(reader, &reader_result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(reader_result);

    num_codes = lierre_reader_result_get_num_qr_codes(reader_result);
    TEST_ASSERT_EQUAL_UINT32(1, num_codes);

    decoded_data = lierre_reader_result_get_qr_code_data(reader_result, 0);
    decoded_size = lierre_reader_result_get_qr_code_data_size(reader_result, 0);

    TEST_ASSERT_NOT_NULL(decoded_data);
    TEST_ASSERT_EQUAL(strlen(test_data), decoded_size);
    TEST_ASSERT_EQUAL_STRING(test_data, (const char *)decoded_data);

    lierre_reader_result_destroy(reader_result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb_data);
    lierre_writer_destroy(writer);
}

static void test_encode_decode_kanji_mode(void)
{
    const uint8_t *decoded_data;
    lierre_writer_param_t writer_param;
    lierre_writer_t *writer;
    lierre_reader_param_t reader_param;
    lierre_reader_t *reader;
    lierre_reader_result_t *reader_result;
    lierre_rgb_data_t *rgb_data;
    lierre_reso_t res;
    lierre_rgba_t fill_color, bg_color;
    lierre_error_t err;
    uint32_t num_codes;
    uint8_t test_data[] = {0x8D, 0x48, 0x93, 0xA1}; /* Shift-JIS: "工藤" (0x8D 0x48, 0x93 0xA1) */
    size_t decoded_size;

    err =
        lierre_writer_param_init(&writer_param, test_data, sizeof(test_data), 4, 2, ECC_MEDIUM, MASK_AUTO, MODE_KANJI);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    TEST_ASSERT_TRUE(lierre_writer_get_res(&writer_param, &res));

    fill_color.r = 0;
    fill_color.g = 0;
    fill_color.b = 0;
    fill_color.a = 255;
    bg_color.r = 255;
    bg_color.g = 255;
    bg_color.b = 255;
    bg_color.a = 255;

    writer = lierre_writer_create(&writer_param, &fill_color, &bg_color);
    TEST_ASSERT_NOT_NULL(writer);

    err = lierre_writer_write(writer);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    rgb_data = convert_rgba_to_rgb(lierre_writer_get_rgba_data(writer), res.width, res.height);
    TEST_ASSERT_NOT_NULL(rgb_data);

    err = lierre_reader_param_init(&reader_param);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    reader = lierre_reader_create(&reader_param);
    TEST_ASSERT_NOT_NULL(reader);

    lierre_reader_set_data(reader, rgb_data);

    err = lierre_reader_read(reader, &reader_result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(reader_result);

    num_codes = lierre_reader_result_get_num_qr_codes(reader_result);
    TEST_ASSERT_EQUAL_UINT32(1, num_codes);

    decoded_data = lierre_reader_result_get_qr_code_data(reader_result, 0);
    decoded_size = lierre_reader_result_get_qr_code_data_size(reader_result, 0);

    TEST_ASSERT_NOT_NULL(decoded_data);
    TEST_ASSERT_EQUAL(sizeof(test_data), decoded_size);
    TEST_ASSERT_EQUAL_MEMORY(test_data, decoded_data, sizeof(test_data));

    lierre_reader_result_destroy(reader_result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb_data);
    lierre_writer_destroy(writer);
}

static void test_encode_decode_kanji_mode_multiple(void)
{
    const uint8_t *decoded_data;
    lierre_writer_param_t writer_param;
    lierre_writer_t *writer;
    lierre_reader_param_t reader_param;
    lierre_reader_t *reader;
    lierre_reader_result_t *reader_result;
    lierre_rgb_data_t *rgb_data;
    lierre_reso_t res;
    lierre_rgba_t fill_color, bg_color;
    lierre_error_t err;
    uint32_t num_codes;
    uint8_t test_data[] = {0x82, 0xA0, 0x82, 0xA2, 0x82, 0xA4}; /* Shift-JIS: "あいう" */
    size_t decoded_size;

    err = lierre_writer_param_init(&writer_param, test_data, sizeof(test_data), 4, 2, ECC_LOW, MASK_AUTO, MODE_KANJI);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    TEST_ASSERT_TRUE(lierre_writer_get_res(&writer_param, &res));

    fill_color.r = 0;
    fill_color.g = 0;
    fill_color.b = 0;
    fill_color.a = 255;
    bg_color.r = 255;
    bg_color.g = 255;
    bg_color.b = 255;
    bg_color.a = 255;

    writer = lierre_writer_create(&writer_param, &fill_color, &bg_color);
    TEST_ASSERT_NOT_NULL(writer);

    err = lierre_writer_write(writer);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    rgb_data = convert_rgba_to_rgb(lierre_writer_get_rgba_data(writer), res.width, res.height);
    TEST_ASSERT_NOT_NULL(rgb_data);

    err = lierre_reader_param_init(&reader_param);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    reader = lierre_reader_create(&reader_param);
    TEST_ASSERT_NOT_NULL(reader);

    lierre_reader_set_data(reader, rgb_data);

    err = lierre_reader_read(reader, &reader_result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(reader_result);

    num_codes = lierre_reader_result_get_num_qr_codes(reader_result);
    TEST_ASSERT_EQUAL_UINT32(1, num_codes);

    decoded_data = lierre_reader_result_get_qr_code_data(reader_result, 0);
    decoded_size = lierre_reader_result_get_qr_code_data_size(reader_result, 0);

    TEST_ASSERT_NOT_NULL(decoded_data);
    TEST_ASSERT_EQUAL(sizeof(test_data), decoded_size);
    TEST_ASSERT_EQUAL_MEMORY(test_data, decoded_data, sizeof(test_data));

    lierre_reader_result_destroy(reader_result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb_data);
    lierre_writer_destroy(writer);
}

static void test_encode_decode_eci_mode(void)
{
    const char *test_str = "Hello UTF-8!";
    const uint8_t *decoded_data;
    lierre_writer_param_t writer_param;
    lierre_writer_t *writer;
    lierre_reader_param_t reader_param;
    lierre_reader_t *reader;
    lierre_reader_result_t *reader_result;
    lierre_rgb_data_t *rgb_data;
    lierre_reso_t res;
    lierre_rgba_t fill_color, bg_color;
    lierre_error_t err;
    uint32_t num_codes;
    size_t decoded_size;

    err = lierre_writer_param_init(&writer_param, (uint8_t *)test_str, strlen(test_str), 4, 2, ECC_LOW, MASK_AUTO,
                                   MODE_ECI);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    TEST_ASSERT_TRUE(lierre_writer_get_res(&writer_param, &res));

    fill_color.r = 0;
    fill_color.g = 0;
    fill_color.b = 0;
    fill_color.a = 255;
    bg_color.r = 255;
    bg_color.g = 255;
    bg_color.b = 255;
    bg_color.a = 255;

    writer = lierre_writer_create(&writer_param, &fill_color, &bg_color);
    TEST_ASSERT_NOT_NULL(writer);

    err = lierre_writer_write(writer);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    rgb_data = convert_rgba_to_rgb(lierre_writer_get_rgba_data(writer), res.width, res.height);
    TEST_ASSERT_NOT_NULL(rgb_data);

    err = lierre_reader_param_init(&reader_param);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);

    reader = lierre_reader_create(&reader_param);
    TEST_ASSERT_NOT_NULL(reader);

    lierre_reader_set_data(reader, rgb_data);

    err = lierre_reader_read(reader, &reader_result);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    TEST_ASSERT_NOT_NULL(reader_result);

    num_codes = lierre_reader_result_get_num_qr_codes(reader_result);
    TEST_ASSERT_EQUAL_UINT32(1, num_codes);

    decoded_data = lierre_reader_result_get_qr_code_data(reader_result, 0);
    decoded_size = lierre_reader_result_get_qr_code_data_size(reader_result, 0);

    TEST_ASSERT_NOT_NULL(decoded_data);
    TEST_ASSERT_EQUAL(strlen(test_str), decoded_size);
    TEST_ASSERT_EQUAL_MEMORY(test_str, decoded_data, strlen(test_str));

    lierre_reader_result_destroy(reader_result);
    lierre_reader_destroy(reader);
    lierre_rgb_destroy(rgb_data);
    lierre_writer_destroy(writer);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_lierre_strerror_success);
    RUN_TEST(test_lierre_strerror_invalid_params);
    RUN_TEST(test_lierre_strerror_invalid_grid_size);
    RUN_TEST(test_lierre_strerror_invalid_version);
    RUN_TEST(test_lierre_strerror_format_ecc);
    RUN_TEST(test_lierre_strerror_data_ecc);
    RUN_TEST(test_lierre_strerror_unknown_data_type);
    RUN_TEST(test_lierre_strerror_data_overflow);
    RUN_TEST(test_lierre_strerror_data_underflow);
    RUN_TEST(test_lierre_strerror_size_exceeded);
    RUN_TEST(test_lierre_strerror_unknown_error);
    RUN_TEST(test_lierre_strerror_error_max);

    RUN_TEST(test_lierre_version_id_returns_value);
    RUN_TEST(test_lierre_version_id_consistent);

    RUN_TEST(test_lierre_buildtime_returns_value);
    RUN_TEST(test_lierre_buildtime_consistent);

    RUN_TEST(test_lierre_rgb_create_basic);
    RUN_TEST(test_lierre_rgb_create_null_data);
    RUN_TEST(test_lierre_rgb_create_zero_size);
    RUN_TEST(test_lierre_rgb_create_zero_width);
    RUN_TEST(test_lierre_rgb_create_zero_height);
    RUN_TEST(test_lierre_rgb_create_large_image);

    RUN_TEST(test_lierre_rgb_destroy_null);

    RUN_TEST(test_encode_decode_simple_text);
    RUN_TEST(test_encode_decode_binary_data);
    RUN_TEST(test_encode_decode_high_ecc);
    RUN_TEST(test_encode_decode_large_data);
    RUN_TEST(test_encode_decode_multithread);
    RUN_TEST(test_encode_decode_custom_colors);
    RUN_TEST(test_encode_decode_inverted_colors);

    RUN_TEST(test_encode_decode_all_versions);
    RUN_TEST(test_encode_decode_all_versions_mt);

    RUN_TEST(test_encode_decode_numeric_mode_simple);
    RUN_TEST(test_encode_decode_numeric_mode_long);

    RUN_TEST(test_encode_decode_alphanumeric_mode_simple);
    RUN_TEST(test_encode_decode_alphanumeric_mode_with_special);

    RUN_TEST(test_encode_decode_kanji_mode);
    RUN_TEST(test_encode_decode_kanji_mode_multiple);

    RUN_TEST(test_encode_decode_eci_mode);

    return UNITY_END();
}

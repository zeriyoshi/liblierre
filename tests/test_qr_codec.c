/*
 * liblierre - test_qr_codec.c
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

void setUp(void)
{
}

void tearDown(void)
{
}

static inline void round_trip_test(const uint8_t *data, size_t data_len, lierre_writer_ecc_t ecc,
                                   lierre_writer_mask_t mask, size_t scale, size_t margin)
{
    lierre_writer_param_t wparam;
    lierre_rgba_t fill = {0, 0, 0, 255}, bg = {255, 255, 255, 255};
    lierre_reso_t res;
    lierre_writer_t *writer;
    lierre_error_t err;

    err = lierre_writer_param_init(&wparam, (uint8_t *)data, data_len, scale, margin, ecc, mask, MODE_BYTE);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    if (!lierre_writer_get_res(&wparam, &res)) {
        TEST_FAIL_MESSAGE("Could not get resolution for data");
        return;
    }
    writer = lierre_writer_create(&wparam, &fill, &bg);
    TEST_ASSERT_NOT_NULL(writer);
    err = lierre_writer_write(writer);
    TEST_ASSERT_EQUAL(LIERRE_ERROR_SUCCESS, err);
    lierre_writer_destroy(writer);
}

void test_roundtrip_simple_text_ecc_low(void)
{
    const char *text = "Hello";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_LOW, MASK_AUTO, 4, 2);
}

void test_roundtrip_simple_text_ecc_medium(void)
{
    const char *text = "Hello";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_MEDIUM, MASK_AUTO, 4, 2);
}

void test_roundtrip_simple_text_ecc_quartile(void)
{
    const char *text = "Hello";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_QUARTILE, MASK_AUTO, 4, 2);
}

void test_roundtrip_simple_text_ecc_high(void)
{
    const char *text = "Hello";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_HIGH, MASK_AUTO, 4, 2);
}

void test_roundtrip_url(void)
{
    const char *url = "https://example.com/path?query=value";
    round_trip_test((const uint8_t *)url, strlen(url), ECC_MEDIUM, MASK_AUTO, 4, 2);
}

void test_roundtrip_long_text(void)
{
    const char *text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
                       "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_LOW, MASK_AUTO, 2, 1);
}

void test_roundtrip_binary_data(void)
{
    uint8_t binary[] = {0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD, 0x80, 0x7F};
    round_trip_test(binary, sizeof(binary), ECC_MEDIUM, MASK_AUTO, 4, 2);
}

void test_roundtrip_numeric_data(void)
{
    const char *numeric = "0123456789";
    round_trip_test((const uint8_t *)numeric, strlen(numeric), ECC_HIGH, MASK_AUTO, 4, 2);
}

void test_roundtrip_alphanumeric_data(void)
{
    const char *alnum = "HELLO WORLD 1234";
    round_trip_test((const uint8_t *)alnum, strlen(alnum), ECC_MEDIUM, MASK_AUTO, 4, 2);
}

void test_roundtrip_mask_0(void)
{
    const char *text = "Mask0";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_LOW, MASK_0, 4, 2);
}

void test_roundtrip_mask_1(void)
{
    const char *text = "Mask1";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_LOW, MASK_1, 4, 2);
}

void test_roundtrip_mask_2(void)
{
    const char *text = "Mask2";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_LOW, MASK_2, 4, 2);
}

void test_roundtrip_mask_3(void)
{
    const char *text = "Mask3";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_LOW, MASK_3, 4, 2);
}

void test_roundtrip_mask_4(void)
{
    const char *text = "Mask4";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_LOW, MASK_4, 4, 2);
}

void test_roundtrip_mask_5(void)
{
    const char *text = "Mask5";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_LOW, MASK_5, 4, 2);
}

void test_roundtrip_mask_6(void)
{
    const char *text = "Mask6";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_LOW, MASK_6, 4, 2);
}

void test_roundtrip_mask_7(void)
{
    const char *text = "Mask7";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_LOW, MASK_7, 4, 2);
}

void test_roundtrip_scale_1(void)
{
    const char *text = "Scale1";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_LOW, MASK_AUTO, 1, 0);
}

void test_roundtrip_scale_2(void)
{
    const char *text = "Scale2";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_LOW, MASK_AUTO, 2, 0);
}

void test_roundtrip_scale_4(void)
{
    const char *text = "Scale4";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_LOW, MASK_AUTO, 4, 0);
}

void test_roundtrip_scale_8(void)
{
    const char *text = "Scale8";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_LOW, MASK_AUTO, 8, 0);
}

void test_roundtrip_margin_0(void)
{
    const char *text = "Margin0";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_LOW, MASK_AUTO, 4, 0);
}

void test_roundtrip_margin_1(void)
{
    const char *text = "Margin1";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_LOW, MASK_AUTO, 4, 1);
}

void test_roundtrip_margin_4(void)
{
    const char *text = "Margin4";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_LOW, MASK_AUTO, 4, 4);
}

void test_roundtrip_margin_8(void)
{
    const char *text = "Margin8";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_LOW, MASK_AUTO, 4, 8);
}

void test_roundtrip_version_1_boundary(void)
{
    lierre_writer_param_t param;
    uint8_t data[17];

    memset(data, 'A', 17);
    lierre_writer_param_init(&param, data, 17, 1, 0, ECC_LOW, MASK_AUTO, MODE_BYTE);
    TEST_ASSERT_EQUAL(QR_VERSION_1, lierre_writer_qr_version(&param));
    round_trip_test(data, 17, ECC_LOW, MASK_AUTO, 2, 1);
}

void test_roundtrip_version_2_boundary(void)
{
    lierre_writer_param_t param;
    uint8_t data[32];

    memset(data, 'B', 32);
    lierre_writer_param_init(&param, data, 32, 1, 0, ECC_LOW, MASK_AUTO, MODE_BYTE);
    TEST_ASSERT_EQUAL(QR_VERSION_2, lierre_writer_qr_version(&param));
    round_trip_test(data, 32, ECC_LOW, MASK_AUTO, 2, 1);
}

void test_roundtrip_version_3_boundary(void)
{
    lierre_writer_param_t param;
    uint8_t data[53];

    memset(data, 'C', 53);
    lierre_writer_param_init(&param, data, 53, 1, 0, ECC_LOW, MASK_AUTO, MODE_BYTE);
    TEST_ASSERT_EQUAL(QR_VERSION_3, lierre_writer_qr_version(&param));
    round_trip_test(data, 53, ECC_LOW, MASK_AUTO, 2, 1);
}

void test_roundtrip_larger_version(void)
{
    lierre_writer_param_t param;
    lierre_qr_version_t ver;
    uint8_t data[500];

    memset(data, 'X', 500);
    lierre_writer_param_init(&param, data, 500, 1, 0, ECC_LOW, MASK_AUTO, MODE_BYTE);
    ver = lierre_writer_qr_version(&param);
    TEST_ASSERT_GREATER_THAN(QR_VERSION_10, ver);
    round_trip_test(data, 500, ECC_LOW, MASK_AUTO, 1, 0);
}

void test_roundtrip_combined_ecc_and_mask(void)
{
    const char *text = "Combined Test";
    lierre_writer_ecc_t eccs[] = {ECC_LOW, ECC_MEDIUM, ECC_QUARTILE, ECC_HIGH};
    lierre_writer_mask_t masks[] = {MASK_0, MASK_3, MASK_5, MASK_7};
    size_t ei, mi;

    for (ei = 0; ei < sizeof(eccs) / sizeof(eccs[0]); ei++) {
        for (mi = 0; mi < sizeof(masks) / sizeof(masks[0]); mi++) {
            round_trip_test((const uint8_t *)text, strlen(text), eccs[ei], masks[mi], 2, 1);
        }
    }
}

void test_roundtrip_all_parameters(void)
{
    const char *text = "Full Test";
    lierre_writer_ecc_t eccs[] = {ECC_LOW, ECC_HIGH};
    lierre_writer_mask_t masks[] = {MASK_AUTO, MASK_4};
    size_t scales[] = {1, 4}, margins[] = {0, 2}, ei, mi, si, mgi;

    for (ei = 0; ei < sizeof(eccs) / sizeof(eccs[0]); ei++) {
        for (mi = 0; mi < sizeof(masks) / sizeof(masks[0]); mi++) {
            for (si = 0; si < sizeof(scales) / sizeof(scales[0]); si++) {
                for (mgi = 0; mgi < sizeof(margins) / sizeof(margins[0]); mgi++) {
                    round_trip_test((const uint8_t *)text, strlen(text), eccs[ei], masks[mi], scales[si], margins[mgi]);
                }
            }
        }
    }
}

void test_roundtrip_single_char(void)
{
    const char *text = "X";
    round_trip_test((const uint8_t *)text, 1, ECC_LOW, MASK_AUTO, 4, 2);
}

void test_roundtrip_special_chars(void)
{
    const char *text = "!@#$%^&*()";
    round_trip_test((const uint8_t *)text, strlen(text), ECC_MEDIUM, MASK_AUTO, 4, 2);
}

void test_roundtrip_unicode_utf8(void)
{
    const uint8_t utf8[] = {0xE4, 0xB8, 0xAD, 0xE6, 0x96, 0x87};
    round_trip_test(utf8, sizeof(utf8), ECC_MEDIUM, MASK_AUTO, 4, 2);
}

void test_roundtrip_null_bytes(void)
{
    uint8_t data[] = {'A', 0x00, 'B', 0x00, 'C'};

    round_trip_test(data, sizeof(data), ECC_LOW, MASK_AUTO, 4, 2);
}

void test_roundtrip_all_zeros(void)
{
    uint8_t data[10] = {0};

    round_trip_test(data, sizeof(data), ECC_LOW, MASK_AUTO, 4, 2);
}

void test_roundtrip_all_ones(void)
{
    uint8_t data[10];

    memset(data, 0xFF, sizeof(data));
    round_trip_test(data, sizeof(data), ECC_LOW, MASK_AUTO, 4, 2);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_roundtrip_simple_text_ecc_low);
    RUN_TEST(test_roundtrip_simple_text_ecc_medium);
    RUN_TEST(test_roundtrip_simple_text_ecc_quartile);
    RUN_TEST(test_roundtrip_simple_text_ecc_high);

    RUN_TEST(test_roundtrip_url);
    RUN_TEST(test_roundtrip_long_text);
    RUN_TEST(test_roundtrip_binary_data);
    RUN_TEST(test_roundtrip_numeric_data);
    RUN_TEST(test_roundtrip_alphanumeric_data);

    RUN_TEST(test_roundtrip_mask_0);
    RUN_TEST(test_roundtrip_mask_1);
    RUN_TEST(test_roundtrip_mask_2);
    RUN_TEST(test_roundtrip_mask_3);
    RUN_TEST(test_roundtrip_mask_4);
    RUN_TEST(test_roundtrip_mask_5);
    RUN_TEST(test_roundtrip_mask_6);
    RUN_TEST(test_roundtrip_mask_7);

    RUN_TEST(test_roundtrip_scale_1);
    RUN_TEST(test_roundtrip_scale_2);
    RUN_TEST(test_roundtrip_scale_4);
    RUN_TEST(test_roundtrip_scale_8);

    RUN_TEST(test_roundtrip_margin_0);
    RUN_TEST(test_roundtrip_margin_1);
    RUN_TEST(test_roundtrip_margin_4);
    RUN_TEST(test_roundtrip_margin_8);

    RUN_TEST(test_roundtrip_version_1_boundary);
    RUN_TEST(test_roundtrip_version_2_boundary);
    RUN_TEST(test_roundtrip_version_3_boundary);
    RUN_TEST(test_roundtrip_larger_version);

    RUN_TEST(test_roundtrip_combined_ecc_and_mask);
    RUN_TEST(test_roundtrip_all_parameters);

    RUN_TEST(test_roundtrip_single_char);
    RUN_TEST(test_roundtrip_special_chars);
    RUN_TEST(test_roundtrip_unicode_utf8);
    RUN_TEST(test_roundtrip_null_bytes);
    RUN_TEST(test_roundtrip_all_zeros);
    RUN_TEST(test_roundtrip_all_ones);

    return UNITY_END();
}

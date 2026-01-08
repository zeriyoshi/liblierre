/*
 * liblierre - writer.h
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef LIERRE_WRITER_H
#define LIERRE_WRITER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <lierre.h>

#define LIERRE_WRITER_ECC_LOW      0
#define LIERRE_WRITER_ECC_MEDIUM   1
#define LIERRE_WRITER_ECC_QUARTILE 2
#define LIERRE_WRITER_ECC_HIGH     3

#define LIERRE_WRITER_MASK_AUTO -1
#define LIERRE_WRITER_MASK_0    0
#define LIERRE_WRITER_MASK_1    1
#define LIERRE_WRITER_MASK_2    2
#define LIERRE_WRITER_MASK_3    3
#define LIERRE_WRITER_MASK_4    4
#define LIERRE_WRITER_MASK_5    5
#define LIERRE_WRITER_MASK_6    6
#define LIERRE_WRITER_MASK_7    7

#define LIERRE_WRITER_MODE_NUMERIC      1
#define LIERRE_WRITER_MODE_ALPHANUMERIC 2
#define LIERRE_WRITER_MODE_BYTE         4
#define LIERRE_WRITER_MODE_KANJI        8
#define LIERRE_WRITER_MODE_ECI          7

#define LIERRE_WRITER_QR_VERSION_ERR -1
#define LIERRE_WRITER_QR_VERSION_1   1
#define LIERRE_WRITER_QR_VERSION_2   2
#define LIERRE_WRITER_QR_VERSION_3   3
#define LIERRE_WRITER_QR_VERSION_4   4
#define LIERRE_WRITER_QR_VERSION_5   5
#define LIERRE_WRITER_QR_VERSION_6   6
#define LIERRE_WRITER_QR_VERSION_7   7
#define LIERRE_WRITER_QR_VERSION_8   8
#define LIERRE_WRITER_QR_VERSION_9   9
#define LIERRE_WRITER_QR_VERSION_10  10
#define LIERRE_WRITER_QR_VERSION_11  11
#define LIERRE_WRITER_QR_VERSION_12  12
#define LIERRE_WRITER_QR_VERSION_13  13
#define LIERRE_WRITER_QR_VERSION_14  14
#define LIERRE_WRITER_QR_VERSION_15  15
#define LIERRE_WRITER_QR_VERSION_16  16
#define LIERRE_WRITER_QR_VERSION_17  17
#define LIERRE_WRITER_QR_VERSION_18  18
#define LIERRE_WRITER_QR_VERSION_19  19
#define LIERRE_WRITER_QR_VERSION_20  20
#define LIERRE_WRITER_QR_VERSION_21  21
#define LIERRE_WRITER_QR_VERSION_22  22
#define LIERRE_WRITER_QR_VERSION_23  23
#define LIERRE_WRITER_QR_VERSION_24  24
#define LIERRE_WRITER_QR_VERSION_25  25
#define LIERRE_WRITER_QR_VERSION_26  26
#define LIERRE_WRITER_QR_VERSION_27  27
#define LIERRE_WRITER_QR_VERSION_28  28
#define LIERRE_WRITER_QR_VERSION_29  29
#define LIERRE_WRITER_QR_VERSION_30  30
#define LIERRE_WRITER_QR_VERSION_31  31
#define LIERRE_WRITER_QR_VERSION_32  32
#define LIERRE_WRITER_QR_VERSION_33  33
#define LIERRE_WRITER_QR_VERSION_34  34
#define LIERRE_WRITER_QR_VERSION_35  35
#define LIERRE_WRITER_QR_VERSION_36  36
#define LIERRE_WRITER_QR_VERSION_37  37
#define LIERRE_WRITER_QR_VERSION_38  38
#define LIERRE_WRITER_QR_VERSION_39  39
#define LIERRE_WRITER_QR_VERSION_40  40

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ECC_LOW = LIERRE_WRITER_ECC_LOW,
    ECC_MEDIUM = LIERRE_WRITER_ECC_MEDIUM,
    ECC_QUARTILE = LIERRE_WRITER_ECC_QUARTILE,
    ECC_HIGH = LIERRE_WRITER_ECC_HIGH
} lierre_writer_ecc_t;

typedef enum {
    MASK_AUTO = LIERRE_WRITER_MASK_AUTO,
    MASK_0 = LIERRE_WRITER_MASK_0,
    MASK_1 = LIERRE_WRITER_MASK_1,
    MASK_2 = LIERRE_WRITER_MASK_2,
    MASK_3 = LIERRE_WRITER_MASK_3,
    MASK_4 = LIERRE_WRITER_MASK_4,
    MASK_5 = LIERRE_WRITER_MASK_5,
    MASK_6 = LIERRE_WRITER_MASK_6,
    MASK_7 = LIERRE_WRITER_MASK_7
} lierre_writer_mask_t;

typedef enum {
    MODE_NUMERIC = LIERRE_WRITER_MODE_NUMERIC,
    MODE_ALPHANUMERIC = LIERRE_WRITER_MODE_ALPHANUMERIC,
    MODE_BYTE = LIERRE_WRITER_MODE_BYTE,
    MODE_KANJI = LIERRE_WRITER_MODE_KANJI,
    MODE_ECI = LIERRE_WRITER_MODE_ECI
} lierre_writer_mode_t;

typedef enum {
    QR_VERSION_ERR = LIERRE_WRITER_QR_VERSION_ERR,
    QR_VERSION_1 = LIERRE_WRITER_QR_VERSION_1,
    QR_VERSION_2 = LIERRE_WRITER_QR_VERSION_2,
    QR_VERSION_3 = LIERRE_WRITER_QR_VERSION_3,
    QR_VERSION_4 = LIERRE_WRITER_QR_VERSION_4,
    QR_VERSION_5 = LIERRE_WRITER_QR_VERSION_5,
    QR_VERSION_6 = LIERRE_WRITER_QR_VERSION_6,
    QR_VERSION_7 = LIERRE_WRITER_QR_VERSION_7,
    QR_VERSION_8 = LIERRE_WRITER_QR_VERSION_8,
    QR_VERSION_9 = LIERRE_WRITER_QR_VERSION_9,
    QR_VERSION_10 = LIERRE_WRITER_QR_VERSION_10,
    QR_VERSION_11 = LIERRE_WRITER_QR_VERSION_11,
    QR_VERSION_12 = LIERRE_WRITER_QR_VERSION_12,
    QR_VERSION_13 = LIERRE_WRITER_QR_VERSION_13,
    QR_VERSION_14 = LIERRE_WRITER_QR_VERSION_14,
    QR_VERSION_15 = LIERRE_WRITER_QR_VERSION_15,
    QR_VERSION_16 = LIERRE_WRITER_QR_VERSION_16,
    QR_VERSION_17 = LIERRE_WRITER_QR_VERSION_17,
    QR_VERSION_18 = LIERRE_WRITER_QR_VERSION_18,
    QR_VERSION_19 = LIERRE_WRITER_QR_VERSION_19,
    QR_VERSION_20 = LIERRE_WRITER_QR_VERSION_20,
    QR_VERSION_21 = LIERRE_WRITER_QR_VERSION_21,
    QR_VERSION_22 = LIERRE_WRITER_QR_VERSION_22,
    QR_VERSION_23 = LIERRE_WRITER_QR_VERSION_23,
    QR_VERSION_24 = LIERRE_WRITER_QR_VERSION_24,
    QR_VERSION_25 = LIERRE_WRITER_QR_VERSION_25,
    QR_VERSION_26 = LIERRE_WRITER_QR_VERSION_26,
    QR_VERSION_27 = LIERRE_WRITER_QR_VERSION_27,
    QR_VERSION_28 = LIERRE_WRITER_QR_VERSION_28,
    QR_VERSION_29 = LIERRE_WRITER_QR_VERSION_29,
    QR_VERSION_30 = LIERRE_WRITER_QR_VERSION_30,
    QR_VERSION_31 = LIERRE_WRITER_QR_VERSION_31,
    QR_VERSION_32 = LIERRE_WRITER_QR_VERSION_32,
    QR_VERSION_33 = LIERRE_WRITER_QR_VERSION_33,
    QR_VERSION_34 = LIERRE_WRITER_QR_VERSION_34,
    QR_VERSION_35 = LIERRE_WRITER_QR_VERSION_35,
    QR_VERSION_36 = LIERRE_WRITER_QR_VERSION_36,
    QR_VERSION_37 = LIERRE_WRITER_QR_VERSION_37,
    QR_VERSION_38 = LIERRE_WRITER_QR_VERSION_38,
    QR_VERSION_39 = LIERRE_WRITER_QR_VERSION_39,
    QR_VERSION_40 = LIERRE_WRITER_QR_VERSION_40
} lierre_qr_version_t;

typedef struct {
    uint8_t *data;
    size_t data_size;
    size_t scale;
    size_t margin;
    lierre_writer_ecc_t ecc_level;
    lierre_writer_mask_t mask_pattern;
    lierre_writer_mode_t mode;
} lierre_writer_param_t;

typedef struct _lierre_writer_t lierre_writer_t;

lierre_error_t lierre_writer_param_init(lierre_writer_param_t *param, uint8_t *data, size_t data_size, size_t scale,
                                        size_t margin, lierre_writer_ecc_t ecc_level, lierre_writer_mask_t mask_pattern,
                                        lierre_writer_mode_t mode);
lierre_qr_version_t lierre_writer_qr_version(const lierre_writer_param_t *param);
bool lierre_writer_get_res(const lierre_writer_param_t *param, lierre_reso_t *res);
size_t lierre_writer_get_res_width(const lierre_writer_param_t *param);
size_t lierre_writer_get_res_height(const lierre_writer_param_t *param);

lierre_writer_t *lierre_writer_create(const lierre_writer_param_t *param, const lierre_rgba_t *fill_color,
                                      const lierre_rgba_t *bg_color);
void lierre_writer_destroy(lierre_writer_t *writer);
lierre_error_t lierre_writer_write(lierre_writer_t *writer);

const uint8_t *lierre_writer_get_rgba_data(const lierre_writer_t *writer);
size_t lierre_writer_get_rgba_data_size(const lierre_writer_t *writer);

#ifdef __cplusplus
}
#endif

#endif /* LIERRE_WRITER_H */

/*
 * liblierre - structs.h
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef LIERRE_INTERNAL_STRUCTS_H
#define LIERRE_INTERNAL_STRUCTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <lierre.h>
#include <lierre/reader.h>
#include <lierre/writer.h>

struct _lierre_reader_t {
    lierre_rgb_data_t *data;
    lierre_reader_param_t *param;
};

struct _lierre_reader_result_t {
    uint32_t num_qr_codes;
    lierre_rect_t *qr_code_rects;
    uint8_t **qr_code_datas;
    size_t *qr_code_data_sizes;
};

struct _lierre_writer_t {
    lierre_rgb_data_t *data;
    lierre_writer_param_t *param;
    uint8_t stroke_color_rgba[4];
    uint8_t fill_color_rgba[4];
};

#endif /* LIERRE_INTERNAL_STRUCTS_H */

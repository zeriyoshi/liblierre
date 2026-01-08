/*
 * liblierre - reader.h
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef LIERRE_READER_H
#define LIERRE_READER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <lierre.h>

#define LIERRE_READER_STRATEGY_NONE                 0
#define LIERRE_READER_STRATEGY_MINIMIZE             (1 << 1) /* minimize image to easy detect QR code*/
#define LIERRE_READER_STRATEGY_GLAYSCALE            (1 << 2) /* convert image to grayscale */
#define LIERRE_READER_STRATEGY_USE_RECT             (1 << 3) /* use rectangle to focus on specific area */
#define LIERRE_READER_STRATEGY_DENOISE              (1 << 4) /* apply denoising filter */
#define LIERRE_READER_STRATEGY_BRIGHTNESS_NORMALIZE (1 << 5) /* normalize brightness */
#define LIERRE_READER_STRATEGY_CONTRAST_NORMALIZE   (1 << 6) /* normalize contrast */
#define LIERRE_READER_STRATEGY_SHARPENING           (1 << 7) /* apply sharpening filter */
#define LIERRE_READER_STRATEGY_MT                   (1 << 8) /* use multi-threading */

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t lierre_reader_strategy_flag_t;

typedef struct {
    lierre_reader_strategy_flag_t strategy_flags;
    const lierre_rect_t *rect;
} lierre_reader_param_t;
typedef struct _lierre_reader_t lierre_reader_t;
typedef struct _lierre_reader_result_t lierre_reader_result_t;

lierre_error_t lierre_reader_param_init(lierre_reader_param_t *param);
void lierre_reader_param_set_flag(lierre_reader_param_t *param, lierre_reader_strategy_flag_t flag);
void lierre_reader_param_set_rect(lierre_reader_param_t *param, const lierre_rect_t *rect);

lierre_reader_t *lierre_reader_create(const lierre_reader_param_t *param);
void lierre_reader_destroy(lierre_reader_t *reader);
void lierre_reader_set_data(lierre_reader_t *reader, lierre_rgb_data_t *data);
lierre_error_t lierre_reader_read(lierre_reader_t *reader, lierre_reader_result_t **result);

void lierre_reader_result_destroy(lierre_reader_result_t *result);
uint32_t lierre_reader_result_get_num_qr_codes(const lierre_reader_result_t *result);
const lierre_rect_t *lierre_reader_result_get_qr_code_rect(const lierre_reader_result_t *result, uint32_t index);
const uint8_t *lierre_reader_result_get_qr_code_data(const lierre_reader_result_t *result, uint32_t index);
size_t lierre_reader_result_get_qr_code_data_size(const lierre_reader_result_t *result, uint32_t index);

#ifdef __cplusplus
}
#endif

#endif /* LIERRE_READER_H */

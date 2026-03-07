/*
 * liblierre - lierre.h
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef LIERRE_H
#define LIERRE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define LIERRE_ERROR_SUCCESS           0
#define LIERRE_ERROR_INVALID_PARAMS    1
#define LIERRE_ERROR_INVALID_GRID_SIZE 2
#define LIERRE_ERROR_INVALID_VERSION   3
#define LIERRE_ERROR_FORMAT_ECC        4
#define LIERRE_ERROR_DATA_ECC          5
#define LIERRE_ERROR_UNKNOWN_DATA_TYPE 6
#define LIERRE_ERROR_DATA_OVERFLOW     7
#define LIERRE_ERROR_DATA_UNDERFLOW    8
#define LIERRE_ERROR_SIZE_EXCEEDED     9

#ifdef __cplusplus
extern "C" {
#endif

typedef enum lierre_error {
    SUCCESS = LIERRE_ERROR_SUCCESS,
    ERROR_INVALID_PARAMS = LIERRE_ERROR_INVALID_PARAMS,
    ERROR_INVALID_GRID_SIZE = LIERRE_ERROR_INVALID_GRID_SIZE,
    ERROR_INVALID_VERSION = LIERRE_ERROR_INVALID_VERSION,
    ERROR_FORMAT_ECC = LIERRE_ERROR_FORMAT_ECC,
    ERROR_DATA_ECC = LIERRE_ERROR_DATA_ECC,
    ERROR_UNKNOWN_DATA_TYPE = LIERRE_ERROR_UNKNOWN_DATA_TYPE,
    ERROR_DATA_OVERFLOW = LIERRE_ERROR_DATA_OVERFLOW,
    ERROR_DATA_UNDERFLOW = LIERRE_ERROR_DATA_UNDERFLOW,
    ERROR_SIZE_EXCEEDED = LIERRE_ERROR_SIZE_EXCEEDED,
    ERROR_MAX = ERROR_SIZE_EXCEEDED + 1
} lierre_error_t;

const char *lierre_strerror(lierre_error_t err);

typedef uint32_t lierre_buildtime_t;

typedef struct {
    uint8_t *data;
    size_t data_size;
    size_t width;
    size_t height;
} lierre_rgb_data_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} lierre_rgba_t;

typedef struct {
    size_t x;
    size_t y;
} lierre_vec2_t;

typedef struct {
    size_t width;
    size_t height;
} lierre_reso_t;

typedef struct {
    lierre_vec2_t origin;
    lierre_reso_t size;
} lierre_rect_t;

lierre_rgb_data_t *lierre_rgb_create(const uint8_t *data, size_t data_size, size_t width, size_t height);
void lierre_rgb_destroy(lierre_rgb_data_t *rgb);

uint32_t lierre_version_id(void);
lierre_buildtime_t lierre_buildtime(void);

#ifdef __cplusplus
}
#endif

#endif /* LIERRE_H */

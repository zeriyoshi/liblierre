/*
 * liblierre - lierre.c
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <string.h>

#include <lierre.h>

#include "internal/memory.h"

#define LIERRE_VERSION_ID 10000000

#ifndef LIERRE_BUILDTIME
#define LIERRE_BUILDTIME 0
#endif

static const char *const ERROR_MESSAGE_TABLE[] = {
    "Success",        "Invalid parameters", "Invalid grid size", "Invalid version", "Format ECC error",
    "Data ECC error", "Unknown data type",  "Data overflow",     "Data underflow",  "Size exceeded"};

extern const char *lierre_strerror(lierre_error_t err)
{
    if (err >= ERROR_MAX) {
        return "Unknown error";
    }

    return ERROR_MESSAGE_TABLE[err];
}

extern uint32_t lierre_version_id()
{
    return (uint32_t)LIERRE_VERSION_ID;
}

extern lierre_buildtime_t lierre_buildtime()
{
    return (lierre_buildtime_t)LIERRE_BUILDTIME;
}

extern lierre_rgb_data_t *lierre_rgb_create(const uint8_t *data, size_t data_size, size_t width, size_t height)
{
    lierre_rgb_data_t *rgb;

    if (!data || data_size == 0 || width == 0 || height == 0) {
        return NULL;
    }

    rgb = (lierre_rgb_data_t *)lmalloc(sizeof(lierre_rgb_data_t));
    if (!rgb) {
        return NULL;
    }

    rgb->data = (uint8_t *)lmalloc(data_size);
    if (!rgb->data) {
        lfree(rgb);
        return NULL;
    }

    lmemcpy(rgb->data, data, data_size);
    rgb->data_size = data_size;
    rgb->width = width;
    rgb->height = height;

    return rgb;
}

extern void lierre_rgb_destroy(lierre_rgb_data_t *rgb)
{
    if (!rgb) {
        return;
    }

    if (rgb->data) {
        lfree(rgb->data);
    }

    lfree(rgb);
}

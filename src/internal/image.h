/*
 * liblierre - image.h
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef LIERRE_INTERNAL_IMAGE_H
#define LIERRE_INTERNAL_IMAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <lierre/portable.h>

void lierre_image_brightness_normalize(uint8_t *image, size_t width, size_t height);
void lierre_image_contrast_normalize(uint8_t *image, size_t width, size_t height);
void lierre_image_denoise_mt(uint8_t *image, size_t width, size_t height, uint32_t num_threads);
void lierre_image_denoise(uint8_t *image, size_t width, size_t height);
void lierre_image_sharpen_mt(uint8_t *image, size_t width, size_t height, uint32_t num_threads);
void lierre_image_sharpen(uint8_t *image, size_t width, size_t height);

#endif /* LIERRE_INTERNAL_IMAGE_H */

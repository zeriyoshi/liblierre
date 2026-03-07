/*
 * liblierre - utils.h
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef LIERRE_TEST_UTILS_H
#define LIERRE_TEST_UTILS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lierre.h>

#pragma pack(push, 1)
typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} test_bmp_file_header_t;

typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bit_count;
    uint32_t compression;
    uint32_t size_image;
    int32_t x_pels_per_meter;
    int32_t y_pels_per_meter;
    uint32_t clr_used;
    uint32_t clr_important;
} test_bmp_info_header_t;
#pragma pack(pop)

static inline uint16_t test_le16_to_cpu(uint16_t v)
{
    const uint8_t *p = (const uint8_t *)&v;

    return (uint16_t)(p[0] | (p[1] << 8));
}

static inline uint32_t test_le32_to_cpu(uint32_t v)
{
    const uint8_t *p = (const uint8_t *)&v;

    return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

static inline int32_t test_le32s_to_cpu(int32_t v)
{
    uint32_t u;

    memcpy(&u, &v, sizeof(u));
    u = test_le32_to_cpu(u);
    memcpy(&v, &u, sizeof(v));

    return v;
}

static inline lierre_rgb_data_t *test_load_bmp(const char *path)
{
    FILE *fp;
    test_bmp_file_header_t file_header;
    test_bmp_info_header_t info_header;
    lierre_rgb_data_t *result;
    uint32_t data_offset;
    uint16_t bit_count;
    uint8_t *rgb_data, *row_buf, *src, *dst;
    int32_t top_down, info_width, info_height;
    size_t width, height, row_size, i, x, y;

    fp = fopen(path, "rb");
    if (!fp) {
        return NULL;
    }

    if (fread(&file_header, sizeof(file_header), 1, fp) != 1) {
        fclose(fp);
        return NULL;
    }

    if (test_le16_to_cpu(file_header.type) != 0x4D42) {
        fclose(fp);
        return NULL;
    }

    data_offset = test_le32_to_cpu(file_header.offset);

    if (fread(&info_header, sizeof(info_header), 1, fp) != 1) {
        fclose(fp);
        return NULL;
    }

    bit_count = test_le16_to_cpu(info_header.bit_count);
    if (bit_count != 24) {
        fclose(fp);
        return NULL;
    }

    info_width = test_le32s_to_cpu(info_header.width);
    info_height = test_le32s_to_cpu(info_header.height);

    width = (size_t)(info_width > 0 ? info_width : -info_width);
    top_down = info_height < 0;
    height = (size_t)(info_height > 0 ? info_height : -info_height);

    row_size = ((width * 3 + 3) / 4) * 4;

    rgb_data = (uint8_t *)malloc(width * height * 3);
    if (!rgb_data) {
        fclose(fp);

        return NULL;
    }

    row_buf = (uint8_t *)malloc(row_size);
    if (!row_buf) {
        free(rgb_data);
        fclose(fp);

        return NULL;
    }

    fseek(fp, (long)data_offset, SEEK_SET);

    for (i = 0; i < height; i++) {
        if (fread(row_buf, row_size, 1, fp) != 1) {
            free(row_buf);
            free(rgb_data);
            fclose(fp);
            return NULL;
        }

        y = top_down ? i : (height - 1 - i);
        src = row_buf;
        dst = rgb_data + y * width * 3;

        for (x = 0; x < width; x++) {
            dst[x * 3 + 0] = src[x * 3 + 2];
            dst[x * 3 + 1] = src[x * 3 + 1];
            dst[x * 3 + 2] = src[x * 3 + 0];
        }
    }

    free(row_buf);
    fclose(fp);

    result = lierre_rgb_create(rgb_data, width * height * 3, width, height);
    free(rgb_data);

    return result;
}

#endif /* LIERRE_TEST_UTILS_H */

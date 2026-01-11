/*
 * liblierre - decoder.h
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef LIERRE_INTERNAL_DECODER_H
#define LIERRE_INTERNAL_DECODER_H

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <lierre.h>
#include <lierre/portable.h>

#include <poporon.h>

#include "memory.h"

#define LIERRE_DECODER_MAX_REGIONS        1024
#define LIERRE_DECODER_MAX_CAPSTONES      32
#define LIERRE_DECODER_MAX_GRIDS          (LIERRE_DECODER_MAX_CAPSTONES * 2)
#define LIERRE_DECODER_PERSPECTIVE_PARAMS 8
#define LIERRE_DECODER_MAX_PAYLOAD        8896

#define LIERRE_DECODER_MT_MAX_THREADS 64

#define LIERRE_DECODER_MAX_VERSION   40
#define LIERRE_DECODER_MAX_GRID_SIZE (LIERRE_DECODER_MAX_VERSION * 4 + 17)
#define LIERRE_DECODER_MAX_BITMAP    (((LIERRE_DECODER_MAX_GRID_SIZE * LIERRE_DECODER_MAX_GRID_SIZE) + 7) / 8)
#define LIERRE_DECODER_MAX_ALIGNMENT 8

#define LIERRE_PIXEL_WHITE  0
#define LIERRE_PIXEL_BLACK  1
#define LIERRE_PIXEL_REGION 2

typedef uint16_t lierre_pixel_t;

typedef struct {
    int32_t x;
    int32_t y;
} decoder_point_t;

typedef struct {
    decoder_point_t seed;
    int32_t count;
    int32_t capstone;
} region_t;

typedef struct {
    int32_t ring;
    int32_t stone;
    decoder_point_t corners[4];
    decoder_point_t center;
    double c[LIERRE_DECODER_PERSPECTIVE_PARAMS];
    int32_t qr_grid;
} capstone_t;

typedef struct {
    int32_t caps[3];
    int32_t align_region;
    decoder_point_t align;
    decoder_point_t tpep[3];
    int32_t grid_size;
    double c[LIERRE_DECODER_PERSPECTIVE_PARAMS];
} grid_t;

typedef struct {
    int32_t y;
    int32_t right;
    int32_t left_up;
    int32_t left_down;
} flood_fill_vars_t;

typedef struct {
    uint8_t *image;
    lierre_pixel_t *pixels;
    int32_t w;
    int32_t h;
    uint8_t threshold;
    int32_t num_regions;
    region_t regions[LIERRE_DECODER_MAX_REGIONS];
    int32_t num_capstones;
    capstone_t capstones[LIERRE_DECODER_MAX_CAPSTONES];
    int32_t num_grids;
    grid_t grids[LIERRE_DECODER_MAX_GRIDS];
    size_t num_flood_fill_vars;
    flood_fill_vars_t *flood_fill_vars;
} decoder_t;

typedef struct {
    decoder_point_t corners[4];
    uint8_t payload[LIERRE_DECODER_MAX_PAYLOAD];
    int32_t payload_len;
} decoder_code_t;

typedef struct {
    uint32_t count;
    decoder_code_t codes[LIERRE_DECODER_MAX_GRIDS];
} decoder_result_t;

typedef struct {
    int32_t bs;
    int32_t dw;
    int32_t ns;
} rs_params_t;

typedef struct {
    int32_t data_bytes;
    int32_t apat[LIERRE_DECODER_MAX_ALIGNMENT];
    rs_params_t ecc[4];
} version_info_t;

typedef struct {
    decoder_point_t corners[4];
    int32_t size;
    uint8_t cell_bitmap[LIERRE_DECODER_MAX_BITMAP];
} qr_code_t;

typedef struct {
    int32_t version;
    int32_t ecc_level;
    int32_t mask;
    int32_t data_type;
    uint8_t payload[LIERRE_DECODER_MAX_PAYLOAD];
    int32_t payload_len;
    uint32_t eci;
} qr_data_t;

typedef struct {
    uint8_t *raw;
    int32_t data_bits;
    int32_t ptr;
    uint8_t data[LIERRE_DECODER_MAX_PAYLOAD];
} datastream_t;

typedef struct {
    decoder_t *decoder;
    int32_t grid_index;
    qr_code_t code;
    qr_data_t data;
    lierre_error_t err;
} decode_thread_ctx_t;

typedef struct {
    decoder_point_t reference;
    int32_t scores[4];
    decoder_point_t *corners;
} corner_finder_data_t;

typedef struct {
    int32_t index;
    double distance;
} capstone_neighbour_t;

typedef struct {
    capstone_neighbour_t entries[LIERRE_DECODER_MAX_CAPSTONES];
    int32_t count;
} capstone_neighbour_list_t;

typedef void (*span_callback_t)(void *user_data, int32_t y, int32_t left, int32_t right);

extern const version_info_t lierre_version_db[LIERRE_DECODER_MAX_VERSION + 1];

decoder_t *lierre_decoder_create(void);
void lierre_decoder_destroy(decoder_t *decoder);
lierre_error_t lierre_decoder_process(decoder_t *decoder, const uint8_t *gray_image, int32_t width, int32_t height,
                                      decoder_result_t *result);
lierre_error_t lierre_decoder_process_mt(decoder_t *decoder, const uint8_t *gray_image, int32_t width, int32_t height,
                                         decoder_result_t *result, uint32_t num_threads);

void flood_fill_seed(decoder_t *decoder, int32_t seed_x, int32_t seed_y, lierre_pixel_t source_color,
                     lierre_pixel_t target_color, span_callback_t callback, void *user_data);
int32_t get_or_create_region(decoder_t *decoder, int32_t x, int32_t y);
void find_region_corners(decoder_t *decoder, int32_t region_id, const decoder_point_t *reference,
                         decoder_point_t *corners);
void scan_finder_patterns(decoder_t *decoder, uint32_t y);
void find_capstone_groups(decoder_t *decoder, int32_t capstone_index);

void perspective_map(const double *coeffs, double u, double v, decoder_point_t *result);
void perspective_setup(double *coeffs, const decoder_point_t *corners, double width, double height);
void perspective_unmap(const double *coeffs, const decoder_point_t *image_point, double *grid_u, double *grid_v);
void extract_qr_code(const decoder_t *decoder, int32_t grid_index, qr_code_t *code);
void test_neighbour_pairs(decoder_t *decoder, int32_t capstone_index, const capstone_neighbour_list_t *horizontal_list,
                          const capstone_neighbour_list_t *vertical_list);

lierre_error_t decode_qr(const qr_code_t *code, qr_data_t *data);

#endif /* LIERRE_INTERNAL_DECODER_H */

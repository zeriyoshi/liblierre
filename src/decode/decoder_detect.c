/*
 * liblierre - decoder_detect.c
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "../internal/decoder.h"

#define FINDER_PATTERN_MODULES      5
#define FINDER_PATTERN_CENTER_RATIO 3
#define FINDER_PATTERN_SCALE_FACTOR 16
#define FINDER_TOLERANCE_DIVISOR    4
#define FINDER_TOLERANCE_MULTIPLIER 3

#define CAPSTONE_AREA_RATIO_MIN    10
#define CAPSTONE_AREA_RATIO_MAX    70
#define CAPSTONE_AREA_RATIO_FACTOR 100

#define FINDER_PATTERN_SIZE          7.0
#define FINDER_PATTERN_CENTER        3.5
#define NEIGHBOR_ALIGNMENT_THRESHOLD 0.2

#define NUM_CORNERS      4
#define NUM_EDGE_SAMPLES 2

static inline void flood_fill_line(lierre_decoder_t *decoder, int32_t x, int32_t y, lierre_pixel_t source_color,
                                   lierre_pixel_t target_color, span_callback_t callback, void *user_data,
                                   int32_t *left_out, int32_t *right_out)
{
    lierre_pixel_t *row;
    int32_t left, right, i;

    row = decoder->pixels + y * decoder->w;
    left = x;
    right = x;

    while (left > 0 && row[left - 1] == source_color) {
        left--;
    }

    while (right < decoder->w - 1 && row[right + 1] == source_color) {
        right++;
    }

    for (i = left; i <= right; i++) {
        row[i] = target_color;
    }

    *left_out = left;
    *right_out = right;

    if (callback) {
        callback(user_data, y, left, right);
    }
}

static inline lierre_flood_fill_vars_t *flood_fill_scan_next(lierre_decoder_t *decoder, lierre_pixel_t *row,
                                                             lierre_pixel_t source_color, lierre_pixel_t target_color,
                                                             span_callback_t callback, void *user_data,
                                                             lierre_flood_fill_vars_t *current_state, int32_t direction)
{
    int32_t *scan_position, next_left;
    lierre_flood_fill_vars_t *next_state;

    scan_position = (direction < 0) ? &current_state->left_up : &current_state->left_down;

    while (*scan_position <= current_state->right) {
        if (row[*scan_position] == source_color) {
            next_state = current_state + 1;
            next_state->y = current_state->y + direction;

            flood_fill_line(decoder, *scan_position, next_state->y, source_color, target_color, callback, user_data,
                            &next_left, &next_state->right);
            next_state->left_down = next_left;
            next_state->left_up = next_left;

            return next_state;
        }

        (*scan_position)++;
    }

    return NULL;
}

void flood_fill_seed(lierre_decoder_t *decoder, int32_t seed_x, int32_t seed_y, lierre_pixel_t source_color,
                     lierre_pixel_t target_color, span_callback_t callback, void *user_data)
{
    lierre_flood_fill_vars_t *stack, *next_state, *current_state, *stack_limit;
    lierre_pixel_t *row;
    int32_t next_left;

    if (source_color == target_color || decoder->pixels[seed_y * decoder->w + seed_x] != source_color) {
        return;
    }

    stack = decoder->flood_fill_vars;
    stack_limit = &stack[decoder->num_flood_fill_vars - 1];

    next_state = stack;
    next_state->y = seed_y;

    flood_fill_line(decoder, seed_x, next_state->y, source_color, target_color, callback, user_data, &next_left,
                    &next_state->right);
    next_state->left_down = next_left;
    next_state->left_up = next_left;

    for (;;) {
        current_state = next_state;

        if (current_state == stack_limit) {
            break;
        }

        if (current_state->y > 0) {
            row = decoder->pixels + (current_state->y - 1) * decoder->w;
            next_state =
                flood_fill_scan_next(decoder, row, source_color, target_color, callback, user_data, current_state, -1);
            if (next_state) {
                continue;
            }
        }

        if (current_state->y < decoder->h - 1) {
            row = decoder->pixels + (current_state->y + 1) * decoder->w;
            next_state =
                flood_fill_scan_next(decoder, row, source_color, target_color, callback, user_data, current_state, 1);
            if (next_state) {
                continue;
            }
        }

        if (current_state > stack) {
            next_state = current_state - 1;
            continue;
        }

        break;
    }
}

static inline void region_area_callback(void *user_data, int32_t y, int32_t left, int32_t right)
{
    (void)y;

    lierre_region_t *region;

    region = (lierre_region_t *)user_data;
    region->count += right - left + 1;
}

int32_t get_or_create_region(lierre_decoder_t *decoder, int32_t x, int32_t y)
{
    lierre_pixel_t pixel;
    lierre_region_t *region_data;
    int32_t region_id;

    if (x < 0 || y < 0 || x >= decoder->w || y >= decoder->h) {
        return -1;
    }

    pixel = decoder->pixels[y * decoder->w + x];

    if (pixel >= LIERRE_PIXEL_REGION) {
        return (int32_t)pixel;
    }

    if (pixel == LIERRE_PIXEL_WHITE) {
        return -1;
    }

    if (decoder->num_regions >= LIERRE_DECODER_MAX_REGIONS) {
        return -1;
    }

    region_id = decoder->num_regions;
    region_data = &decoder->regions[decoder->num_regions++];

    lmemset(region_data, 0, sizeof(*region_data));
    region_data->seed.x = x;
    region_data->seed.y = y;
    region_data->capstone = -1;

    flood_fill_seed(decoder, x, y, pixel, (lierre_pixel_t)region_id, region_area_callback, region_data);

    return region_id;
}

static inline void find_farthest_corner_callback(void *user_data, int32_t y, int32_t left, int32_t right)
{
    corner_finder_data_t *finder;
    int32_t x_coords[2], delta_y, delta_x, distance_squared, i;

    finder = (corner_finder_data_t *)user_data;
    x_coords[0] = left;
    x_coords[1] = right;
    delta_y = y - finder->reference.y;

    for (i = 0; i < 2; i++) {
        delta_x = x_coords[i] - finder->reference.x;
        distance_squared = delta_x * delta_x + delta_y * delta_y;

        if (distance_squared > finder->scores[0]) {
            finder->scores[0] = distance_squared;
            finder->corners[0].x = x_coords[i];
            finder->corners[0].y = y;
        }
    }
}

static inline void find_remaining_corners_callback(void *user_data, int32_t y, int32_t left, int32_t right)
{
    corner_finder_data_t *finder;
    int32_t x_coords[2], up_score, right_score, scores[4], i, j;

    finder = (corner_finder_data_t *)user_data;
    x_coords[0] = left;
    x_coords[1] = right;

    for (i = 0; i < 2; i++) {
        up_score = x_coords[i] * finder->reference.x + y * finder->reference.y;
        right_score = x_coords[i] * -finder->reference.y + y * finder->reference.x;
        scores[0] = up_score;
        scores[1] = right_score;
        scores[2] = -up_score;
        scores[3] = -right_score;

        for (j = 0; j < 4; j++) {
            if (scores[j] > finder->scores[j]) {
                finder->scores[j] = scores[j];
                finder->corners[j].x = x_coords[i];
                finder->corners[j].y = y;
            }
        }
    }
}

void find_region_corners(lierre_decoder_t *decoder, int32_t region_id, const lierre_decoder_point_t *reference,
                         lierre_decoder_point_t *corners)
{
    lierre_region_t *region;
    corner_finder_data_t finder = {0};
    int32_t i;

    region = &decoder->regions[region_id];

    finder.corners = corners;
    finder.reference = *reference;
    finder.scores[0] = -1;

    flood_fill_seed(decoder, region->seed.x, region->seed.y, (lierre_pixel_t)region_id, LIERRE_PIXEL_BLACK,
                    find_farthest_corner_callback, &finder);

    finder.reference.x = finder.corners[0].x - reference->x;
    finder.reference.y = finder.corners[0].y - reference->y;

    for (i = 0; i < 4; i++) {
        corners[i] = region->seed;
    }

    i = region->seed.x * finder.reference.x + region->seed.y * finder.reference.y;
    finder.scores[0] = i;
    finder.scores[2] = -i;
    i = region->seed.x * -finder.reference.y + region->seed.y * finder.reference.x;
    finder.scores[1] = i;
    finder.scores[3] = -i;

    flood_fill_seed(decoder, region->seed.x, region->seed.y, LIERRE_PIXEL_BLACK, (lierre_pixel_t)region_id,
                    find_remaining_corners_callback, &finder);
}

static inline void record_capstone(lierre_decoder_t *decoder, int32_t ring_region_id, int32_t stone_region_id)
{
    lierre_region_t *stone_region, *ring_region;
    lierre_capstone_t *capstone;
    int32_t capstone_index;

    if (decoder->num_capstones >= LIERRE_DECODER_MAX_CAPSTONES) {
        return;
    }

    stone_region = &decoder->regions[stone_region_id];
    ring_region = &decoder->regions[ring_region_id];

    capstone_index = decoder->num_capstones;
    capstone = &decoder->capstones[decoder->num_capstones++];

    lmemset(capstone, 0, sizeof(*capstone));
    capstone->qr_grid = -1;
    capstone->ring = ring_region_id;
    capstone->stone = stone_region_id;
    stone_region->capstone = capstone_index;
    ring_region->capstone = capstone_index;

    find_region_corners(decoder, ring_region_id, &stone_region->seed, capstone->corners);
    perspective_setup(capstone->c, capstone->corners, FINDER_PATTERN_SIZE, FINDER_PATTERN_SIZE);
    perspective_map(capstone->c, FINDER_PATTERN_CENTER, FINDER_PATTERN_CENTER, &capstone->center);
}

static inline void test_capstone(lierre_decoder_t *decoder, uint32_t x, uint32_t y, uint32_t *pattern_widths)
{
    lierre_region_t *stone_region_data, *ring_region_data;
    uint32_t area_ratio;
    int32_t ring_right_region, stone_region, ring_left_region;

    ring_right_region = get_or_create_region(decoder, (int32_t)(x - pattern_widths[4]), (int32_t)y);
    stone_region = get_or_create_region(
        decoder, (int32_t)(x - pattern_widths[4] - pattern_widths[3] - pattern_widths[2]), (int32_t)y);
    ring_left_region = get_or_create_region(decoder,
                                            (int32_t)(x - pattern_widths[4] - pattern_widths[3] - pattern_widths[2] -
                                                      pattern_widths[1] - pattern_widths[0]),
                                            (int32_t)y);

    if (ring_left_region < 0 || ring_right_region < 0 || stone_region < 0) {
        return;
    }

    if (ring_left_region != ring_right_region) {
        return;
    }

    if (ring_left_region == stone_region) {
        return;
    }

    stone_region_data = &decoder->regions[stone_region];
    ring_region_data = &decoder->regions[ring_left_region];

    if (stone_region_data->capstone >= 0 || ring_region_data->capstone >= 0) {
        return;
    }

    area_ratio = (stone_region_data->count * CAPSTONE_AREA_RATIO_FACTOR) / ring_region_data->count;
    if (area_ratio < CAPSTONE_AREA_RATIO_MIN || area_ratio > CAPSTONE_AREA_RATIO_MAX) {
        return;
    }

    record_capstone(decoder, ring_left_region, stone_region);
}

void scan_finder_patterns(lierre_decoder_t *decoder, uint32_t y)
{
    lierre_pixel_t *row, current_color;
    uint32_t x, previous_color, run_length, run_count, pattern_widths[5] = {0}, average_width, tolerance, i;
    int32_t is_valid, scale_factor;

    row = decoder->pixels + y * decoder->w;
    previous_color = 0;
    run_length = 0;
    run_count = 0;

    for (x = 0; x < (uint32_t)decoder->w; x++) {
        current_color = row[x] ? 1 : 0;

        if (x && current_color != previous_color) {
            lmemmove(pattern_widths, pattern_widths + 1, sizeof(pattern_widths[0]) * (FINDER_PATTERN_MODULES - 1));
            pattern_widths[FINDER_PATTERN_MODULES - 1] = run_length;
            run_length = 0;
            run_count++;

            if (!current_color && run_count >= FINDER_PATTERN_MODULES) {
                scale_factor = FINDER_PATTERN_SCALE_FACTOR;
                average_width = (pattern_widths[0] + pattern_widths[1] + pattern_widths[3] + pattern_widths[4]) *
                                scale_factor / FINDER_TOLERANCE_DIVISOR;
                tolerance = average_width * FINDER_TOLERANCE_MULTIPLIER / FINDER_TOLERANCE_DIVISOR;
                is_valid = 1;

                if ((pattern_widths[0] * scale_factor < average_width - tolerance ||
                     pattern_widths[0] * scale_factor > average_width + tolerance) ||
                    (pattern_widths[1] * scale_factor < average_width - tolerance ||
                     pattern_widths[1] * scale_factor > average_width + tolerance) ||
                    (pattern_widths[2] * scale_factor < FINDER_PATTERN_CENTER_RATIO * average_width - tolerance ||
                     pattern_widths[2] * scale_factor > FINDER_PATTERN_CENTER_RATIO * average_width + tolerance) ||
                    (pattern_widths[3] * scale_factor < average_width - tolerance ||
                     pattern_widths[3] * scale_factor > average_width + tolerance) ||
                    (pattern_widths[4] * scale_factor < average_width - tolerance ||
                     pattern_widths[4] * scale_factor > average_width + tolerance)) {
                    is_valid = 0;
                }

                if (is_valid) {
                    test_capstone(decoder, x, y, pattern_widths);
                }
            }
        }

        run_length++;
        previous_color = current_color;
    }
}

void find_capstone_groups(lierre_decoder_t *decoder, int32_t capstone_index)
{
    lierre_capstone_t *current_capstone, *other_capstone;
    capstone_neighbour_list_t horizontal_list, vertical_list;
    capstone_neighbour_t *neighbour;
    int32_t j;
    double grid_u, grid_v;

    current_capstone = &decoder->capstones[capstone_index];
    horizontal_list.count = 0;
    vertical_list.count = 0;

    for (j = 0; j < decoder->num_capstones; j++) {
        if (capstone_index == j) {
            continue;
        }

        other_capstone = &decoder->capstones[j];
        perspective_unmap(current_capstone->c, &other_capstone->center, &grid_u, &grid_v);

        grid_u = fabs(grid_u - FINDER_PATTERN_CENTER);
        grid_v = fabs(grid_v - FINDER_PATTERN_CENTER);

        if (grid_u < NEIGHBOR_ALIGNMENT_THRESHOLD * grid_v) {
            neighbour = &horizontal_list.entries[horizontal_list.count++];
            neighbour->index = j;
            neighbour->distance = grid_v;
        }

        if (grid_v < NEIGHBOR_ALIGNMENT_THRESHOLD * grid_u) {
            neighbour = &vertical_list.entries[vertical_list.count++];
            neighbour->index = j;
            neighbour->distance = grid_u;
        }
    }

    if (horizontal_list.count && vertical_list.count) {
        test_neighbour_pairs(decoder, capstone_index, &horizontal_list, &vertical_list);
    }
}

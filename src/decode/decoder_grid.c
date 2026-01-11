/*
 * liblierre - decoder_grid.c
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "../internal/decoder.h"

#define QR_VERSION_MIN               1
#define QR_VERSION1_SIZE             17
#define QR_VERSION_SIZE_INCREMENT    4
#define QR_VERSION_ESTIMATION_OFFSET 15.0
#define QR_VERSION2_MIN_SIZE         21

#define FINDER_PATTERN_SIZE   7
#define FINDER_PATTERN_CENTER 3

#define TIMING_PATTERN_POSITION 6
#define TIMING_PATTERN_OFFSET   7
#define TIMING_PATTERN_MARGIN   14

#define PERSPECTIVE_ADJUSTMENT_FACTOR 0.02
#define PERSPECTIVE_STEP_DECAY        0.5
#define PERSPECTIVE_REFINEMENT_PASSES 5
#define PERSPECTIVE_PARAM_ITERATIONS  16

#define CELL_SAMPLE_COUNT    3
#define CELL_SAMPLE_OFFSET_1 0.3
#define CELL_SAMPLE_OFFSET_2 0.5
#define CELL_SAMPLE_OFFSET_3 0.7
#define CELL_CENTER_OFFSET   0.5

#define ROUNDING_OFFSET 0.5
#define AVERAGE_FACTOR  0.5
#define AVERAGE_DIVISOR 2.0

#define ALIGNMENT_RING_RADIUS_1 1
#define ALIGNMENT_RING_RADIUS_2 2
#define ALIGNMENT_RING_RADIUS_3 3

#define ALIGNMENT_SEARCH_AREA_FACTOR 100
#define ALIGNMENT_SIZE_FACTOR_MIN    2
#define ALIGNMENT_SIZE_FACTOR_MAX    2
#define SPIRAL_DIRECTION_COUNT       4

#define NUM_CORNERS   4
#define NUM_CAPSTONES 3

#define SQUARENESS_THRESHOLD 0.2

const version_info_t lierre_version_db[LIERRE_DECODER_MAX_VERSION + 1] = {
    {0, {0}, {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}},
    {26, {0}, {{26, 16, 1}, {26, 19, 1}, {26, 9, 1}, {26, 13, 1}}},
    {44, {6, 18, 0}, {{44, 28, 1}, {44, 34, 1}, {44, 16, 1}, {44, 22, 1}}},
    {70, {6, 22, 0}, {{70, 44, 1}, {70, 55, 1}, {35, 13, 2}, {35, 17, 2}}},
    {100, {6, 26, 0}, {{50, 32, 2}, {100, 80, 1}, {25, 9, 4}, {50, 24, 2}}},
    {134, {6, 30, 0}, {{67, 43, 2}, {134, 108, 1}, {33, 11, 2}, {33, 15, 2}}},
    {172, {6, 34, 0}, {{43, 27, 4}, {86, 68, 2}, {43, 15, 4}, {43, 19, 4}}},
    {196, {6, 22, 38, 0}, {{49, 31, 4}, {98, 78, 2}, {39, 13, 4}, {32, 14, 2}}},
    {242, {6, 24, 42, 0}, {{60, 38, 2}, {121, 97, 2}, {40, 14, 4}, {40, 18, 4}}},
    {292, {6, 26, 46, 0}, {{58, 36, 3}, {146, 116, 2}, {36, 12, 4}, {36, 16, 4}}},
    {346, {6, 28, 50, 0}, {{69, 43, 4}, {86, 68, 2}, {43, 15, 6}, {43, 19, 6}}},
    {404, {6, 30, 54, 0}, {{80, 50, 1}, {101, 81, 4}, {36, 12, 3}, {50, 22, 4}}},
    {466, {6, 32, 58, 0}, {{58, 36, 6}, {116, 92, 2}, {42, 14, 7}, {46, 20, 4}}},
    {532, {6, 34, 62, 0}, {{59, 37, 8}, {133, 107, 4}, {33, 11, 12}, {44, 20, 8}}},
    {581, {6, 26, 46, 66, 0}, {{64, 40, 4}, {145, 115, 3}, {36, 12, 11}, {36, 16, 11}}},
    {655, {6, 26, 48, 70, 0}, {{65, 41, 5}, {109, 87, 5}, {36, 12, 11}, {54, 24, 5}}},
    {733, {6, 26, 50, 74, 0}, {{73, 45, 7}, {122, 98, 5}, {45, 15, 3}, {43, 19, 15}}},
    {815, {6, 30, 54, 78, 0}, {{74, 46, 10}, {135, 107, 1}, {42, 14, 2}, {50, 22, 1}}},
    {901, {6, 30, 56, 82, 0}, {{69, 43, 9}, {150, 120, 5}, {42, 14, 2}, {50, 22, 17}}},
    {991, {6, 30, 58, 86, 0}, {{70, 44, 3}, {141, 113, 3}, {39, 13, 9}, {47, 21, 17}}},
    {1085, {6, 34, 62, 90, 0}, {{67, 41, 3}, {135, 107, 3}, {43, 15, 15}, {54, 24, 15}}},
    {1156, {6, 28, 50, 72, 94, 0}, {{68, 42, 17}, {144, 116, 4}, {46, 16, 19}, {50, 22, 17}}},
    {1258, {6, 26, 50, 74, 98, 0}, {{74, 46, 17}, {139, 111, 2}, {37, 13, 34}, {54, 24, 7}}},
    {1364, {6, 30, 54, 78, 102, 0}, {{75, 47, 4}, {151, 121, 4}, {45, 15, 16}, {54, 24, 11}}},
    {1474, {6, 28, 54, 80, 106, 0}, {{73, 45, 6}, {147, 117, 6}, {46, 16, 30}, {54, 24, 11}}},
    {1588, {6, 32, 58, 84, 110, 0}, {{75, 47, 8}, {132, 106, 8}, {45, 15, 22}, {54, 24, 7}}},
    {1706, {6, 30, 58, 86, 114, 0}, {{74, 46, 19}, {142, 114, 10}, {46, 16, 33}, {50, 22, 28}}},
    {1828, {6, 34, 62, 90, 118, 0}, {{73, 45, 22}, {152, 122, 8}, {45, 15, 12}, {53, 23, 8}}},
    {1921, {6, 26, 50, 74, 98, 122, 0}, {{73, 45, 3}, {147, 117, 3}, {45, 15, 11}, {54, 24, 4}}},
    {2051, {6, 30, 54, 78, 102, 126, 0}, {{73, 45, 21}, {146, 116, 7}, {45, 15, 19}, {53, 23, 1}}},
    {2185, {6, 26, 52, 78, 104, 130, 0}, {{75, 47, 19}, {145, 115, 5}, {45, 15, 23}, {54, 24, 15}}},
    {2323, {6, 30, 56, 82, 108, 134, 0}, {{74, 46, 2}, {145, 115, 13}, {45, 15, 23}, {54, 24, 42}}},
    {2465, {6, 34, 60, 86, 112, 138, 0}, {{74, 46, 10}, {145, 115, 17}, {45, 15, 19}, {54, 24, 10}}},
    {2611, {6, 30, 58, 86, 114, 142, 0}, {{74, 46, 14}, {145, 115, 17}, {45, 15, 11}, {54, 24, 29}}},
    {2761, {6, 34, 62, 90, 118, 146, 0}, {{74, 46, 14}, {145, 115, 13}, {46, 16, 59}, {54, 24, 44}}},
    {2876, {6, 30, 54, 78, 102, 126, 150, 0}, {{75, 47, 12}, {151, 121, 12}, {45, 15, 22}, {54, 24, 39}}},
    {3034, {6, 24, 50, 76, 102, 128, 154, 0}, {{75, 47, 6}, {151, 121, 6}, {45, 15, 2}, {54, 24, 46}}},
    {3196, {6, 28, 54, 80, 106, 132, 158, 0}, {{74, 46, 29}, {152, 122, 17}, {45, 15, 24}, {54, 24, 49}}},
    {3362, {6, 32, 58, 84, 110, 136, 162, 0}, {{74, 46, 13}, {152, 122, 4}, {45, 15, 42}, {54, 24, 48}}},
    {3532, {6, 26, 54, 82, 110, 138, 166, 0}, {{75, 47, 40}, {147, 117, 20}, {45, 15, 10}, {54, 24, 43}}},
    {3706, {6, 30, 58, 86, 114, 142, 170, 0}, {{75, 47, 18}, {148, 118, 19}, {45, 15, 20}, {54, 24, 34}}}};

extern void perspective_map(const double *coeffs, double u, double v, decoder_point_t *result)
{
    double denominator, x, y;

    denominator = 1.0 / (coeffs[6] * u + coeffs[7] * v + 1.0);
    x = (coeffs[0] * u + coeffs[1] * v + coeffs[2]) * denominator;
    y = (coeffs[3] * u + coeffs[4] * v + coeffs[5]) * denominator;

    result->x = (int32_t)(x + ROUNDING_OFFSET);
    result->y = (int32_t)(y + ROUNDING_OFFSET);
}

extern void perspective_setup(double *coeffs, const decoder_point_t *corners, double width, double height)
{
    double x0, y0, x1, y1, x2, y2, x3, y3, width_denominator, height_denominator;

    x0 = corners[0].x;
    y0 = corners[0].y;
    x1 = corners[1].x;
    y1 = corners[1].y;
    x2 = corners[2].x;
    y2 = corners[2].y;
    x3 = corners[3].x;
    y3 = corners[3].y;

    width_denominator = 1.0 / (width * (x2 * y3 - x3 * y2 + (x3 - x2) * y1 + x1 * (y2 - y3)));
    height_denominator = 1.0 / (height * (x2 * y3 + x1 * (y2 - y3) - x3 * y2 + (x3 - x2) * y1));

    coeffs[0] = (x1 * (x2 * y3 - x3 * y2) + x0 * (-x2 * y3 + x3 * y2 + (x2 - x3) * y1) + x1 * (x3 - x2) * y0) *
                width_denominator;
    coeffs[1] = -(x0 * (x2 * y3 + x1 * (y2 - y3) - x2 * y1) - x1 * x3 * y2 + x2 * x3 * y1 + (x1 * x3 - x2 * x3) * y0) *
                height_denominator;
    coeffs[2] = x0;
    coeffs[3] = (y0 * (x1 * (y3 - y2) - x2 * y3 + x3 * y2) + y1 * (x2 * y3 - x3 * y2) + x0 * y1 * (y2 - y3)) *
                width_denominator;
    coeffs[4] = (x0 * (y1 * y3 - y2 * y3) + x1 * y2 * y3 - x2 * y1 * y3 + y0 * (x3 * y2 - x1 * y2 + (x2 - x3) * y1)) *
                height_denominator;
    coeffs[5] = y0;
    coeffs[6] = (x1 * (y3 - y2) + x0 * (y2 - y3) + (x2 - x3) * y1 + (x3 - x2) * y0) * width_denominator;
    coeffs[7] = (-x2 * y3 + x1 * y3 + x3 * y2 + x0 * (y1 - y2) - x3 * y1 + (x2 - x1) * y0) * height_denominator;
}

extern void perspective_unmap(const double *coeffs, const decoder_point_t *image_point, double *grid_u, double *grid_v)
{
    double x, y, denominator;

    x = (double)image_point->x;
    y = (double)image_point->y;
    denominator =
        1.0 / (-coeffs[0] * coeffs[7] * y + coeffs[1] * coeffs[6] * y +
               (coeffs[3] * coeffs[7] - coeffs[4] * coeffs[6]) * x + coeffs[0] * coeffs[4] - coeffs[1] * coeffs[3]);

    *grid_u = -(coeffs[1] * (y - coeffs[5]) - coeffs[2] * coeffs[7] * y + (coeffs[5] * coeffs[7] - coeffs[4]) * x +
                coeffs[2] * coeffs[4]) *
              denominator;
    *grid_v = (coeffs[0] * (y - coeffs[5]) - coeffs[2] * coeffs[6] * y + (coeffs[5] * coeffs[6] - coeffs[3]) * x +
               coeffs[2] * coeffs[3]) *
              denominator;
}

static inline int32_t compute_line_intersection(const decoder_point_t *line1_start, const decoder_point_t *line1_end,
                                                const decoder_point_t *line2_start, const decoder_point_t *line2_end,
                                                decoder_point_t *intersection)
{
    int32_t a, b, c, d, e, f, determinant;

    a = -(line1_end->y - line1_start->y);
    b = line1_end->x - line1_start->x;
    c = -(line2_end->y - line2_start->y);
    d = line2_end->x - line2_start->x;
    e = a * line1_end->x + b * line1_end->y;
    f = c * line2_end->x + d * line2_end->y;

    determinant = a * d - b * c;
    if (!determinant) {
        return 0;
    }

    intersection->x = (d * e - b * f) / determinant;
    intersection->y = (-c * e + a * f) / determinant;

    return 1;
}

static inline double compute_point_distance(const decoder_point_t *point_a, const decoder_point_t *point_b)
{
    double delta_x, delta_y;

    delta_x = (double)abs(point_a->x - point_b->x) + 1.0;
    delta_y = (double)abs(point_a->y - point_b->y) + 1.0;

    return sqrt(delta_x * delta_x + delta_y * delta_y);
}

static inline void estimate_grid_size(decoder_t *decoder, int32_t grid_index)
{
    grid_t *grid;
    capstone_t *cap_a, *cap_b, *cap_c;
    int32_t version;
    double distance_ab, distance_bc, capstone_ab_size, capstone_bc_size, vertical_modules, horizontal_modules,
        average_modules;

    grid = &decoder->grids[grid_index];
    cap_a = &decoder->capstones[grid->caps[0]];
    cap_b = &decoder->capstones[grid->caps[1]];
    cap_c = &decoder->capstones[grid->caps[2]];

    distance_ab = compute_point_distance(&cap_b->corners[0], &cap_a->corners[3]);
    capstone_ab_size = (compute_point_distance(&cap_b->corners[0], &cap_b->corners[3]) +
                        compute_point_distance(&cap_a->corners[0], &cap_a->corners[3])) /
                       AVERAGE_DIVISOR;
    vertical_modules = (double)FINDER_PATTERN_SIZE * distance_ab / capstone_ab_size;

    distance_bc = compute_point_distance(&cap_b->corners[0], &cap_c->corners[1]);
    capstone_bc_size = (compute_point_distance(&cap_b->corners[0], &cap_b->corners[1]) +
                        compute_point_distance(&cap_c->corners[0], &cap_c->corners[1])) /
                       AVERAGE_DIVISOR;
    horizontal_modules = (double)FINDER_PATTERN_SIZE * distance_bc / capstone_bc_size;

    average_modules = (vertical_modules + horizontal_modules) * AVERAGE_FACTOR;
    version = (int32_t)((average_modules - QR_VERSION_ESTIMATION_OFFSET) / (double)QR_VERSION_SIZE_INCREMENT);

    if (version < QR_VERSION_MIN) {
        version = QR_VERSION_MIN;
    }

    if (version > LIERRE_DECODER_MAX_VERSION) {
        version = LIERRE_DECODER_MAX_VERSION;
    }

    grid->grid_size = QR_VERSION_SIZE_INCREMENT * version + QR_VERSION1_SIZE;
}

static inline int32_t read_grid_cell(const decoder_t *decoder, int32_t grid_index, int32_t x, int32_t y)
{
    const grid_t *grid;
    decoder_point_t image_point;

    grid = &decoder->grids[grid_index];
    perspective_map(grid->c, (double)x + CELL_CENTER_OFFSET, (double)y + CELL_CENTER_OFFSET, &image_point);

    if (image_point.y < 0 || image_point.y >= decoder->h || image_point.x < 0 || image_point.x >= decoder->w) {
        return 0;
    }

    return decoder->pixels[image_point.y * decoder->w + image_point.x] ? 1 : -1;
}

static inline int32_t compute_cell_fitness(const decoder_t *decoder, int32_t grid_index, int32_t x, int32_t y)
{
    static const double sample_offsets[CELL_SAMPLE_COUNT] = {CELL_SAMPLE_OFFSET_1, CELL_SAMPLE_OFFSET_2,
                                                             CELL_SAMPLE_OFFSET_3};
    const grid_t *grid;
    decoder_point_t image_point;
    int32_t score, sample_x, sample_y;

    grid = &decoder->grids[grid_index];
    score = 0;

    for (sample_y = 0; sample_y < CELL_SAMPLE_COUNT; sample_y++) {
        for (sample_x = 0; sample_x < CELL_SAMPLE_COUNT; sample_x++) {
            perspective_map(grid->c, (double)x + sample_offsets[sample_x], (double)y + sample_offsets[sample_y],
                            &image_point);

            if (image_point.y < 0 || image_point.y >= decoder->h || image_point.x < 0 || image_point.x >= decoder->w) {
                continue;
            }

            if (decoder->pixels[image_point.y * decoder->w + image_point.x]) {
                score++;
            } else {
                score--;
            }
        }
    }

    return score;
}

static inline int32_t compute_ring_fitness(const decoder_t *decoder, int32_t grid_index, int32_t center_x,
                                           int32_t center_y, int32_t radius)
{
    int32_t i, score;

    score = 0;

    for (i = 0; i < radius * 2; i++) {
        score += compute_cell_fitness(decoder, grid_index, center_x - radius + i, center_y - radius);
        score += compute_cell_fitness(decoder, grid_index, center_x - radius, center_y + radius - i);
        score += compute_cell_fitness(decoder, grid_index, center_x + radius, center_y - radius + i);
        score += compute_cell_fitness(decoder, grid_index, center_x + radius - i, center_y + radius);
    }

    return score;
}

static inline int32_t compute_alignment_pattern_fitness(const decoder_t *decoder, int32_t grid_index, int32_t center_x,
                                                        int32_t center_y)
{
    return compute_cell_fitness(decoder, grid_index, center_x, center_y) -
           compute_ring_fitness(decoder, grid_index, center_x, center_y, ALIGNMENT_RING_RADIUS_1) +
           compute_ring_fitness(decoder, grid_index, center_x, center_y, ALIGNMENT_RING_RADIUS_2);
}

static inline int32_t compute_capstone_fitness(const decoder_t *decoder, int32_t grid_index, int32_t x, int32_t y)
{
    x += FINDER_PATTERN_CENTER;
    y += FINDER_PATTERN_CENTER;

    return compute_cell_fitness(decoder, grid_index, x, y) +
           compute_ring_fitness(decoder, grid_index, x, y, ALIGNMENT_RING_RADIUS_1) -
           compute_ring_fitness(decoder, grid_index, x, y, ALIGNMENT_RING_RADIUS_2) +
           compute_ring_fitness(decoder, grid_index, x, y, ALIGNMENT_RING_RADIUS_3);
}

static inline int32_t compute_total_grid_fitness(const decoder_t *decoder, int32_t grid_index)
{
    const grid_t *grid;
    const version_info_t *version_info;
    int32_t version, score, i, j, alignment_count, expected_value;

    grid = &decoder->grids[grid_index];
    version = (grid->grid_size - QR_VERSION1_SIZE) / QR_VERSION_SIZE_INCREMENT;

    if (version < QR_VERSION_MIN || version > LIERRE_DECODER_MAX_VERSION) {
        return 0;
    }

    version_info = &lierre_version_db[version];
    score = 0;

    for (i = 0; i < grid->grid_size - TIMING_PATTERN_MARGIN; i++) {
        expected_value = (i & 1) ? 1 : -1;
        score += compute_cell_fitness(decoder, grid_index, i + TIMING_PATTERN_OFFSET, TIMING_PATTERN_POSITION) *
                 expected_value;
        score += compute_cell_fitness(decoder, grid_index, TIMING_PATTERN_POSITION, i + TIMING_PATTERN_OFFSET) *
                 expected_value;
    }

    score += compute_capstone_fitness(decoder, grid_index, 0, 0);
    score += compute_capstone_fitness(decoder, grid_index, grid->grid_size - FINDER_PATTERN_SIZE, 0);
    score += compute_capstone_fitness(decoder, grid_index, 0, grid->grid_size - FINDER_PATTERN_SIZE);

    alignment_count = 0;
    while (alignment_count < LIERRE_DECODER_MAX_ALIGNMENT && version_info->apat[alignment_count]) {
        alignment_count++;
    }

    for (i = 1; i + 1 < alignment_count; i++) {
        score += compute_alignment_pattern_fitness(decoder, grid_index, TIMING_PATTERN_POSITION, version_info->apat[i]);
        score += compute_alignment_pattern_fitness(decoder, grid_index, version_info->apat[i], TIMING_PATTERN_POSITION);
    }

    for (i = 1; i < alignment_count; i++) {
        for (j = 1; j < alignment_count; j++) {
            score +=
                compute_alignment_pattern_fitness(decoder, grid_index, version_info->apat[i], version_info->apat[j]);
        }
    }

    return score;
}

static inline void refine_perspective(decoder_t *decoder, int32_t grid_index)
{
    grid_t *grid;
    int32_t best_fitness, pass, i, j, test_fitness;
    double adjustment_steps[LIERRE_DECODER_PERSPECTIVE_PARAMS], original_value, step, new_value;

    grid = &decoder->grids[grid_index];
    best_fitness = compute_total_grid_fitness(decoder, grid_index);

    for (i = 0; i < LIERRE_DECODER_PERSPECTIVE_PARAMS; i++) {
        adjustment_steps[i] = grid->c[i] * PERSPECTIVE_ADJUSTMENT_FACTOR;
    }

    for (pass = 0; pass < PERSPECTIVE_REFINEMENT_PASSES; pass++) {
        for (i = 0; i < PERSPECTIVE_PARAM_ITERATIONS; i++) {
            j = i >> 1;
            original_value = grid->c[j];
            step = adjustment_steps[j];
            new_value = (i & 1) ? original_value + step : original_value - step;

            grid->c[j] = new_value;
            test_fitness = compute_total_grid_fitness(decoder, grid_index);

            if (test_fitness > best_fitness) {
                best_fitness = test_fitness;
            } else {
                grid->c[j] = original_value;
            }
        }

        for (i = 0; i < LIERRE_DECODER_PERSPECTIVE_PARAMS; i++) {
            adjustment_steps[i] *= PERSPECTIVE_STEP_DECAY;
        }
    }
}

static inline void find_leftmost_point_callback(void *user_data, int32_t y, int32_t left, int32_t right)
{
    corner_finder_data_t *finder;
    int32_t x_coords[2], i, distance;

    finder = (corner_finder_data_t *)user_data;
    x_coords[0] = left;
    x_coords[1] = right;

    for (i = 0; i < 2; i++) {
        distance = -finder->reference.y * x_coords[i] + finder->reference.x * y;
        if (distance < finder->scores[0]) {
            finder->scores[0] = distance;
            finder->corners[0].x = x_coords[i];
            finder->corners[0].y = y;
        }
    }
}

static inline void search_alignment_pattern(decoder_t *decoder, int32_t grid_index)
{
    static const int32_t direction_dx[SPIRAL_DIRECTION_COUNT] = {1, 0, -1, 0},
                         direction_dy[SPIRAL_DIRECTION_COUNT] = {0, -1, 0, 1};
    grid_t *grid;
    capstone_t *capstone_a, *capstone_c;
    decoder_point_t point_a, search_point, point_c;
    region_t *region;
    int32_t expected_size, step_size, direction, i, region_id;
    double u, v;

    grid = &decoder->grids[grid_index];
    capstone_a = &decoder->capstones[grid->caps[0]];
    capstone_c = &decoder->capstones[grid->caps[2]];

    search_point = grid->align;

    perspective_unmap(capstone_a->c, &search_point, &u, &v);
    perspective_map(capstone_a->c, u, v + 1.0, &point_a);
    perspective_unmap(capstone_c->c, &search_point, &u, &v);
    perspective_map(capstone_c->c, u + 1.0, v, &point_c);

    expected_size = abs((point_a.x - search_point.x) * -(point_c.y - search_point.y) +
                        (point_a.y - search_point.y) * (point_c.x - search_point.x));

    step_size = 1;
    direction = 0;

    while (step_size * step_size < expected_size * ALIGNMENT_SEARCH_AREA_FACTOR) {
        for (i = 0; i < step_size; i++) {
            region_id = get_or_create_region(decoder, search_point.x, search_point.y);

            if (region_id >= 0) {
                region = &decoder->regions[region_id];
                if (region->count >= expected_size / ALIGNMENT_SIZE_FACTOR_MIN &&
                    region->count <= expected_size * ALIGNMENT_SIZE_FACTOR_MAX) {
                    grid->align_region = region_id;
                    return;
                }
            }

            search_point.x += direction_dx[direction];
            search_point.y += direction_dy[direction];
        }

        direction = (direction + 1) % SPIRAL_DIRECTION_COUNT;
        if (!(direction & 1)) {
            step_size++;
        }
    }
}

static inline void setup_grid_perspective(decoder_t *decoder, int32_t grid_index)
{
    grid_t *grid;
    decoder_point_t corner_points[4];

    grid = &decoder->grids[grid_index];

    corner_points[0] = decoder->capstones[grid->caps[1]].corners[0];
    corner_points[1] = decoder->capstones[grid->caps[2]].corners[0];
    corner_points[2] = grid->align;
    corner_points[3] = decoder->capstones[grid->caps[0]].corners[0];

    perspective_setup(grid->c, corner_points, (double)(grid->grid_size - FINDER_PATTERN_SIZE),
                      (double)(grid->grid_size - FINDER_PATTERN_SIZE));
    refine_perspective(decoder, grid_index);
}

static inline void rotate_capstone_corners(capstone_t *capstone, const decoder_point_t *origin,
                                           const decoder_point_t *direction)
{
    decoder_point_t rotated_corners[NUM_CORNERS], *corner;
    int32_t j, best_index, score, best_score;

    best_index = 0;
    best_score = INT32_MAX;

    for (j = 0; j < NUM_CORNERS; j++) {
        corner = &capstone->corners[j];
        score = (corner->x - origin->x) * -direction->y + (corner->y - origin->y) * direction->x;

        if (score < best_score) {
            best_index = j;
            best_score = score;
        }
    }

    for (j = 0; j < NUM_CORNERS; j++) {
        rotated_corners[j] = capstone->corners[(j + best_index) % NUM_CORNERS];
    }

    lmemcpy(capstone->corners, rotated_corners, sizeof(capstone->corners));
    perspective_setup(capstone->c, capstone->corners, (double)FINDER_PATTERN_SIZE, (double)FINDER_PATTERN_SIZE);
}

static inline void create_qr_grid(decoder_t *decoder, int32_t cap_a, int32_t cap_b, int32_t cap_c)
{
    decoder_point_t origin, direction;
    grid_t *grid;
    capstone_t *capstone;
    region_t *region;
    corner_finder_data_t finder;
    int32_t i, grid_index, temp;

    if (decoder->num_grids >= LIERRE_DECODER_MAX_GRIDS) {
        return;
    }

    origin = decoder->capstones[cap_a].center;
    direction.x = decoder->capstones[cap_c].center.x - decoder->capstones[cap_a].center.x;
    direction.y = decoder->capstones[cap_c].center.y - decoder->capstones[cap_a].center.y;

    if ((decoder->capstones[cap_b].center.x - origin.x) * -direction.y +
            (decoder->capstones[cap_b].center.y - origin.y) * direction.x >
        0) {
        temp = cap_a;
        cap_a = cap_c;
        cap_c = temp;
        direction.x = -direction.x;
        direction.y = -direction.y;
    }

    grid_index = decoder->num_grids;
    grid = &decoder->grids[decoder->num_grids++];

    lmemset(grid, 0, sizeof(*grid));
    grid->caps[0] = cap_a;
    grid->caps[1] = cap_b;
    grid->caps[2] = cap_c;
    grid->align_region = -1;

    for (i = 0; i < NUM_CAPSTONES; i++) {
        capstone = &decoder->capstones[grid->caps[i]];
        rotate_capstone_corners(capstone, &origin, &direction);
        capstone->qr_grid = grid_index;
    }

    estimate_grid_size(decoder, grid_index);

    if (!compute_line_intersection(&decoder->capstones[cap_a].corners[0], &decoder->capstones[cap_a].corners[1],
                                   &decoder->capstones[cap_c].corners[0], &decoder->capstones[cap_c].corners[3],
                                   &grid->align)) {
        for (i = 0; i < NUM_CAPSTONES; i++) {
            decoder->capstones[grid->caps[i]].qr_grid = -1;
        }

        decoder->num_grids--;

        return;
    }

    if (grid->grid_size > QR_VERSION2_MIN_SIZE) {
        search_alignment_pattern(decoder, grid_index);

        if (grid->align_region >= 0) {
            region = &decoder->regions[grid->align_region];
            grid->align = region->seed;

            lmemset(&finder, 0, sizeof(finder));
            finder.reference = direction;
            finder.corners = &grid->align;
            finder.scores[0] = -direction.y * grid->align.x + direction.x * grid->align.y;

            flood_fill_seed(decoder, region->seed.x, region->seed.y, (lierre_pixel_t)grid->align_region,
                            LIERRE_PIXEL_BLACK, NULL, NULL);
            flood_fill_seed(decoder, region->seed.x, region->seed.y, LIERRE_PIXEL_BLACK,
                            (lierre_pixel_t)grid->align_region, find_leftmost_point_callback, &finder);
        }
    }

    setup_grid_perspective(decoder, grid_index);
}

void extract_qr_code(const decoder_t *decoder, int32_t grid_index, qr_code_t *code)
{
    const grid_t *grid;
    int32_t row, col, bit_index;

    lmemset(code, 0, sizeof(*code));

    if (grid_index < 0 || grid_index >= decoder->num_grids) {
        return;
    }

    grid = &decoder->grids[grid_index];

    perspective_map(grid->c, 0.0, 0.0, &code->corners[0]);
    perspective_map(grid->c, (double)grid->grid_size, 0.0, &code->corners[1]);
    perspective_map(grid->c, (double)grid->grid_size, (double)grid->grid_size, &code->corners[2]);
    perspective_map(grid->c, 0.0, (double)grid->grid_size, &code->corners[3]);

    code->size = grid->grid_size;

    if (code->size > LIERRE_DECODER_MAX_GRID_SIZE) {
        return;
    }

    bit_index = 0;
    for (row = 0; row < grid->grid_size; row++) {
        for (col = 0; col < grid->grid_size; col++) {
            if (read_grid_cell(decoder, grid_index, col, row) > 0) {
                code->cell_bitmap[bit_index >> 3] |= (1 << (bit_index & 7));
            }
            bit_index++;
        }
    }
}

extern void test_neighbour_pairs(decoder_t *decoder, int32_t capstone_index,
                                 const capstone_neighbour_list_t *horizontal_list,
                                 const capstone_neighbour_list_t *vertical_list)
{
    const capstone_neighbour_t *horizontal_neighbour, *vertical_neighbour;
    int32_t j, k;
    double squareness;

    for (j = 0; j < horizontal_list->count; j++) {
        horizontal_neighbour = &horizontal_list->entries[j];
        for (k = 0; k < vertical_list->count; k++) {
            vertical_neighbour = &vertical_list->entries[k];
            squareness = fabs(1.0 - horizontal_neighbour->distance / vertical_neighbour->distance);

            if (squareness < SQUARENESS_THRESHOLD) {
                create_qr_grid(decoder, horizontal_neighbour->index, capstone_index, vertical_neighbour->index);
            }
        }
    }
}

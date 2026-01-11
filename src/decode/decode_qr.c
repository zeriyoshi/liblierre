/*
 * liblierre - decode_qr.c
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <poporon/bch.h>

#include "../internal/decoder.h"

#define FORMAT_BITS_COUNT      15
#define FORMAT_XOR_MASK        0x5412
#define FORMAT_DATA_SHIFT      10
#define FORMAT_ECC_LEVEL_SHIFT 3
#define FORMAT_MASK_BITS       7

#define FORMAT_BCH_SYMBOL_SIZE  4
#define FORMAT_BCH_GEN_POLY     0x13
#define FORMAT_BCH_CORRECTION_T 3

#define FINDER_PATTERN_SIZE        7
#define FINDER_EDGE_SIZE           8
#define FINDER_CENTER              9
#define TIMING_PATTERN_POSITION    6
#define VERSION_INFO_OFFSET        11
#define VERSION_INFO_SIZE          6
#define QR_VERSION1_SIZE           17
#define LIERRE_QR_VERSION_INFO_MIN 7

#define MODE_NUMERIC      1
#define MODE_ALPHANUMERIC 2
#define MODE_BYTE         4
#define MODE_ECI          7
#define MODE_KANJI        8

#define ALPHANUMERIC_CHARSET_SIZE 45

#define KANJI_ENCODE_DIVISOR 0xc0
#define KANJI_SJIS_BASE1     0x8140
#define KANJI_SJIS_BOUNDARY  0x9ffc
#define KANJI_SJIS_BASE2     0xc140

#define VERSION_THRESHOLD_SMALL  10
#define VERSION_THRESHOLD_MEDIUM 27

#define NUMERIC_BITS_SMALL  10
#define NUMERIC_BITS_MEDIUM 12
#define NUMERIC_BITS_LARGE  14

#define ALPHA_BITS_SMALL   9
#define ALPHA_BITS_MEDIUM  11
#define ALPHA_BITS_LARGE   13
#define BYTE_BITS_SMALL    8
#define BYTE_BITS_LARGE    16
#define KANJI_BITS_SMALL   8
#define KANJI_BITS_MEDIUM  10
#define KANJI_BITS_LARGE   12
#define KANJI_ENCODED_BITS 13

static inline int32_t grid_bit(const qr_code_t *code, int32_t x, int32_t y)
{
    int32_t bit_position;

    bit_position = y * code->size + x;
    return (code->cell_bitmap[bit_position >> 3] >> (bit_position & 7)) & 1;
}

static inline lierre_error_t correct_format_bits(uint16_t *format_bits)
{
    poporon_bch_t *bch;
    uint32_t corrected;

    bch = poporon_bch_create(FORMAT_BCH_SYMBOL_SIZE, FORMAT_BCH_GEN_POLY, FORMAT_BCH_CORRECTION_T);
    if (!bch) {
        return LIERRE_ERROR_FORMAT_ECC;
    }

    if (!poporon_bch_decode(bch, (uint32_t)*format_bits, &corrected, NULL)) {
        poporon_bch_destroy(bch);
        return LIERRE_ERROR_FORMAT_ECC;
    }

    poporon_bch_destroy(bch);

    *format_bits = (uint16_t)corrected;

    return LIERRE_ERROR_SUCCESS;
}

static inline lierre_error_t read_format(const qr_code_t *code, qr_data_t *data, int32_t use_secondary)
{
    static const int32_t primary_x_positions[FORMAT_BITS_COUNT] = {8, 8, 8, 8, 8, 8, 8, 8, 7, 5, 4, 3, 2, 1, 0},
                         primary_y_positions[FORMAT_BITS_COUNT] = {0, 1, 2, 3, 4, 5, 7, 8, 8, 8, 8, 8, 8, 8, 8};
    lierre_error_t err;
    uint16_t format_bits, format_data;
    int32_t i;

    format_bits = 0;

    if (use_secondary) {
        for (i = 0; i < FINDER_PATTERN_SIZE; i++) {
            format_bits = (uint16_t)((format_bits << 1) | grid_bit(code, FINDER_EDGE_SIZE, code->size - 1 - i));
        }

        for (i = 0; i < FINDER_EDGE_SIZE; i++) {
            format_bits =
                (uint16_t)((format_bits << 1) | grid_bit(code, code->size - FINDER_EDGE_SIZE + i, FINDER_EDGE_SIZE));
        }
    } else {
        for (i = FORMAT_BITS_COUNT - 1; i >= 0; i--) {
            format_bits =
                (uint16_t)((format_bits << 1) | grid_bit(code, primary_x_positions[i], primary_y_positions[i]));
        }
    }

    format_bits ^= FORMAT_XOR_MASK;

    err = correct_format_bits(&format_bits);
    if (err) {
        return err;
    }

    format_data = (uint16_t)(format_bits >> FORMAT_DATA_SHIFT);
    data->ecc_level = format_data >> FORMAT_ECC_LEVEL_SHIFT;
    data->mask = format_data & FORMAT_MASK_BITS;

    return LIERRE_ERROR_SUCCESS;
}

static inline int32_t mask_bit(int32_t mask_pattern, int32_t row, int32_t col)
{
    switch (mask_pattern) {
    case 0:
        return !((row + col) % 2);
    case 1:
        return !(row % 2);
    case 2:
        return !(col % 3);
    case 3:
        return !((row + col) % 3);
    case 4:
        return !(((row / 2) + (col / 3)) % 2);
    case 5:
        return !((row * col) % 2 + (row * col) % 3);
    case 6:
        return !(((row * col) % 2 + (row * col) % 3) % 2);
    case 7:
        return !(((row * col) % 3 + (row + col) % 2) % 2);
    }

    return 0;
}

static inline int32_t is_reserved_cell(int32_t version, int32_t row, int32_t col)
{
    const version_info_t *version_info;
    int32_t size, alignment_row_idx, alignment_col_idx, alignment_idx, pos;

    version_info = &lierre_version_db[version];
    size = version * 4 + QR_VERSION1_SIZE;
    alignment_row_idx = -1;
    alignment_col_idx = -1;

    if (row < FINDER_CENTER && col < FINDER_CENTER) {
        return 1;
    }

    if (row + FINDER_EDGE_SIZE >= size && col < FINDER_CENTER) {
        return 1;
    }

    if (row < FINDER_CENTER && col + FINDER_EDGE_SIZE >= size) {
        return 1;
    }

    if (row == TIMING_PATTERN_POSITION || col == TIMING_PATTERN_POSITION) {
        return 1;
    }

    if (version >= LIERRE_QR_VERSION_INFO_MIN) {
        if (row < TIMING_PATTERN_POSITION && col + VERSION_INFO_OFFSET >= size) {
            return 1;
        }

        if (row + VERSION_INFO_OFFSET >= size && col < TIMING_PATTERN_POSITION) {
            return 1;
        }
    }

    for (alignment_idx = 0; alignment_idx < LIERRE_DECODER_MAX_ALIGNMENT && version_info->apat[alignment_idx];
         alignment_idx++) {
        pos = version_info->apat[alignment_idx];

        if ((pos - row) >= -2 && (pos - row) <= 2) {
            alignment_row_idx = alignment_idx;
        }

        if ((pos - col) >= -2 && (pos - col) <= 2) {
            alignment_col_idx = alignment_idx;
        }
    }

    if (alignment_row_idx >= 0 && alignment_col_idx >= 0) {
        alignment_idx--;

        if (alignment_row_idx > 0 && alignment_row_idx < alignment_idx) {
            return 1;
        }

        if (alignment_col_idx > 0 && alignment_col_idx < alignment_idx) {
            return 1;
        }

        if (alignment_col_idx == alignment_idx && alignment_row_idx == alignment_idx) {
            return 1;
        }
    }

    return 0;
}

static inline void read_bit(const qr_code_t *code, qr_data_t *data, datastream_t *ds, int32_t row, int32_t col)
{
    int32_t bit_offset, byte_offset, value;

    bit_offset = ds->data_bits & 7;
    byte_offset = ds->data_bits >> 3;
    value = grid_bit(code, col, row);

    if (mask_bit(data->mask, row, col)) {
        value ^= 1;
    }

    if (value) {
        ds->raw[byte_offset] |= (0x80 >> bit_offset);
    }

    ds->data_bits++;
}

static inline void read_data(const qr_code_t *code, qr_data_t *data, datastream_t *ds)
{
    int32_t row, col, direction;

    row = code->size - 1;
    col = code->size - 1;
    direction = -1;

    while (col > 0) {
        if (col == 6) {
            col--;
        }

        if (!is_reserved_cell(data->version, row, col)) {
            read_bit(code, data, ds, row, col);
        }

        if (!is_reserved_cell(data->version, row, col - 1)) {
            read_bit(code, data, ds, row, col - 1);
        }

        row += direction;
        if (row < 0 || row >= code->size) {
            direction = -direction;
            col -= 2;
            row += direction;
        }
    }
}

static inline lierre_error_t correct_block_with_poporon(uint8_t *block_data, const rs_params_t *ecc_params)
{
    poporon_t *rs_decoder;
    int32_t parity_bytes;
    size_t corrected;
    bool success;

    parity_bytes = ecc_params->bs - ecc_params->dw;

    rs_decoder = poporon_create(8, 0x11D, 0, 1, (uint8_t)parity_bytes);
    if (!rs_decoder) {
        return LIERRE_ERROR_DATA_ECC;
    }

    success =
        poporon_decode_u8(rs_decoder, block_data, (size_t)ecc_params->dw, &block_data[ecc_params->dw], &corrected);
    poporon_destroy(rs_decoder);

    if (!success) {
        return LIERRE_ERROR_DATA_ECC;
    }

    return LIERRE_ERROR_SUCCESS;
}

static inline lierre_error_t codestream_ecc(qr_data_t *data, datastream_t *ds)
{
    const version_info_t *version_info;
    const rs_params_t *short_block_ecc, *current_ecc;
    rs_params_t long_block_ecc;
    lierre_error_t err;
    uint8_t *dst;
    int32_t long_block_count, total_blocks, ecc_offset, dst_offset, block_idx, byte_idx, parity_count;

    version_info = &lierre_version_db[data->version];
    short_block_ecc = &version_info->ecc[data->ecc_level];
    long_block_count =
        (version_info->data_bytes - short_block_ecc->bs * short_block_ecc->ns) / (short_block_ecc->bs + 1);
    total_blocks = long_block_count + short_block_ecc->ns;
    ecc_offset = short_block_ecc->dw * total_blocks + long_block_count;
    dst_offset = 0;

    lmemcpy(&long_block_ecc, short_block_ecc, sizeof(long_block_ecc));
    long_block_ecc.dw++;
    long_block_ecc.bs++;

    for (block_idx = 0; block_idx < total_blocks; block_idx++) {
        dst = ds->data + dst_offset;
        current_ecc = (block_idx < short_block_ecc->ns) ? short_block_ecc : &long_block_ecc;
        parity_count = current_ecc->bs - current_ecc->dw;

        for (byte_idx = 0; byte_idx < current_ecc->dw; byte_idx++) {
            dst[byte_idx] = ds->raw[byte_idx * total_blocks + block_idx];
        }

        for (byte_idx = 0; byte_idx < parity_count; byte_idx++) {
            dst[current_ecc->dw + byte_idx] = ds->raw[ecc_offset + byte_idx * total_blocks + block_idx];
        }

        err = correct_block_with_poporon(dst, current_ecc);
        if (err) {
            return err;
        }

        dst_offset += current_ecc->dw;
    }

    ds->data_bits = dst_offset * 8;

    return LIERRE_ERROR_SUCCESS;
}

static inline int32_t bits_remaining(const datastream_t *ds)
{
    return ds->data_bits - ds->ptr;
}

static inline int32_t take_bits(datastream_t *ds, int32_t count)
{
    int32_t result, bit_position;
    uint8_t byte_value;

    result = 0;

    while (count && (ds->ptr < ds->data_bits)) {
        byte_value = ds->data[ds->ptr >> 3];
        bit_position = ds->ptr & 7;

        result <<= 1;
        if ((byte_value << bit_position) & 0x80) {
            result |= 1;
        }

        ds->ptr++;
        count--;
    }

    return result;
}

static inline int32_t decode_numeric_tuple(qr_data_t *data, datastream_t *ds, int32_t bits, int32_t digits)
{
    int32_t value, i;

    if (bits_remaining(ds) < bits) {
        return -1;
    }

    value = take_bits(ds, bits);

    for (i = digits - 1; i >= 0; i--) {
        data->payload[data->payload_len + i] = (uint8_t)(value % 10 + '0');
        value /= 10;
    }

    data->payload_len += digits;

    return 0;
}

static inline lierre_error_t decode_numeric(qr_data_t *data, datastream_t *ds)
{
    int32_t count_bits, char_count;

    count_bits = NUMERIC_BITS_LARGE;

    if (data->version < VERSION_THRESHOLD_SMALL) {
        count_bits = NUMERIC_BITS_SMALL;
    } else if (data->version < VERSION_THRESHOLD_MEDIUM) {
        count_bits = NUMERIC_BITS_MEDIUM;
    }

    char_count = take_bits(ds, count_bits);
    if (data->payload_len + char_count + 1 > LIERRE_DECODER_MAX_PAYLOAD) {
        return LIERRE_ERROR_DATA_OVERFLOW;
    }

    while (char_count >= 3) {
        if (decode_numeric_tuple(data, ds, 10, 3) < 0) {
            return LIERRE_ERROR_DATA_UNDERFLOW;
        }

        char_count -= 3;
    }

    if (char_count >= 2) {
        if (decode_numeric_tuple(data, ds, 7, 2) < 0) {
            return LIERRE_ERROR_DATA_UNDERFLOW;
        }

        char_count -= 2;
    }

    if (char_count) {
        if (decode_numeric_tuple(data, ds, 4, 1) < 0) {
            return LIERRE_ERROR_DATA_UNDERFLOW;
        }
    }

    return LIERRE_ERROR_SUCCESS;
}

static inline int32_t decode_alpha_tuple(qr_data_t *data, datastream_t *ds, int32_t bits, int32_t digits)
{
    static const char *ALPHANUMERIC_CHARSET = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";
    int32_t value, i;

    if (bits_remaining(ds) < bits) {
        return -1;
    }

    value = take_bits(ds, bits);

    for (i = 0; i < digits; i++) {
        data->payload[data->payload_len + digits - i - 1] =
            (uint8_t)ALPHANUMERIC_CHARSET[value % ALPHANUMERIC_CHARSET_SIZE];
        value /= ALPHANUMERIC_CHARSET_SIZE;
    }

    data->payload_len += digits;

    return 0;
}

static inline lierre_error_t decode_alpha(qr_data_t *data, datastream_t *ds)
{
    int32_t count_bits, char_count;

    count_bits = ALPHA_BITS_LARGE;

    if (data->version < VERSION_THRESHOLD_SMALL) {
        count_bits = ALPHA_BITS_SMALL;
    } else if (data->version < VERSION_THRESHOLD_MEDIUM) {
        count_bits = ALPHA_BITS_MEDIUM;
    }

    char_count = take_bits(ds, count_bits);
    if (data->payload_len + char_count + 1 > LIERRE_DECODER_MAX_PAYLOAD) {
        return LIERRE_ERROR_DATA_OVERFLOW;
    }

    while (char_count >= 2) {
        if (decode_alpha_tuple(data, ds, 11, 2) < 0) {
            return LIERRE_ERROR_DATA_UNDERFLOW;
        }

        char_count -= 2;
    }

    if (char_count) {
        if (decode_alpha_tuple(data, ds, 6, 1) < 0) {
            return LIERRE_ERROR_DATA_UNDERFLOW;
        }
    }

    return LIERRE_ERROR_SUCCESS;
}

static inline lierre_error_t decode_byte(qr_data_t *data, datastream_t *ds)
{
    int32_t count_bits, char_count, i;

    count_bits = BYTE_BITS_LARGE;

    if (data->version < VERSION_THRESHOLD_SMALL) {
        count_bits = BYTE_BITS_SMALL;
    }

    char_count = take_bits(ds, count_bits);
    if (data->payload_len + char_count + 1 > LIERRE_DECODER_MAX_PAYLOAD) {
        return LIERRE_ERROR_DATA_OVERFLOW;
    }

    if (bits_remaining(ds) < char_count * 8) {
        return LIERRE_ERROR_DATA_UNDERFLOW;
    }

    for (i = 0; i < char_count; i++) {
        data->payload[data->payload_len++] = (uint8_t)take_bits(ds, 8);
    }

    return LIERRE_ERROR_SUCCESS;
}

static inline lierre_error_t decode_kanji(qr_data_t *data, datastream_t *ds)
{
    int32_t count_bits, char_count, i, encoded_value, high_byte, low_byte, intermediate;
    uint16_t sjis_char;

    count_bits = KANJI_BITS_LARGE;

    if (data->version < VERSION_THRESHOLD_SMALL) {
        count_bits = KANJI_BITS_SMALL;
    } else if (data->version < VERSION_THRESHOLD_MEDIUM) {
        count_bits = KANJI_BITS_MEDIUM;
    }

    char_count = take_bits(ds, count_bits);
    if (data->payload_len + char_count * 2 + 1 > LIERRE_DECODER_MAX_PAYLOAD) {
        return LIERRE_ERROR_DATA_OVERFLOW;
    }

    if (bits_remaining(ds) < char_count * KANJI_ENCODED_BITS) {
        return LIERRE_ERROR_DATA_UNDERFLOW;
    }

    for (i = 0; i < char_count; i++) {
        encoded_value = take_bits(ds, KANJI_ENCODED_BITS);
        high_byte = encoded_value / KANJI_ENCODE_DIVISOR;
        low_byte = encoded_value % KANJI_ENCODE_DIVISOR;
        intermediate = (high_byte << 8) | low_byte;

        if (intermediate + KANJI_SJIS_BASE1 <= KANJI_SJIS_BOUNDARY) {
            sjis_char = (uint16_t)(intermediate + KANJI_SJIS_BASE1);
        } else {
            sjis_char = (uint16_t)(intermediate + KANJI_SJIS_BASE2);
        }

        data->payload[data->payload_len++] = (uint8_t)(sjis_char >> 8);
        data->payload[data->payload_len++] = (uint8_t)(sjis_char & 0xff);
    }

    return LIERRE_ERROR_SUCCESS;
}

static inline lierre_error_t decode_eci(qr_data_t *data, datastream_t *ds)
{
    if (bits_remaining(ds) < 8) {
        return LIERRE_ERROR_DATA_UNDERFLOW;
    }

    data->eci = (uint32_t)take_bits(ds, 8);

    if ((data->eci & 0xc0) == 0x80) {
        if (bits_remaining(ds) < 8) {
            return LIERRE_ERROR_DATA_UNDERFLOW;
        }

        data->eci = (data->eci << 8) | (uint32_t)take_bits(ds, 8);
    } else if ((data->eci & 0xe0) == 0xc0) {
        if (bits_remaining(ds) < 16) {
            return LIERRE_ERROR_DATA_UNDERFLOW;
        }

        data->eci = (data->eci << 16) | (uint32_t)take_bits(ds, 16);
    }

    return LIERRE_ERROR_SUCCESS;
}

static inline lierre_error_t decode_payload(qr_data_t *data, datastream_t *ds)
{
    lierre_error_t err;
    int32_t mode_indicator;

    while (bits_remaining(ds) >= 4) {
        err = LIERRE_ERROR_SUCCESS;
        mode_indicator = take_bits(ds, 4);

        switch (mode_indicator) {
        case MODE_NUMERIC:
            err = decode_numeric(data, ds);
            break;
        case MODE_ALPHANUMERIC:
            err = decode_alpha(data, ds);
            break;
        case MODE_BYTE:
            err = decode_byte(data, ds);
            break;
        case MODE_KANJI:
            err = decode_kanji(data, ds);
            break;
        case MODE_ECI:
            err = decode_eci(data, ds);
            break;
        default:
            goto done;
        }

        if (err) {
            return err;
        }

        if (!(mode_indicator & (mode_indicator - 1)) && (mode_indicator > data->data_type)) {
            data->data_type = mode_indicator;
        }
    }

done:
    if (data->payload_len >= LIERRE_DECODER_MAX_PAYLOAD) {
        data->payload_len = LIERRE_DECODER_MAX_PAYLOAD - 1;
    }

    data->payload[data->payload_len] = 0;

    return LIERRE_ERROR_SUCCESS;
}

lierre_error_t decode_qr(const qr_code_t *code, qr_data_t *data)
{
    lierre_error_t err;
    datastream_t ds = {0};

    if (code->size > LIERRE_DECODER_MAX_GRID_SIZE) {
        return LIERRE_ERROR_INVALID_GRID_SIZE;
    }

    if ((code->size - QR_VERSION1_SIZE) % 4) {
        return LIERRE_ERROR_INVALID_GRID_SIZE;
    }

    lmemset(data, 0, sizeof(*data));

    data->version = (code->size - QR_VERSION1_SIZE) / 4;

    if (data->version < 1 || data->version > LIERRE_DECODER_MAX_VERSION) {
        return LIERRE_ERROR_INVALID_VERSION;
    }

    err = read_format(code, data, 0);
    if (err) {
        err = read_format(code, data, 1);
    }
    if (err) {
        return err;
    }

    ds.raw = data->payload;

    read_data(code, data, &ds);
    err = codestream_ecc(data, &ds);
    if (err) {
        return err;
    }

    ds.raw = NULL;

    err = decode_payload(data, &ds);
    if (err) {
        return err;
    }

    return LIERRE_ERROR_SUCCESS;
}

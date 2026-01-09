/*
 * liblierre - writer.c
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <string.h>

#include <lierre.h>
#include <lierre/writer.h>

#include <poporon.h>

#include "../internal/memory.h"
#include "../internal/structs.h"

#define LIERRE_QR_VERSION_MAX             40
#define LIERRE_QR_VERSION_MIN             1
#define LIERRE_QR_VERSION1_SIZE           17
#define LIERRE_QR_VERSION_SIZE_FORMULA(v) ((v) * 4 + LIERRE_QR_VERSION1_SIZE)

#define RS_GF256_PRIMITIVE_POLY 0x11D
#define RS_GF256_GENERATOR_ROOT 0x1
#define RS_BYTE_BITS            7

#define VERSION_INFO_MIN         7
#define VERSION_INFO_BITS        12
#define VERSION_INFO_POLY        0x1F25
#define VERSION_INFO_SHIFT       11
#define VERSION_INFO_DATA_SHIFT  12
#define VERSION_INFO_AREA_WIDTH  3
#define VERSION_INFO_AREA_HEIGHT 6
#define VERSION_INFO_OFFSET      11

#define FORMAT_POLY            0x537
#define FORMAT_XOR_MASK        0x5412
#define FORMAT_POLY_SHIFT      9
#define FORMAT_DATA_SHIFT      10
#define FORMAT_BITS_COUNT      15
#define FORMAT_BITS_LOOP_END   6
#define FORMAT_BITS_LOOP_START 9

#define QR_MODE_NUMERIC_INDICATOR      0x1
#define QR_MODE_ALPHANUMERIC_INDICATOR 0x2
#define QR_MODE_BYTE_INDICATOR         0x4
#define QR_MODE_ECI_INDICATOR          0x7
#define QR_MODE_KANJI_INDICATOR        0x8
#define QR_MODE_INDICATOR_BITS         4
#define QR_TERMINATOR_MAX_BITS         4
#define QR_PAD_BYTE_BITS               8

#define VERSION_THRESHOLD_SMALL  10
#define VERSION_THRESHOLD_MEDIUM 27
#define NUMERIC_BITS_SMALL       10
#define NUMERIC_BITS_MEDIUM      12
#define NUMERIC_BITS_LARGE       14
#define ALPHA_BITS_SMALL         9
#define ALPHA_BITS_MEDIUM        11
#define ALPHA_BITS_LARGE         13
#define BYTE_BITS_SMALL          8
#define BYTE_BITS_LARGE          16
#define KANJI_BITS_SMALL         8
#define KANJI_BITS_MEDIUM        10
#define KANJI_BITS_LARGE         12

#define NUMERIC_GROUP_SIZE      3
#define NUMERIC_GROUP_BITS      10
#define NUMERIC_REMAINDER2_BITS 7
#define NUMERIC_REMAINDER1_BITS 4

#define ALPHANUMERIC_CHARSET_SIZE   45
#define ALPHANUMERIC_GROUP_SIZE     2
#define ALPHANUMERIC_GROUP_BITS     11
#define ALPHANUMERIC_REMAINDER_BITS 6

#define KANJI_ENCODED_BITS      13
#define KANJI_SJIS_RANGE1_START 0x8140
#define KANJI_SJIS_RANGE1_END   0x9FFC
#define KANJI_SJIS_RANGE2_START 0xE040
#define KANJI_SJIS_RANGE2_END   0xEBBF
#define KANJI_ENCODE_BASE1      0x8140
#define KANJI_ENCODE_BASE2      0xC140
#define KANJI_ENCODE_MULTIPLIER 0xC0

#define PAD_BYTE_FIRST  0xEC
#define PAD_BYTE_SECOND 0x11

#define ECI_SINGLE_BYTE_MAX 127
#define ECI_DOUBLE_BYTE_MAX 16383
#define ECI_PREFIX_2BYTE    0x80
#define ECI_PREFIX_3BYTE    0xC0
#define ECI_MASK_2BYTE      0x3F
#define ECI_MASK_3BYTE      0x1F
#define ECI_BITS_3BYTE      24
#define ECI_DEFAULT_VALUE   26

#define QR_MASK_COUNT 8

#define FINDER_PATTERN_CENTER    3
#define FINDER_PATTERN_RADIUS    4
#define FINDER_QUIET_SIZE        8
#define FINDER_CORNER_SIZE       9
#define TIMING_PATTERN_POSITION  6
#define TIMING_PATTERN_START     7
#define ALIGNMENT_PATTERN_SIZE   5
#define ALIGNMENT_PATTERN_OFFSET 2
#define ALIGNMENT_BASE_POSITION  6

#define PENALTY_RUN_THRESHOLD        5
#define PENALTY_RUN_BASE             3
#define PENALTY_FINDER_LIKE          40
#define PENALTY_2X2_BLOCK            3
#define PENALTY_BALANCE_MULTIPLIER   10
#define PENALTY_BALANCE_FACTOR_DARK  20
#define PENALTY_BALANCE_FACTOR_TOTAL 10
#define PENALTY_HISTORY_SIZE         7
#define PENALTY_HISTORY_MOVE_SIZE    6

#define ALPHA_LETTER_OFFSET  10
#define ALPHA_SPACE_VALUE    36
#define ALPHA_DOLLAR_VALUE   37
#define ALPHA_PERCENT_VALUE  38
#define ALPHA_ASTERISK_VALUE 39
#define ALPHA_PLUS_VALUE     40
#define ALPHA_MINUS_VALUE    41
#define ALPHA_DOT_VALUE      42
#define ALPHA_SLASH_VALUE    43
#define ALPHA_COLON_VALUE    44

#define LIERRE_QR_BUFFER_LEN_FOR_VERSION(n) (((((n) * 4 + 17) * ((n) * 4 + 17) + 7) >> 3) + 1)
#define LIERRE_QR_BUFFER_LEN_MAX            LIERRE_QR_BUFFER_LEN_FOR_VERSION(LIERRE_QR_VERSION_MAX)

#define LIERRE_QR_RS_DEGREE_MAX 30

static const int8_t LIERRE_ECC_CODEWORDS_PER_BLOCK[4][41] = {
    {-1, 7,  10, 15, 20, 26, 18, 20, 24, 30, 18, 20, 24, 26, 30, 22, 24, 28, 30, 28, 28,
     28, 28, 30, 30, 26, 28, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},
    {-1, 10, 16, 26, 18, 24, 16, 18, 22, 22, 26, 30, 22, 22, 24, 24, 28, 28, 26, 26, 26,
     26, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28},
    {-1, 13, 22, 18, 26, 18, 24, 18, 22, 20, 24, 28, 26, 24, 20, 30, 24, 28, 28, 26, 30,
     28, 30, 30, 30, 30, 28, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},
    {-1, 17, 28, 22, 16, 22, 28, 26, 26, 24, 28, 24, 28, 22, 24, 24, 30, 28, 28, 26, 28,
     30, 24, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},
};

static const int8_t LIERRE_NUM_ERROR_CORRECTION_BLOCKS[4][41] = {
    {-1, 1, 1, 1,  1,  1,  2,  2,  2,  2,  4,  4,  4,  4,  4,  6,  6,  6,  6,  7, 8,
     8,  9, 9, 10, 12, 12, 12, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 24, 25},
    {-1, 1,  1,  1,  2,  2,  4,  4,  4,  5,  5,  5,  8,  9,  9,  10, 10, 11, 13, 14, 16,
     17, 17, 18, 20, 21, 23, 25, 26, 28, 29, 31, 33, 35, 37, 38, 40, 43, 45, 47, 49},
    {-1, 1,  1,  2,  2,  4,  4,  6,  6,  8,  8,  8,  10, 12, 16, 12, 17, 16, 18, 21, 20,
     23, 23, 25, 27, 29, 34, 34, 35, 38, 40, 43, 45, 48, 51, 53, 56, 59, 62, 65, 68},
    {-1, 1,  1,  2,  4,  4,  4,  5,  6,  8,  8,  11, 11, 16, 16, 18, 16, 19, 21, 25, 25,
     25, 34, 30, 32, 35, 37, 40, 42, 45, 48, 51, 54, 57, 60, 63, 66, 70, 74, 77, 81},
};

static const int16_t LIERRE_DATA_CODEWORDS[4][41] = {
    /* ECL L */
    {-1,   19,   34,   55,   80,   108,  136,  156,  194,  232,  274,  324,  370,  428,
     461,  523,  589,  647,  721,  795,  861,  932,  1006, 1094, 1174, 1276, 1370, 1468,
     1531, 1631, 1735, 1843, 1955, 2071, 2191, 2306, 2434, 2566, 2702, 2812, 2956},
    /* ECL M */
    {-1,   16,   28,   44,   64,   86,   108,  124,  154,  182,  216,  254,  290,  334,
     365,  415,  453,  507,  563,  627,  669,  714,  782,  860,  914,  1000, 1062, 1128,
     1193, 1267, 1373, 1455, 1541, 1631, 1725, 1812, 1914, 1992, 2102, 2216, 2334},
    /* ECL Q */
    {-1,  13,  22,  34,  48,  62,  76,  88,  110, 132, 154,  180,  206,  244,  261,  295,  325,  367,  397,  445, 485,
     512, 568, 614, 664, 718, 754, 808, 871, 911, 985, 1033, 1115, 1171, 1231, 1286, 1354, 1426, 1502, 1582, 1666},
    /* ECL H */
    {-1,  9,   16,  26,  36,  46,  60,  66,  86,  100, 122, 140, 158, 180, 197, 223,  253,  283,  313,  341, 385,
     406, 442, 464, 514, 538, 596, 628, 661, 701, 745, 793, 845, 901, 961, 986, 1054, 1096, 1142, 1222, 1276},
};

static const int16_t LIERRE_CHAR_CAPACITY[4][41][4] = {
    /* ECL L */
    {
        {-1, -1, -1, -1},         /* version 0 (invalid) */
        {41, 25, 17, 10},         /* version 1 */
        {77, 47, 32, 20},         /* version 2 */
        {127, 77, 53, 32},        /* version 3 */
        {187, 114, 78, 48},       /* version 4 */
        {255, 154, 106, 65},      /* version 5 */
        {322, 195, 134, 82},      /* version 6 */
        {370, 224, 154, 95},      /* version 7 */
        {461, 279, 192, 118},     /* version 8 */
        {552, 335, 230, 141},     /* version 9 */
        {652, 395, 271, 167},     /* version 10 */
        {772, 468, 321, 198},     /* version 11 */
        {883, 535, 367, 226},     /* version 12 */
        {1022, 619, 425, 262},    /* version 13 */
        {1101, 667, 458, 282},    /* version 14 */
        {1250, 758, 520, 320},    /* version 15 */
        {1408, 854, 586, 361},    /* version 16 */
        {1548, 938, 644, 397},    /* version 17 */
        {1725, 1046, 718, 442},   /* version 18 */
        {1903, 1153, 792, 488},   /* version 19 */
        {2061, 1249, 858, 528},   /* version 20 */
        {2232, 1352, 929, 572},   /* version 21 */
        {2409, 1460, 1003, 618},  /* version 22 */
        {2620, 1588, 1091, 672},  /* version 23 */
        {2812, 1704, 1171, 721},  /* version 24 */
        {3057, 1853, 1273, 784},  /* version 25 */
        {3283, 1990, 1367, 842},  /* version 26 */
        {3517, 2132, 1465, 902},  /* version 27 */
        {3669, 2223, 1528, 940},  /* version 28 */
        {3909, 2369, 1628, 1002}, /* version 29 */
        {4158, 2520, 1732, 1066}, /* version 30 */
        {4417, 2677, 1840, 1132}, /* version 31 */
        {4686, 2840, 1952, 1201}, /* version 32 */
        {4965, 3009, 2068, 1273}, /* version 33 */
        {5253, 3183, 2188, 1347}, /* version 34 */
        {5529, 3351, 2303, 1417}, /* version 35 */
        {5836, 3537, 2431, 1496}, /* version 36 */
        {6153, 3729, 2563, 1577}, /* version 37 */
        {6479, 3927, 2699, 1661}, /* version 38 */
        {6743, 4087, 2809, 1729}, /* version 39 */
        {7089, 4296, 2953, 1817}, /* version 40 */
    },
    /* ECL M */
    {
        {-1, -1, -1, -1},         /* version 0 (invalid) */
        {34, 20, 14, 8},          /* version 1 */
        {63, 38, 26, 16},         /* version 2 */
        {101, 61, 42, 26},        /* version 3 */
        {149, 90, 62, 38},        /* version 4 */
        {202, 122, 84, 52},       /* version 5 */
        {255, 154, 106, 65},      /* version 6 */
        {293, 178, 122, 75},      /* version 7 */
        {365, 221, 152, 93},      /* version 8 */
        {432, 262, 180, 111},     /* version 9 */
        {513, 311, 213, 131},     /* version 10 */
        {604, 366, 251, 155},     /* version 11 */
        {691, 419, 287, 177},     /* version 12 */
        {796, 483, 331, 204},     /* version 13 */
        {871, 528, 362, 223},     /* version 14 */
        {991, 600, 412, 254},     /* version 15 */
        {1082, 656, 450, 277},    /* version 16 */
        {1212, 734, 504, 310},    /* version 17 */
        {1346, 816, 560, 345},    /* version 18 */
        {1500, 909, 624, 384},    /* version 19 */
        {1600, 970, 666, 410},    /* version 20 */
        {1708, 1035, 711, 438},   /* version 21 */
        {1872, 1134, 779, 480},   /* version 22 */
        {2059, 1248, 857, 528},   /* version 23 */
        {2188, 1326, 911, 561},   /* version 24 */
        {2395, 1451, 997, 614},   /* version 25 */
        {2544, 1542, 1059, 652},  /* version 26 */
        {2701, 1637, 1125, 692},  /* version 27 */
        {2857, 1732, 1190, 732},  /* version 28 */
        {3035, 1839, 1264, 778},  /* version 29 */
        {3289, 1994, 1370, 843},  /* version 30 */
        {3486, 2113, 1452, 894},  /* version 31 */
        {3693, 2238, 1538, 947},  /* version 32 */
        {3909, 2369, 1628, 1002}, /* version 33 */
        {4134, 2506, 1722, 1060}, /* version 34 */
        {4343, 2632, 1809, 1113}, /* version 35 */
        {4588, 2780, 1911, 1176}, /* version 36 */
        {4775, 2894, 1989, 1224}, /* version 37 */
        {5039, 3054, 2099, 1292}, /* version 38 */
        {5313, 3220, 2213, 1362}, /* version 39 */
        {5596, 3391, 2331, 1435}, /* version 40 */
    },
    /* ECL Q */
    {
        {-1, -1, -1, -1},         /* version 0 (invalid) */
        {27, 16, 11, 7},          /* version 1 */
        {48, 29, 20, 12},         /* version 2 */
        {77, 47, 32, 20},         /* version 3 */
        {111, 67, 46, 28},        /* version 4 */
        {144, 87, 60, 37},        /* version 5 */
        {178, 108, 74, 45},       /* version 6 */
        {207, 125, 86, 53},       /* version 7 */
        {259, 157, 108, 66},      /* version 8 */
        {312, 189, 130, 80},      /* version 9 */
        {364, 221, 151, 93},      /* version 10 */
        {427, 259, 177, 109},     /* version 11 */
        {489, 296, 203, 125},     /* version 12 */
        {580, 352, 241, 149},     /* version 13 */
        {621, 376, 258, 159},     /* version 14 */
        {703, 426, 292, 180},     /* version 15 */
        {775, 470, 322, 198},     /* version 16 */
        {876, 531, 364, 224},     /* version 17 */
        {948, 574, 394, 243},     /* version 18 */
        {1063, 644, 442, 272},    /* version 19 */
        {1159, 702, 482, 297},    /* version 20 */
        {1224, 742, 509, 314},    /* version 21 */
        {1358, 823, 565, 348},    /* version 22 */
        {1468, 890, 611, 376},    /* version 23 */
        {1588, 963, 661, 407},    /* version 24 */
        {1718, 1041, 715, 440},   /* version 25 */
        {1804, 1094, 751, 462},   /* version 26 */
        {1933, 1172, 805, 496},   /* version 27 */
        {2085, 1263, 868, 534},   /* version 28 */
        {2181, 1322, 908, 559},   /* version 29 */
        {2358, 1429, 982, 604},   /* version 30 */
        {2473, 1499, 1030, 634},  /* version 31 */
        {2670, 1618, 1112, 684},  /* version 32 */
        {2805, 1700, 1168, 719},  /* version 33 */
        {2949, 1787, 1228, 756},  /* version 34 */
        {3081, 1867, 1283, 790},  /* version 35 */
        {3244, 1966, 1351, 832},  /* version 36 */
        {3417, 2071, 1423, 876},  /* version 37 */
        {3599, 2181, 1499, 923},  /* version 38 */
        {3791, 2298, 1579, 972},  /* version 39 */
        {3993, 2420, 1663, 1024}, /* version 40 */
    },
    /* ECL H */
    {
        {-1, -1, -1, -1},        /* version 0 (invalid) */
        {17, 10, 7, 4},          /* version 1 */
        {34, 20, 14, 8},         /* version 2 */
        {58, 35, 24, 15},        /* version 3 */
        {82, 50, 34, 21},        /* version 4 */
        {106, 64, 44, 27},       /* version 5 */
        {139, 84, 58, 36},       /* version 6 */
        {154, 93, 64, 39},       /* version 7 */
        {202, 122, 84, 52},      /* version 8 */
        {235, 143, 98, 60},      /* version 9 */
        {288, 174, 119, 74},     /* version 10 */
        {331, 200, 137, 85},     /* version 11 */
        {374, 227, 155, 96},     /* version 12 */
        {427, 259, 177, 109},    /* version 13 */
        {468, 283, 194, 120},    /* version 14 */
        {530, 321, 220, 136},    /* version 15 */
        {602, 365, 250, 154},    /* version 16 */
        {674, 408, 280, 173},    /* version 17 */
        {746, 452, 310, 191},    /* version 18 */
        {813, 493, 338, 208},    /* version 19 */
        {919, 557, 382, 235},    /* version 20 */
        {969, 587, 403, 248},    /* version 21 */
        {1056, 640, 439, 270},   /* version 22 */
        {1108, 672, 461, 284},   /* version 23 */
        {1228, 744, 511, 315},   /* version 24 */
        {1286, 779, 535, 330},   /* version 25 */
        {1425, 864, 593, 365},   /* version 26 */
        {1501, 910, 625, 385},   /* version 27 */
        {1581, 958, 658, 405},   /* version 28 */
        {1677, 1016, 698, 430},  /* version 29 */
        {1782, 1080, 742, 457},  /* version 30 */
        {1897, 1150, 790, 486},  /* version 31 */
        {2022, 1226, 842, 518},  /* version 32 */
        {2157, 1307, 898, 553},  /* version 33 */
        {2301, 1394, 958, 590},  /* version 34 */
        {2361, 1431, 983, 605},  /* version 35 */
        {2524, 1530, 1051, 647}, /* version 36 */
        {2625, 1591, 1093, 673}, /* version 37 */
        {2735, 1658, 1139, 701}, /* version 38 */
        {2927, 1774, 1219, 750}, /* version 39 */
        {3057, 1852, 1273, 784}, /* version 40 */
    },
};

static inline void lierre_append_bits(uint32_t val, uint8_t num_bits, uint8_t buffer[], int32_t *bit_len)
{
    int32_t i;

    for (i = num_bits - 1; i >= 0; i--, (*bit_len)++) {
        buffer[*bit_len >> 3] |= ((val >> i) & 1) << (7 - (*bit_len & 7));
    }
}

static inline int32_t lierre_get_num_raw_data_modules(uint8_t ver)
{
    int32_t result, num_align;

    result = (16 * ver + 128) * ver + 64;
    if (ver >= 2) {
        num_align = ver / 7 + 2;
        result -= (25 * num_align - 10) * num_align - 55;
        if (ver >= 7) {
            result -= 36;
        }
    }
    return result;
}

static inline int32_t lierre_get_num_data_codewords(uint8_t version, uint8_t ecl)
{
    if (ecl > 3 || version < 1 || version > 40) {
        return LIERRE_DATA_CODEWORDS[0][version < 1 ? 1 : (version > 40 ? 40 : version)];
    }

    return LIERRE_DATA_CODEWORDS[ecl][version];
}

static inline void lierre_add_ecc_and_interleave(uint8_t data[], uint8_t version, uint8_t ecl, uint8_t result[])
{
    const uint8_t *dat;
    uint8_t num_blocks, block_ecc_len, num_short_blocks, short_block_data_len, *ecc;
    int32_t raw_codewords, data_len, i, j, k, dat_len;
    poporon_t *pprn;

    num_blocks = LIERRE_NUM_ERROR_CORRECTION_BLOCKS[ecl][version];
    block_ecc_len = LIERRE_ECC_CODEWORDS_PER_BLOCK[ecl][version];
    raw_codewords = lierre_get_num_raw_data_modules(version) >> 3;
    data_len = lierre_get_num_data_codewords(version, ecl);
    num_short_blocks = num_blocks - raw_codewords % num_blocks;
    short_block_data_len = raw_codewords / num_blocks - block_ecc_len;

    pprn = poporon_create(8, RS_GF256_PRIMITIVE_POLY, 0, 1, block_ecc_len);
    if (!pprn) {
        return;
    }

    dat = data;

    for (i = 0; i < num_blocks; i++) {
        dat_len = short_block_data_len + (i < num_short_blocks ? 0 : 1);
        ecc = &data[data_len];

        poporon_encode_u8(pprn, (uint8_t *)dat, (size_t)dat_len, ecc);

        for (j = 0, k = i; j < dat_len; j++, k += num_blocks) {
            if (j == short_block_data_len) {
                k -= num_short_blocks;
            }
            result[k] = dat[j];
        }

        for (j = 0, k = data_len + i; j < block_ecc_len; j++, k += num_blocks) {
            result[k] = ecc[j];
        }

        dat += dat_len;
    }

    poporon_destroy(pprn);
}

static inline uint8_t lierre_get_alignment_pattern_positions(uint8_t version, uint8_t result[7])
{
    uint8_t num_align, step, i, pos;

    if (version == 1) {
        return 0;
    }

    num_align = version / 7 + 2;
    step = (version * 8 + num_align * 3 + 5) / (num_align * 4 - 4) * 2;

    for (i = num_align - 1, pos = version * 4 + 10; i >= 1; i--, pos -= step) {
        result[i] = pos;
    }

    result[0] = 6;

    return num_align;
}

static inline void lierre_fill_rectangle(int32_t left, int32_t top, int32_t width, int32_t height, uint8_t qrcode[])
{
    int32_t dx, dy, qrsize, index, byte_idx, bit_idx;

    qrsize = qrcode[0];

    for (dy = 0; dy < height; dy++) {
        for (dx = 0; dx < width; dx++) {
            index = (top + dy) * qrsize + (left + dx);
            byte_idx = (index >> 3) + 1;
            bit_idx = index & 7;
            qrcode[byte_idx] |= 1 << bit_idx;
        }
    }
}

static inline bool lierre_get_module(const uint8_t qrcode[], int32_t x, int32_t y)
{
    int32_t qrsize, index;

    qrsize = qrcode[0];
    if (x < 0 || x >= qrsize || y < 0 || y >= qrsize) {
        return false;
    }

    index = y * qrsize + x;

    return ((qrcode[(index >> 3) + 1] >> (index & 7)) & 1) != 0;
}

static inline void lierre_set_module(uint8_t qrcode[], int32_t x, int32_t y, bool is_dark)
{
    int32_t qrsize, index, bit_idx, byte_idx;

    qrsize = qrcode[0];
    if (x < 0 || x >= qrsize || y < 0 || y >= qrsize) {
        return;
    }

    index = y * qrsize + x;
    bit_idx = index & 7;
    byte_idx = (index >> 3) + 1;

    if (is_dark) {
        qrcode[byte_idx] |= 1 << bit_idx;
    } else {
        qrcode[byte_idx] &= (1 << bit_idx) ^ 0xFF;
    }
}

static inline void lierre_initialize_function_modules(uint8_t version, uint8_t qrcode[])
{
    uint8_t qrsize, align_pat_pos[7], num_align, i, j;

    qrsize = LIERRE_QR_VERSION_SIZE_FORMULA(version);
    lmemset(qrcode, 0, (size_t)(((qrsize * qrsize + 7) >> 3) + 1) * sizeof(qrcode[0]));
    qrcode[0] = qrsize;

    lierre_fill_rectangle(TIMING_PATTERN_POSITION, 0, 1, qrsize, qrcode);
    lierre_fill_rectangle(0, TIMING_PATTERN_POSITION, qrsize, 1, qrcode);

    lierre_fill_rectangle(0, 0, FINDER_CORNER_SIZE, FINDER_CORNER_SIZE, qrcode);
    lierre_fill_rectangle(qrsize - FINDER_QUIET_SIZE, 0, FINDER_QUIET_SIZE, FINDER_CORNER_SIZE, qrcode);
    lierre_fill_rectangle(0, qrsize - FINDER_QUIET_SIZE, FINDER_CORNER_SIZE, FINDER_QUIET_SIZE, qrcode);

    num_align = lierre_get_alignment_pattern_positions(version, align_pat_pos);

    for (i = 0; i < num_align; i++) {
        for (j = 0; j < num_align; j++) {
            if (!((i == 0 && j == 0) || (i == 0 && j == num_align - 1) || (i == num_align - 1 && j == 0))) {
                lierre_fill_rectangle(align_pat_pos[i] - ALIGNMENT_PATTERN_OFFSET,
                                      align_pat_pos[j] - ALIGNMENT_PATTERN_OFFSET, ALIGNMENT_PATTERN_SIZE,
                                      ALIGNMENT_PATTERN_SIZE, qrcode);
            }
        }
    }

    if (version >= VERSION_INFO_MIN) {
        lierre_fill_rectangle(qrsize - VERSION_INFO_OFFSET, 0, VERSION_INFO_AREA_WIDTH, VERSION_INFO_AREA_HEIGHT,
                              qrcode);
        lierre_fill_rectangle(0, qrsize - VERSION_INFO_OFFSET, VERSION_INFO_AREA_HEIGHT, VERSION_INFO_AREA_WIDTH,
                              qrcode);
    }
}

static inline void lierre_draw_light_function_modules(uint8_t qrcode[], uint8_t version)
{
    uint32_t bits;
    uint8_t align_pat_pos[7], num_align;
    int32_t qrsize, i, j, k, dx, dy, dist, rem;

    qrsize = qrcode[0];
    for (i = TIMING_PATTERN_START; i < qrsize - TIMING_PATTERN_START; i += 2) {
        lierre_set_module(qrcode, TIMING_PATTERN_POSITION, i, false);
        lierre_set_module(qrcode, i, TIMING_PATTERN_POSITION, false);
    }

    for (dy = -FINDER_PATTERN_RADIUS; dy <= FINDER_PATTERN_RADIUS; dy++) {
        for (dx = -FINDER_PATTERN_RADIUS; dx <= FINDER_PATTERN_RADIUS; dx++) {
            dist = (dx < 0) ? -dx : dx;

            if ((dy < 0 ? -dy : dy) > dist) {
                dist = (dy < 0) ? -dy : dy;
            }

            if (dist == ALIGNMENT_PATTERN_OFFSET || dist == FINDER_PATTERN_RADIUS) {
                lierre_set_module(qrcode, FINDER_PATTERN_CENTER + dx, FINDER_PATTERN_CENTER + dy, false);
                lierre_set_module(qrcode, qrsize - FINDER_PATTERN_RADIUS + dx, FINDER_PATTERN_CENTER + dy, false);
                lierre_set_module(qrcode, FINDER_PATTERN_CENTER + dx, qrsize - FINDER_PATTERN_RADIUS + dy, false);
            }
        }
    }

    num_align = lierre_get_alignment_pattern_positions(version, align_pat_pos);
    for (i = 0; i < num_align; i++) {
        for (j = 0; j < num_align; j++) {
            if ((i == 0 && j == 0) || (i == 0 && j == num_align - 1) || (i == num_align - 1 && j == 0)) {
                continue;
            }

            for (dy = -1; dy <= 1; dy++) {
                for (dx = -1; dx <= 1; dx++) {
                    lierre_set_module(qrcode, align_pat_pos[i] + dx, align_pat_pos[j] + dy, dx == 0 && dy == 0);
                }
            }
        }
    }

    if (version >= VERSION_INFO_MIN) {
        rem = version;

        for (i = 0; i < VERSION_INFO_BITS; i++) {
            rem = (rem << 1) ^ ((rem >> VERSION_INFO_SHIFT) * VERSION_INFO_POLY);
        }

        bits = (uint32_t)version << VERSION_INFO_DATA_SHIFT | (uint32_t)rem;

        for (i = 0; i < VERSION_INFO_AREA_HEIGHT; i++) {
            for (j = 0; j < VERSION_INFO_AREA_WIDTH; j++) {
                k = qrsize - VERSION_INFO_OFFSET + j;

                lierre_set_module(qrcode, k, i, (bits & 1) != 0);
                lierre_set_module(qrcode, i, k, (bits & 1) != 0);
                bits >>= 1;
            }
        }
    }
}

static inline void lierre_draw_format_bits(uint8_t ecl, int8_t mask, uint8_t qrcode[])
{
    static const int32_t table[] = {1, 0, 3, 2};
    int32_t data, rem, bits_val, qrsize, i;

    data = table[ecl] << 3 | mask;
    rem = data;

    for (i = 0; i < FORMAT_DATA_SHIFT; i++) {
        rem = (rem << 1) ^ ((rem >> FORMAT_POLY_SHIFT) * FORMAT_POLY);
    }

    bits_val = (data << FORMAT_DATA_SHIFT | rem) ^ FORMAT_XOR_MASK;

    for (i = 0; i <= 5; i++) {
        lierre_set_module(qrcode, 8, i, ((bits_val >> i) & 1) != 0);
    }

    lierre_set_module(qrcode, 8, 7, ((bits_val >> 6) & 1) != 0);
    lierre_set_module(qrcode, 8, 8, ((bits_val >> 7) & 1) != 0);
    lierre_set_module(qrcode, 7, 8, ((bits_val >> 8) & 1) != 0);

    for (i = FORMAT_BITS_LOOP_START; i < FORMAT_BITS_COUNT; i++) {
        lierre_set_module(qrcode, 14 - i, 8, ((bits_val >> i) & 1) != 0);
    }

    qrsize = qrcode[0];

    for (i = 0; i < FINDER_QUIET_SIZE; i++) {
        lierre_set_module(qrcode, qrsize - 1 - i, 8, ((bits_val >> i) & 1) != 0);
    }

    for (i = FINDER_QUIET_SIZE; i < FORMAT_BITS_COUNT; i++) {
        lierre_set_module(qrcode, 8, qrsize - FORMAT_BITS_COUNT + i, ((bits_val >> i) & 1) != 0);
    }

    lierre_set_module(qrcode, 8, qrsize - FINDER_QUIET_SIZE, true);
}

static inline void lierre_draw_codewords(const uint8_t data[], int32_t data_len, uint8_t qrcode[])
{
    int32_t qrsize, bit_idx, right, vert, j, x, y;
    bool upward, dark;

    qrsize = qrcode[0];
    bit_idx = 0;

    for (right = qrsize - 1; right >= 1; right -= 2) {
        if (right == 6) {
            right = 5;
        }

        for (vert = 0; vert < qrsize; vert++) {
            for (j = 0; j < 2; j++) {
                x = right - j;
                upward = ((right + 1) & 2) == 0;
                y = upward ? qrsize - 1 - vert : vert;

                if (!lierre_get_module(qrcode, x, y) && bit_idx < data_len * 8) {
                    dark = ((data[bit_idx >> 3] >> (7 - (bit_idx & 7))) & 1) != 0;
                    lierre_set_module(qrcode, x, y, dark);
                    bit_idx++;
                }
            }
        }
    }
}

static inline void lierre_apply_mask(const uint8_t function_modules[], uint8_t qrcode[], int8_t mask)
{
    int32_t qrsize, y, x;
    bool invert, val;

    qrsize = qrcode[0];

    for (y = 0; y < qrsize; y++) {
        for (x = 0; x < qrsize; x++) {
            if (lierre_get_module(function_modules, x, y)) {
                continue;
            }

            switch (mask) {
            case 0:
                invert = (x + y) % 2 == 0;
                break;
            case 1:
                invert = y % 2 == 0;
                break;
            case 2:
                invert = x % 3 == 0;
                break;
            case 3:
                invert = (x + y) % 3 == 0;
                break;
            case 4:
                invert = (x / 3 + y / 2) % 2 == 0;
                break;
            case 5:
                invert = x * y % 2 + x * y % 3 == 0;
                break;
            case 6:
                invert = (x * y % 2 + x * y % 3) % 2 == 0;
                break;
            case 7:
                invert = ((x + y) % 2 + x * y % 3) % 2 == 0;
                break;
            default:
                invert = false;
                break;
            }

            val = lierre_get_module(qrcode, x, y);
            lierre_set_module(qrcode, x, y, val ^ invert);
        }
    }
}

static inline int32_t lierre_finder_penalty_add_history(int32_t current_run_len,
                                                        int32_t run_history[PENALTY_HISTORY_SIZE], int32_t qrsize)
{
    if (run_history[0] == 0) {
        current_run_len += qrsize;
    }

    lmemmove(&run_history[1], &run_history[0], PENALTY_HISTORY_MOVE_SIZE * sizeof(run_history[0]));
    run_history[0] = current_run_len;

    return current_run_len;
}

static inline int32_t lierre_finder_penalty_count_patterns(const int32_t run_history[PENALTY_HISTORY_SIZE],
                                                           int32_t qrsize)
{
    int32_t n;
    bool core;

    n = run_history[1];
    (void)qrsize;
    core = n > 0 && run_history[2] == n && run_history[3] == n * 3 && run_history[4] == n && run_history[5] == n;
    return (core && run_history[0] >= n * 4 && run_history[6] >= n ? 1 : 0) +
           (core && run_history[6] >= n * 4 && run_history[0] >= n ? 1 : 0);
}

static inline int32_t lierre_finder_penalty_terminate(bool current_run_color, int32_t current_run_len,
                                                      int32_t run_history[PENALTY_HISTORY_SIZE], int32_t qrsize)
{
    if (current_run_color) {
        lierre_finder_penalty_add_history(current_run_len, run_history, qrsize);
        current_run_len = 0;
    }

    current_run_len += qrsize;

    lierre_finder_penalty_add_history(current_run_len, run_history, qrsize);

    return lierre_finder_penalty_count_patterns(run_history, qrsize);
}

static inline int32_t lierre_get_penalty_score(const uint8_t qrcode[])
{
    int32_t qrsize, y, x, run_x, run_y, run_history[PENALTY_HISTORY_SIZE], dark, total, k, result;
    bool run_color, color;

    qrsize = qrcode[0];
    result = 0;

    for (y = 0; y < qrsize; y++) {
        run_color = false;
        run_x = 0;
        lmemset(run_history, 0, sizeof(run_history));

        for (x = 0; x < qrsize; x++) {
            if (lierre_get_module(qrcode, x, y) == run_color) {
                run_x++;
                if (run_x == PENALTY_RUN_THRESHOLD) {
                    result += PENALTY_RUN_BASE;
                } else if (run_x > PENALTY_RUN_THRESHOLD) {
                    result++;
                }
            } else {
                lierre_finder_penalty_add_history(run_x, run_history, qrsize);
                if (!run_color) {
                    result += lierre_finder_penalty_count_patterns(run_history, qrsize) * PENALTY_FINDER_LIKE;
                }
                run_color = lierre_get_module(qrcode, x, y);
                run_x = 1;
            }
        }
        result += lierre_finder_penalty_terminate(run_color, run_x, run_history, qrsize) * PENALTY_FINDER_LIKE;
    }

    for (x = 0; x < qrsize; x++) {
        run_color = false;
        run_y = 0;
        lmemset(run_history, 0, sizeof(run_history));

        for (y = 0; y < qrsize; y++) {
            if (lierre_get_module(qrcode, x, y) == run_color) {
                run_y++;
                if (run_y == PENALTY_RUN_THRESHOLD) {
                    result += PENALTY_RUN_BASE;
                } else if (run_y > PENALTY_RUN_THRESHOLD) {
                    result++;
                }
            } else {
                lierre_finder_penalty_add_history(run_y, run_history, qrsize);
                if (!run_color) {
                    result += lierre_finder_penalty_count_patterns(run_history, qrsize) * PENALTY_FINDER_LIKE;
                }
                run_color = lierre_get_module(qrcode, x, y);
                run_y = 1;
            }
        }

        result += lierre_finder_penalty_terminate(run_color, run_y, run_history, qrsize) * PENALTY_FINDER_LIKE;
    }

    for (y = 0; y < qrsize - 1; y++) {
        for (x = 0; x < qrsize - 1; x++) {
            color = lierre_get_module(qrcode, x, y);
            if (color == lierre_get_module(qrcode, x + 1, y) && color == lierre_get_module(qrcode, x, y + 1) &&
                color == lierre_get_module(qrcode, x + 1, y + 1)) {
                result += PENALTY_2X2_BLOCK;
            }
        }
    }

    dark = 0;
    for (y = 0; y < qrsize; y++) {
        for (x = 0; x < qrsize; x++) {
            if (lierre_get_module(qrcode, x, y)) {
                dark++;
            }
        }
    }

    total = qrsize * qrsize;

    k = ((dark * PENALTY_BALANCE_FACTOR_DARK - total * PENALTY_BALANCE_FACTOR_TOTAL) < 0
             ? -(dark * PENALTY_BALANCE_FACTOR_DARK - total * PENALTY_BALANCE_FACTOR_TOTAL)
             : (dark * PENALTY_BALANCE_FACTOR_DARK - total * PENALTY_BALANCE_FACTOR_TOTAL));

    k = (k + total - 1) / total - 1;

    result += k * PENALTY_BALANCE_MULTIPLIER;

    return result;
}

static inline bool lierre_is_numeric_data(const uint8_t *data, size_t data_len)
{
    size_t i;

    for (i = 0; i < data_len; i++) {
        if (data[i] < '0' || data[i] > '9') {
            return false;
        }
    }

    return true;
}

static inline int32_t lierre_numeric_count_bits(uint8_t version, size_t char_count)
{
    int32_t count_bits;

    if (version < VERSION_THRESHOLD_SMALL) {
        count_bits = NUMERIC_BITS_SMALL;
    } else if (version < VERSION_THRESHOLD_MEDIUM) {
        count_bits = NUMERIC_BITS_MEDIUM;
    } else {
        count_bits = NUMERIC_BITS_LARGE;
    }

    return QR_MODE_INDICATOR_BITS + count_bits + ((int32_t)char_count / NUMERIC_GROUP_SIZE) * NUMERIC_GROUP_BITS +
           ((char_count % NUMERIC_GROUP_SIZE == 1)
                ? NUMERIC_REMAINDER1_BITS
                : ((char_count % NUMERIC_GROUP_SIZE == 2) ? NUMERIC_REMAINDER2_BITS : 0));
}

static inline bool lierre_encode_numeric(const uint8_t *data, size_t data_len, uint8_t temp_buffer[], uint8_t qrcode[],
                                         uint8_t ecl, int8_t min_version, int8_t max_version, int8_t mask)
{
    int8_t version, best_mask;
    int32_t data_capacity_bits, data_used_bits, bit_len, terminator_bits, i, min_penalty, penalty;
    int32_t count_bits, value;
    uint8_t pad_byte;
    size_t idx;

    for (version = min_version;; version++) {
        data_capacity_bits = lierre_get_num_data_codewords(version, ecl) * 8;
        data_used_bits = lierre_numeric_count_bits(version, data_len);

        if (data_used_bits <= data_capacity_bits) {
            break;
        }

        if (version >= max_version) {
            qrcode[0] = 0;
            return false;
        }
    }

    if (version < VERSION_THRESHOLD_SMALL) {
        count_bits = NUMERIC_BITS_SMALL;
    } else if (version < VERSION_THRESHOLD_MEDIUM) {
        count_bits = NUMERIC_BITS_MEDIUM;
    } else {
        count_bits = NUMERIC_BITS_LARGE;
    }

    lmemset(qrcode, 0, (size_t)LIERRE_QR_BUFFER_LEN_FOR_VERSION(version) * sizeof(qrcode[0]));
    bit_len = 0;
    lierre_append_bits(QR_MODE_NUMERIC_INDICATOR, QR_MODE_INDICATOR_BITS, qrcode, &bit_len);
    lierre_append_bits((uint32_t)data_len, count_bits, qrcode, &bit_len);

    idx = 0;
    while (idx + NUMERIC_GROUP_SIZE <= data_len) {
        value = (data[idx] - '0') * 100 + (data[idx + 1] - '0') * 10 + (data[idx + 2] - '0');
        lierre_append_bits((uint32_t)value, NUMERIC_GROUP_BITS, qrcode, &bit_len);
        idx += NUMERIC_GROUP_SIZE;
    }

    if (data_len - idx == 2) {
        value = (data[idx] - '0') * 10 + (data[idx + 1] - '0');
        lierre_append_bits((uint32_t)value, NUMERIC_REMAINDER2_BITS, qrcode, &bit_len);
    } else if (data_len - idx == 1) {
        value = data[idx] - '0';
        lierre_append_bits((uint32_t)value, NUMERIC_REMAINDER1_BITS, qrcode, &bit_len);
    }

    data_capacity_bits = lierre_get_num_data_codewords(version, ecl) * QR_PAD_BYTE_BITS;
    terminator_bits = data_capacity_bits - bit_len;

    if (terminator_bits > QR_TERMINATOR_MAX_BITS) {
        terminator_bits = QR_TERMINATOR_MAX_BITS;
    }

    lierre_append_bits(0, terminator_bits, qrcode, &bit_len);
    lierre_append_bits(0, (QR_PAD_BYTE_BITS - bit_len % QR_PAD_BYTE_BITS) % QR_PAD_BYTE_BITS, qrcode, &bit_len);

    for (pad_byte = PAD_BYTE_FIRST; bit_len < data_capacity_bits; pad_byte ^= PAD_BYTE_FIRST ^ PAD_BYTE_SECOND) {
        lierre_append_bits(pad_byte, QR_PAD_BYTE_BITS, qrcode, &bit_len);
    }

    lierre_add_ecc_and_interleave(qrcode, version, ecl, temp_buffer);
    lierre_initialize_function_modules(version, qrcode);
    lierre_draw_codewords(temp_buffer, lierre_get_num_raw_data_modules(version) >> 3, qrcode);
    lierre_draw_light_function_modules(qrcode, version);
    lierre_initialize_function_modules(version, temp_buffer);

    if (mask < 0) {
        min_penalty = INT32_MAX;
        best_mask = 0;

        for (i = 0; i < QR_MASK_COUNT; i++) {
            lierre_apply_mask(temp_buffer, qrcode, (int8_t)i);
            lierre_draw_format_bits(ecl, (int8_t)i, qrcode);
            penalty = lierre_get_penalty_score(qrcode);

            if (penalty < min_penalty) {
                best_mask = (int8_t)i;
                min_penalty = penalty;
            }

            lierre_apply_mask(temp_buffer, qrcode, (int8_t)i);
        }
        mask = best_mask;
    }

    lierre_apply_mask(temp_buffer, qrcode, mask);
    lierre_draw_format_bits(ecl, mask, qrcode);

    return true;
}

static inline int8_t lierre_alphanumeric_char_value(uint8_t c)
{
    if (c >= '0' && c <= '9') {
        return (int8_t)(c - '0');
    }

    if (c >= 'A' && c <= 'Z') {
        return (int8_t)(c - 'A' + ALPHA_LETTER_OFFSET);
    }

    switch (c) {
    case ' ':
        return ALPHA_SPACE_VALUE;
    case '$':
        return ALPHA_DOLLAR_VALUE;
    case '%':
        return ALPHA_PERCENT_VALUE;
    case '*':
        return ALPHA_ASTERISK_VALUE;
    case '+':
        return ALPHA_PLUS_VALUE;
    case '-':
        return ALPHA_MINUS_VALUE;
    case '.':
        return ALPHA_DOT_VALUE;
    case '/':
        return ALPHA_SLASH_VALUE;
    case ':':
        return ALPHA_COLON_VALUE;
    default:
        return -1;
    }
}

static inline bool lierre_is_alphanumeric_data(const uint8_t *data, size_t data_len)
{
    size_t i;

    for (i = 0; i < data_len; i++) {
        if (lierre_alphanumeric_char_value(data[i]) < 0) {
            return false;
        }
    }

    return true;
}

static inline int32_t lierre_alphanumeric_count_bits(uint8_t version, size_t char_count)
{
    int32_t count_bits;

    if (version < VERSION_THRESHOLD_SMALL) {
        count_bits = ALPHA_BITS_SMALL;
    } else if (version < VERSION_THRESHOLD_MEDIUM) {
        count_bits = ALPHA_BITS_MEDIUM;
    } else {
        count_bits = ALPHA_BITS_LARGE;
    }

    return QR_MODE_INDICATOR_BITS + count_bits +
           ((int32_t)char_count / ALPHANUMERIC_GROUP_SIZE) * ALPHANUMERIC_GROUP_BITS +
           ((char_count % ALPHANUMERIC_GROUP_SIZE) ? ALPHANUMERIC_REMAINDER_BITS : 0);
}

static inline bool lierre_encode_alphanumeric(const uint8_t *data, size_t data_len, uint8_t temp_buffer[],
                                              uint8_t qrcode[], uint8_t ecl, int8_t min_version, int8_t max_version,
                                              int8_t mask)
{
    int8_t version, best_mask;
    int32_t data_capacity_bits, data_used_bits, bit_len, terminator_bits, i, min_penalty, penalty;
    int32_t count_bits, value;
    uint8_t pad_byte;
    size_t idx;

    for (version = min_version;; version++) {
        data_capacity_bits = lierre_get_num_data_codewords(version, ecl) * 8;
        data_used_bits = lierre_alphanumeric_count_bits(version, data_len);

        if (data_used_bits <= data_capacity_bits) {
            break;
        }

        if (version >= max_version) {
            qrcode[0] = 0;
            return false;
        }
    }

    if (version < VERSION_THRESHOLD_SMALL) {
        count_bits = ALPHA_BITS_SMALL;
    } else if (version < VERSION_THRESHOLD_MEDIUM) {
        count_bits = ALPHA_BITS_MEDIUM;
    } else {
        count_bits = ALPHA_BITS_LARGE;
    }

    lmemset(qrcode, 0, (size_t)LIERRE_QR_BUFFER_LEN_FOR_VERSION(version) * sizeof(qrcode[0]));
    bit_len = 0;
    lierre_append_bits(QR_MODE_ALPHANUMERIC_INDICATOR, QR_MODE_INDICATOR_BITS, qrcode, &bit_len);
    lierre_append_bits((uint32_t)data_len, count_bits, qrcode, &bit_len);

    idx = 0;
    while (idx + ALPHANUMERIC_GROUP_SIZE <= data_len) {
        value = lierre_alphanumeric_char_value(data[idx]) * ALPHANUMERIC_CHARSET_SIZE +
                lierre_alphanumeric_char_value(data[idx + 1]);
        lierre_append_bits((uint32_t)value, ALPHANUMERIC_GROUP_BITS, qrcode, &bit_len);
        idx += ALPHANUMERIC_GROUP_SIZE;
    }

    if (data_len - idx == 1) {
        value = lierre_alphanumeric_char_value(data[idx]);
        lierre_append_bits((uint32_t)value, ALPHANUMERIC_REMAINDER_BITS, qrcode, &bit_len);
    }

    data_capacity_bits = lierre_get_num_data_codewords(version, ecl) * QR_PAD_BYTE_BITS;
    terminator_bits = data_capacity_bits - bit_len;

    if (terminator_bits > QR_TERMINATOR_MAX_BITS) {
        terminator_bits = QR_TERMINATOR_MAX_BITS;
    }

    lierre_append_bits(0, terminator_bits, qrcode, &bit_len);
    lierre_append_bits(0, (QR_PAD_BYTE_BITS - bit_len % QR_PAD_BYTE_BITS) % QR_PAD_BYTE_BITS, qrcode, &bit_len);

    for (pad_byte = PAD_BYTE_FIRST; bit_len < data_capacity_bits; pad_byte ^= PAD_BYTE_FIRST ^ PAD_BYTE_SECOND) {
        lierre_append_bits(pad_byte, QR_PAD_BYTE_BITS, qrcode, &bit_len);
    }

    lierre_add_ecc_and_interleave(qrcode, version, ecl, temp_buffer);
    lierre_initialize_function_modules(version, qrcode);
    lierre_draw_codewords(temp_buffer, lierre_get_num_raw_data_modules(version) >> 3, qrcode);
    lierre_draw_light_function_modules(qrcode, version);
    lierre_initialize_function_modules(version, temp_buffer);

    if (mask < 0) {
        min_penalty = INT32_MAX;
        best_mask = 0;

        for (i = 0; i < QR_MASK_COUNT; i++) {
            lierre_apply_mask(temp_buffer, qrcode, (int8_t)i);
            lierre_draw_format_bits(ecl, (int8_t)i, qrcode);
            penalty = lierre_get_penalty_score(qrcode);

            if (penalty < min_penalty) {
                best_mask = (int8_t)i;
                min_penalty = penalty;
            }

            lierre_apply_mask(temp_buffer, qrcode, (int8_t)i);
        }
        mask = best_mask;
    }

    lierre_apply_mask(temp_buffer, qrcode, mask);
    lierre_draw_format_bits(ecl, mask, qrcode);

    return true;
}

static inline bool lierre_is_kanji_byte_pair(uint8_t high, uint8_t low)
{
    uint16_t code;

    code = ((uint16_t)high << 8) | low;

    return (code >= KANJI_SJIS_RANGE1_START && code <= KANJI_SJIS_RANGE1_END) ||
           (code >= KANJI_SJIS_RANGE2_START && code <= KANJI_SJIS_RANGE2_END);
}

static inline bool lierre_is_kanji_data(const uint8_t *data, size_t data_len)
{
    size_t i;

    if (data_len == 0 || data_len % 2 != 0) {
        return false;
    }

    for (i = 0; i < data_len; i += 2) {
        if (!lierre_is_kanji_byte_pair(data[i], data[i + 1])) {
            return false;
        }
    }

    return true;
}

static inline int32_t lierre_kanji_count_bits(uint8_t version, size_t char_count)
{
    int32_t count_bits;

    if (version < VERSION_THRESHOLD_SMALL) {
        count_bits = KANJI_BITS_SMALL;
    } else if (version < VERSION_THRESHOLD_MEDIUM) {
        count_bits = KANJI_BITS_MEDIUM;
    } else {
        count_bits = KANJI_BITS_LARGE;
    }

    return QR_MODE_INDICATOR_BITS + count_bits + (int32_t)char_count * KANJI_ENCODED_BITS;
}

static inline bool lierre_encode_kanji(const uint8_t *data, size_t data_len, uint8_t temp_buffer[], uint8_t qrcode[],
                                       uint8_t ecl, int8_t min_version, int8_t max_version, int8_t mask)
{
    int32_t data_capacity_bits, data_used_bits, bit_len, terminator_bits, i, min_penalty, penalty, count_bits,
        high_byte, low_byte, intermediate, encoded_value;
    int8_t version, best_mask;
    uint16_t sjis_char;
    uint8_t pad_byte;
    size_t idx, char_count;

    char_count = data_len / 2;

    for (version = min_version;; version++) {
        data_capacity_bits = lierre_get_num_data_codewords(version, ecl) * 8;
        data_used_bits = lierre_kanji_count_bits(version, char_count);

        if (data_used_bits <= data_capacity_bits) {
            break;
        }

        if (version >= max_version) {
            qrcode[0] = 0;
            return false;
        }
    }

    if (version < VERSION_THRESHOLD_SMALL) {
        count_bits = KANJI_BITS_SMALL;
    } else if (version < VERSION_THRESHOLD_MEDIUM) {
        count_bits = KANJI_BITS_MEDIUM;
    } else {
        count_bits = KANJI_BITS_LARGE;
    }

    lmemset(qrcode, 0, (size_t)LIERRE_QR_BUFFER_LEN_FOR_VERSION(version) * sizeof(qrcode[0]));
    bit_len = 0;
    lierre_append_bits(QR_MODE_KANJI_INDICATOR, QR_MODE_INDICATOR_BITS, qrcode, &bit_len);
    lierre_append_bits((uint32_t)char_count, count_bits, qrcode, &bit_len);

    for (idx = 0; idx < data_len; idx += 2) {
        sjis_char = ((uint16_t)data[idx] << 8) | data[idx + 1];

        if (sjis_char >= KANJI_SJIS_RANGE1_START && sjis_char <= KANJI_SJIS_RANGE1_END) {
            intermediate = sjis_char - KANJI_ENCODE_BASE1;
        } else {
            intermediate = sjis_char - KANJI_ENCODE_BASE2;
        }

        high_byte = (intermediate >> 8) & 0xFF;
        low_byte = intermediate & 0xFF;
        encoded_value = high_byte * KANJI_ENCODE_MULTIPLIER + low_byte;

        lierre_append_bits((uint32_t)encoded_value, KANJI_ENCODED_BITS, qrcode, &bit_len);
    }

    data_capacity_bits = lierre_get_num_data_codewords(version, ecl) * QR_PAD_BYTE_BITS;
    terminator_bits = data_capacity_bits - bit_len;

    if (terminator_bits > QR_TERMINATOR_MAX_BITS) {
        terminator_bits = QR_TERMINATOR_MAX_BITS;
    }

    lierre_append_bits(0, terminator_bits, qrcode, &bit_len);
    lierre_append_bits(0, (QR_PAD_BYTE_BITS - bit_len % QR_PAD_BYTE_BITS) % QR_PAD_BYTE_BITS, qrcode, &bit_len);

    for (pad_byte = PAD_BYTE_FIRST; bit_len < data_capacity_bits; pad_byte ^= PAD_BYTE_FIRST ^ PAD_BYTE_SECOND) {
        lierre_append_bits(pad_byte, QR_PAD_BYTE_BITS, qrcode, &bit_len);
    }

    lierre_add_ecc_and_interleave(qrcode, version, ecl, temp_buffer);
    lierre_initialize_function_modules(version, qrcode);
    lierre_draw_codewords(temp_buffer, lierre_get_num_raw_data_modules(version) >> 3, qrcode);
    lierre_draw_light_function_modules(qrcode, version);
    lierre_initialize_function_modules(version, temp_buffer);

    if (mask < 0) {
        min_penalty = INT32_MAX;
        best_mask = 0;

        for (i = 0; i < QR_MASK_COUNT; i++) {
            lierre_apply_mask(temp_buffer, qrcode, (int8_t)i);
            lierre_draw_format_bits(ecl, (int8_t)i, qrcode);
            penalty = lierre_get_penalty_score(qrcode);

            if (penalty < min_penalty) {
                best_mask = (int8_t)i;
                min_penalty = penalty;
            }

            lierre_apply_mask(temp_buffer, qrcode, (int8_t)i);
        }
        mask = best_mask;
    }

    lierre_apply_mask(temp_buffer, qrcode, mask);
    lierre_draw_format_bits(ecl, mask, qrcode);

    return true;
}

static inline int32_t lierre_eci_header_bits(uint32_t eci_value)
{
    if (eci_value <= ECI_SINGLE_BYTE_MAX) {
        return QR_MODE_INDICATOR_BITS + QR_PAD_BYTE_BITS;
    } else if (eci_value <= ECI_DOUBLE_BYTE_MAX) {
        return QR_MODE_INDICATOR_BITS + BYTE_BITS_LARGE;
    } else {
        return QR_MODE_INDICATOR_BITS + ECI_BITS_3BYTE;
    }
}

static inline bool lierre_encode_eci(const uint8_t *data, size_t data_len, uint8_t temp_buffer[], uint8_t qrcode[],
                                     uint8_t ecl, int8_t min_version, int8_t max_version, int8_t mask,
                                     uint32_t eci_value)
{
    uint8_t pad_byte;
    int8_t version, best_mask;
    int32_t data_capacity_bits, data_used_bits, bit_len, terminator_bits, i, min_penalty, penalty;
    int32_t eci_header_bits;

    eci_header_bits = lierre_eci_header_bits(eci_value);

    for (version = min_version;; version++) {
        data_capacity_bits = lierre_get_num_data_codewords(version, ecl) * QR_PAD_BYTE_BITS;
        data_used_bits = eci_header_bits + QR_MODE_INDICATOR_BITS +
                         ((version < VERSION_THRESHOLD_SMALL) ? BYTE_BITS_SMALL : BYTE_BITS_LARGE) +
                         (int32_t)data_len * QR_PAD_BYTE_BITS;

        if (data_used_bits <= data_capacity_bits) {
            break;
        }

        if (version >= max_version) {
            qrcode[0] = 0;
            return false;
        }
    }

    lmemset(qrcode, 0, (size_t)LIERRE_QR_BUFFER_LEN_FOR_VERSION(version) * sizeof(qrcode[0]));
    bit_len = 0;

    lierre_append_bits(QR_MODE_ECI_INDICATOR, QR_MODE_INDICATOR_BITS, qrcode, &bit_len);

    if (eci_value <= ECI_SINGLE_BYTE_MAX) {
        lierre_append_bits(eci_value, QR_PAD_BYTE_BITS, qrcode, &bit_len);
    } else if (eci_value <= ECI_DOUBLE_BYTE_MAX) {
        lierre_append_bits(ECI_PREFIX_2BYTE | ((eci_value >> 8) & ECI_MASK_2BYTE), QR_PAD_BYTE_BITS, qrcode, &bit_len);
        lierre_append_bits(eci_value & 0xFF, QR_PAD_BYTE_BITS, qrcode, &bit_len);
    } else {
        lierre_append_bits(ECI_PREFIX_3BYTE | ((eci_value >> 16) & ECI_MASK_3BYTE), QR_PAD_BYTE_BITS, qrcode, &bit_len);
        lierre_append_bits((eci_value >> 8) & 0xFF, QR_PAD_BYTE_BITS, qrcode, &bit_len);
        lierre_append_bits(eci_value & 0xFF, QR_PAD_BYTE_BITS, qrcode, &bit_len);
    }

    lierre_append_bits(QR_MODE_BYTE_INDICATOR, QR_MODE_INDICATOR_BITS, qrcode, &bit_len);
    lierre_append_bits((uint32_t)data_len, (version < VERSION_THRESHOLD_SMALL) ? BYTE_BITS_SMALL : BYTE_BITS_LARGE,
                       qrcode, &bit_len);

    for (i = 0; i < (int32_t)data_len; i++) {
        lierre_append_bits(data[i], QR_PAD_BYTE_BITS, qrcode, &bit_len);
    }

    data_capacity_bits = lierre_get_num_data_codewords(version, ecl) * QR_PAD_BYTE_BITS;
    terminator_bits = data_capacity_bits - bit_len;

    if (terminator_bits > QR_TERMINATOR_MAX_BITS) {
        terminator_bits = QR_TERMINATOR_MAX_BITS;
    }

    lierre_append_bits(0, terminator_bits, qrcode, &bit_len);
    lierre_append_bits(0, (QR_PAD_BYTE_BITS - bit_len % QR_PAD_BYTE_BITS) % QR_PAD_BYTE_BITS, qrcode, &bit_len);

    for (pad_byte = PAD_BYTE_FIRST; bit_len < data_capacity_bits; pad_byte ^= PAD_BYTE_FIRST ^ PAD_BYTE_SECOND) {
        lierre_append_bits(pad_byte, QR_PAD_BYTE_BITS, qrcode, &bit_len);
    }

    lierre_add_ecc_and_interleave(qrcode, version, ecl, temp_buffer);
    lierre_initialize_function_modules(version, qrcode);
    lierre_draw_codewords(temp_buffer, lierre_get_num_raw_data_modules(version) >> 3, qrcode);
    lierre_draw_light_function_modules(qrcode, version);
    lierre_initialize_function_modules(version, temp_buffer);

    if (mask < 0) {
        min_penalty = INT32_MAX;
        best_mask = 0;

        for (i = 0; i < QR_MASK_COUNT; i++) {
            lierre_apply_mask(temp_buffer, qrcode, (int8_t)i);
            lierre_draw_format_bits(ecl, (int8_t)i, qrcode);
            penalty = lierre_get_penalty_score(qrcode);

            if (penalty < min_penalty) {
                best_mask = (int8_t)i;
                min_penalty = penalty;
            }

            lierre_apply_mask(temp_buffer, qrcode, (int8_t)i);
        }
        mask = best_mask;
    }

    lierre_apply_mask(temp_buffer, qrcode, mask);
    lierre_draw_format_bits(ecl, mask, qrcode);

    return true;
}

static inline bool lierre_encode_binary(const uint8_t *data, size_t data_len, uint8_t temp_buffer[], uint8_t qrcode[],
                                        uint8_t ecl, int8_t min_version, int8_t max_version, int8_t mask)
{
    uint8_t pad_byte;
    int8_t version, best_mask;
    int32_t data_capacity_bits, data_used_bits, bit_len, terminator_bits, i, min_penalty, penalty;

    for (version = min_version;; version++) {
        data_capacity_bits = lierre_get_num_data_codewords(version, ecl) * QR_PAD_BYTE_BITS;
        data_used_bits = QR_MODE_INDICATOR_BITS +
                         ((version < VERSION_THRESHOLD_SMALL) ? BYTE_BITS_SMALL : BYTE_BITS_LARGE) +
                         (int32_t)data_len * QR_PAD_BYTE_BITS;

        if (data_used_bits <= data_capacity_bits) {
            break;
        }

        if (version >= max_version) {
            qrcode[0] = 0;
            return false;
        }
    }

    lmemset(qrcode, 0, (size_t)LIERRE_QR_BUFFER_LEN_FOR_VERSION(version) * sizeof(qrcode[0]));
    bit_len = 0;
    lierre_append_bits(QR_MODE_BYTE_INDICATOR, QR_MODE_INDICATOR_BITS, qrcode, &bit_len);
    lierre_append_bits((uint32_t)data_len, (version < VERSION_THRESHOLD_SMALL) ? BYTE_BITS_SMALL : BYTE_BITS_LARGE,
                       qrcode, &bit_len);

    for (i = 0; i < (int32_t)data_len; i++) {
        lierre_append_bits(data[i], QR_PAD_BYTE_BITS, qrcode, &bit_len);
    }

    data_capacity_bits = lierre_get_num_data_codewords(version, ecl) * QR_PAD_BYTE_BITS;
    terminator_bits = data_capacity_bits - bit_len;

    if (terminator_bits > QR_TERMINATOR_MAX_BITS) {
        terminator_bits = QR_TERMINATOR_MAX_BITS;
    }

    lierre_append_bits(0, terminator_bits, qrcode, &bit_len);
    lierre_append_bits(0, (QR_PAD_BYTE_BITS - bit_len % QR_PAD_BYTE_BITS) % QR_PAD_BYTE_BITS, qrcode, &bit_len);

    for (pad_byte = PAD_BYTE_FIRST; bit_len < data_capacity_bits; pad_byte ^= PAD_BYTE_FIRST ^ PAD_BYTE_SECOND) {
        lierre_append_bits(pad_byte, QR_PAD_BYTE_BITS, qrcode, &bit_len);
    }

    lierre_add_ecc_and_interleave(qrcode, version, ecl, temp_buffer);
    lierre_initialize_function_modules(version, qrcode);
    lierre_draw_codewords(temp_buffer, lierre_get_num_raw_data_modules(version) >> 3, qrcode);
    lierre_draw_light_function_modules(qrcode, version);
    lierre_initialize_function_modules(version, temp_buffer);

    if (mask < 0) {
        min_penalty = INT32_MAX;
        best_mask = 0;

        for (i = 0; i < QR_MASK_COUNT; i++) {
            lierre_apply_mask(temp_buffer, qrcode, (int8_t)i);
            lierre_draw_format_bits(ecl, (int8_t)i, qrcode);
            penalty = lierre_get_penalty_score(qrcode);

            if (penalty < min_penalty) {
                best_mask = (int8_t)i;
                min_penalty = penalty;
            }

            lierre_apply_mask(temp_buffer, qrcode, (int8_t)i);
        }
        mask = best_mask;
    }

    lierre_apply_mask(temp_buffer, qrcode, mask);
    lierre_draw_format_bits(ecl, mask, qrcode);

    return true;
}

static inline int32_t lierre_get_qr_size(const uint8_t qrcode[])
{
    return qrcode[0];
}

static inline bool lierre_get_qr_module(const uint8_t qrcode[], int32_t x, int32_t y)
{
    return lierre_get_module(qrcode, x, y);
}

extern lierre_error_t lierre_writer_param_init(lierre_writer_param_t *param, uint8_t *data, size_t data_size,
                                               size_t scale, size_t margin, lierre_writer_ecc_t ecc_level,
                                               lierre_writer_mask_t mask_pattern, lierre_writer_mode_t mode)
{
    if (!param || !data || data_size == 0 || scale == 0) {
        return LIERRE_ERROR_INVALID_PARAMS;
    }

    param->data = data;
    param->data_size = data_size;
    param->scale = scale;
    param->margin = margin;
    param->ecc_level = ecc_level;
    param->mask_pattern = mask_pattern;
    param->mode = mode;

    return LIERRE_ERROR_SUCCESS;
}

extern lierre_qr_version_t lierre_writer_qr_version(const lierre_writer_param_t *param)
{
    lierre_qr_version_t ver;
    uint8_t ecl, mode_idx;
    size_t char_count;

    if (!param || !param->data || param->data_size == 0) {
        return LIERRE_WRITER_QR_VERSION_ERR;
    }

    ecl = (uint8_t)param->ecc_level;
    if (ecl > 3) {
        ecl = 0;
    }

    switch (param->mode) {
    case MODE_NUMERIC:
        mode_idx = 0;
        char_count = param->data_size;
        break;
    case MODE_ALPHANUMERIC:
        mode_idx = 1;
        char_count = param->data_size;
        break;
    case MODE_KANJI:
        mode_idx = 3;
        char_count = param->data_size / 2;
        break;
    case MODE_BYTE:
    default:
        mode_idx = 2;
        char_count = param->data_size;
        break;
    }

    for (ver = 1; ver <= 40; ver++) {
        if ((size_t)LIERRE_CHAR_CAPACITY[ecl][ver][mode_idx] >= char_count) {
            return ver;
        }
    }

    return LIERRE_WRITER_QR_VERSION_ERR;
}

extern bool lierre_writer_get_res(const lierre_writer_param_t *param, lierre_reso_t *res)
{
    lierre_qr_version_t ver;
    size_t qr_size, total_size;

    if (!param || !res) {
        return false;
    }

    ver = lierre_writer_qr_version(param);
    if (ver == LIERRE_WRITER_QR_VERSION_ERR) {
        return false;
    }

    qr_size = (size_t)LIERRE_QR_VERSION_SIZE_FORMULA(ver);
    total_size = (qr_size + param->margin * 2) * param->scale;

    res->width = total_size;
    res->height = total_size;

    return true;
}

extern size_t lierre_writer_get_res_width(const lierre_writer_param_t *param)
{
    lierre_reso_t res;

    if (!lierre_writer_get_res(param, &res)) {
        return 0;
    }

    return res.width;
}

extern size_t lierre_writer_get_res_height(const lierre_writer_param_t *param)
{
    lierre_reso_t res;

    if (!lierre_writer_get_res(param, &res)) {
        return 0;
    }

    return res.height;
}

extern lierre_writer_t *lierre_writer_create(const lierre_writer_param_t *param, const lierre_rgba_t *fill_color,
                                             const lierre_rgba_t *bg_color)
{
    lierre_writer_t *writer;
    lierre_reso_t res;
    size_t data_size;

    if (!param || !fill_color || !bg_color) {
        return NULL;
    }

    if (!lierre_writer_get_res(param, &res)) {
        return NULL;
    }

    writer = lmalloc(sizeof(lierre_writer_t));
    if (!writer) {
        return NULL;
    }

    writer->param = lmalloc(sizeof(lierre_writer_param_t));
    if (!writer->param) {
        lfree(writer);
        return NULL;
    }
    lmemcpy(writer->param, param, sizeof(lierre_writer_param_t));

    data_size = res.width * res.height * 4;
    writer->data = lmalloc(sizeof(lierre_rgb_data_t));
    if (!writer->data) {
        lfree(writer->param);
        lfree(writer);
        return NULL;
    }

    writer->data->data = lmalloc(data_size);
    if (!writer->data->data) {
        lfree(writer->data);
        lfree(writer->param);
        lfree(writer);
        return NULL;
    }

    writer->data->data_size = data_size;
    writer->data->width = res.width;
    writer->data->height = res.height;

    writer->stroke_color_rgba[0] = fill_color->r;
    writer->stroke_color_rgba[1] = fill_color->g;
    writer->stroke_color_rgba[2] = fill_color->b;
    writer->stroke_color_rgba[3] = fill_color->a;

    writer->fill_color_rgba[0] = bg_color->r;
    writer->fill_color_rgba[1] = bg_color->g;
    writer->fill_color_rgba[2] = bg_color->b;
    writer->fill_color_rgba[3] = bg_color->a;

    return writer;
}

extern void lierre_writer_destroy(lierre_writer_t *writer)
{
    if (!writer) {
        return;
    }

    if (writer->data) {
        if (writer->data->data) {
            lfree(writer->data->data);
        }

        lfree(writer->data);
    }

    if (writer->param) {
        lfree(writer->param);
    }

    lfree(writer);
}

extern lierre_error_t lierre_writer_write(lierre_writer_t *writer)
{
    lierre_qr_version_t ver;
    uint8_t *temp_buffer, *qr_buffer, *pixel;
    int32_t qr_size, x, y, px, py;
    size_t scale, margin, sx, sy, img_x, img_y, offset;
    bool is_dark;

    if (!writer || !writer->param || !writer->data || !writer->data->data) {
        return LIERRE_ERROR_INVALID_PARAMS;
    }

    ver = lierre_writer_qr_version(writer->param);
    if (ver == LIERRE_WRITER_QR_VERSION_ERR) {
        return LIERRE_ERROR_SIZE_EXCEEDED;
    }

    temp_buffer = lcalloc(1, LIERRE_QR_BUFFER_LEN_MAX);
    qr_buffer = lcalloc(1, LIERRE_QR_BUFFER_LEN_MAX);
    if (!temp_buffer || !qr_buffer) {
        lfree(temp_buffer);
        lfree(qr_buffer);

        return LIERRE_ERROR_DATA_OVERFLOW;
    }

    bool encode_success = false;

    switch (writer->param->mode) {
    case MODE_NUMERIC:
        encode_success =
            lierre_encode_numeric(writer->param->data, writer->param->data_size, temp_buffer, qr_buffer,
                                  (uint8_t)writer->param->ecc_level, 1, 40, (int8_t)writer->param->mask_pattern);
        break;
    case MODE_ALPHANUMERIC:
        encode_success =
            lierre_encode_alphanumeric(writer->param->data, writer->param->data_size, temp_buffer, qr_buffer,
                                       (uint8_t)writer->param->ecc_level, 1, 40, (int8_t)writer->param->mask_pattern);
        break;
    case MODE_KANJI:
        encode_success =
            lierre_encode_kanji(writer->param->data, writer->param->data_size, temp_buffer, qr_buffer,
                                (uint8_t)writer->param->ecc_level, 1, 40, (int8_t)writer->param->mask_pattern);
        break;
    case MODE_ECI:
        encode_success =
            lierre_encode_eci(writer->param->data, writer->param->data_size, temp_buffer, qr_buffer,
                              (uint8_t)writer->param->ecc_level, 1, 40, (int8_t)writer->param->mask_pattern, 26);
        break;
    case MODE_BYTE:
    default:
        encode_success =
            lierre_encode_binary(writer->param->data, writer->param->data_size, temp_buffer, qr_buffer,
                                 (uint8_t)writer->param->ecc_level, 1, 40, (int8_t)writer->param->mask_pattern);
        break;
    }

    if (!encode_success) {
        lfree(temp_buffer);
        lfree(qr_buffer);

        return LIERRE_ERROR_SIZE_EXCEEDED;
    }

    qr_size = lierre_get_qr_size(qr_buffer);
    scale = writer->param->scale;
    margin = writer->param->margin;

    for (x = 0; x < (int32_t)writer->data->width; x++) {
        offset = (size_t)x * 4;
        pixel = &writer->data->data[offset];
        pixel[0] = writer->fill_color_rgba[0];
        pixel[1] = writer->fill_color_rgba[1];
        pixel[2] = writer->fill_color_rgba[2];
        pixel[3] = writer->fill_color_rgba[3];
    }
    for (y = 1; y < (int32_t)writer->data->height; y++) {
        lmemcpy(&writer->data->data[(size_t)y * writer->data->width * 4], writer->data->data, writer->data->width * 4);
    }

    for (py = 0; py < qr_size; py++) {
        for (px = 0; px < qr_size; px++) {
            is_dark = lierre_get_qr_module(qr_buffer, px, py);
            for (sy = 0; sy < scale; sy++) {
                for (sx = 0; sx < scale; sx++) {
                    img_x = (margin + (size_t)px) * scale + sx;
                    img_y = (margin + (size_t)py) * scale + sy;

                    if (img_x < writer->data->width && img_y < writer->data->height) {
                        offset = (img_y * writer->data->width + img_x) * 4;
                        pixel = &writer->data->data[offset];

                        if (is_dark) {
                            pixel[0] = writer->stroke_color_rgba[0];
                            pixel[1] = writer->stroke_color_rgba[1];
                            pixel[2] = writer->stroke_color_rgba[2];
                            pixel[3] = writer->stroke_color_rgba[3];
                        }
                    }
                }
            }
        }
    }

    lfree(temp_buffer);
    lfree(qr_buffer);

    return LIERRE_ERROR_SUCCESS;
}

extern const uint8_t *lierre_writer_get_rgba_data(const lierre_writer_t *writer)
{
    if (!writer || !writer->data) {
        return NULL;
    }

    return writer->data->data;
}

extern size_t lierre_writer_get_rgba_data_size(const lierre_writer_t *writer)
{
    if (!writer || !writer->data) {
        return 0;
    }

    return writer->data->data_size;
}

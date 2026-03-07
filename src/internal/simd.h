/*
 * liblierre - simd.h
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef LIERRE_INTERNAL_SIMD_H
#define LIERRE_INTERNAL_SIMD_H

#include <stddef.h>
#include <stdint.h>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#define LIERRE_SIMD_NEON 1
#include <arm_neon.h>
#endif

#if defined(__AVX2__)
#define LIERRE_SIMD_AVX2 1
#include <immintrin.h>
#endif

#if defined(__wasm_simd128__)
#define LIERRE_SIMD_WASM 1
#include <wasm_simd128.h>
#endif

#endif /* LIERRE_INTERNAL_SIMD_H */

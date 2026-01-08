/*
 * liblierre - memory.h
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef LIERRE_INTERNAL_MEMORY_H
#define LIERRE_INTERNAL_MEMORY_H

#include <stdlib.h>
#include <string.h>

#define lmalloc(size)             malloc(size)
#define lcalloc(count, size)      calloc(count, size)
#define lfree(ptr)                free(ptr)
#define lmemmove(dest, src, size) memmove(dest, src, size)
#define lmemcpy(dest, src, size)  memcpy(dest, src, size)
#define lmemset(ptr, value, size) memset(ptr, value, size)

#endif /* LIERRE_INTERNAL_MEMORY_H */

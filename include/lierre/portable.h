/*
 * liblierre - portable.h
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef LIERRE_PORTABLE_H
#define LIERRE_PORTABLE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32

#include <windows.h>

typedef HANDLE lierre_thread_t;

typedef struct {
    void *(*start_routine)(void *);
    void *arg;
} lierre_pthread_wrapper_ctx_t;

#else

#include <pthread.h>

typedef pthread_t lierre_thread_t;

#endif

uint32_t lierre_get_cpu_count(void);

int lierre_thread_create(lierre_thread_t *thread, void *(*start_routine)(void *), void *arg);
int lierre_thread_join(lierre_thread_t thread, void **retval);

#ifdef __cplusplus
}
#endif

#endif /* LIERRE_PORTABLE_H */

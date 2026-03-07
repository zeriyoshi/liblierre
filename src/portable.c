/*
 * liblierre - portable.c
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

/* LCOV_EXCL_START */

#include <lierre/portable.h>

#ifdef _WIN32

#include <errno.h>
#include <stdlib.h>

static inline DWORD WINAPI win32_thread_wrapper(LPVOID arg)
{
    lierre_pthread_wrapper_ctx_t *ctx = (lierre_pthread_wrapper_ctx_t *)arg;
    void *(*start_routine)(void *);
    void *routine_arg;

    if (!ctx) {
        return 1;
    }

    start_routine = ctx->start_routine;
    routine_arg = ctx->arg;
    free(ctx);

    if (start_routine) {
        start_routine(routine_arg);
    }

    return 0;
}

extern int lierre_thread_create(lierre_thread_t *thread, void *(*start_routine)(void *), void *arg)
{
    lierre_pthread_wrapper_ctx_t *ctx;
    HANDLE handle;

    if (!thread || !start_routine) {
        return EINVAL;
    }

    ctx = (lierre_pthread_wrapper_ctx_t *)malloc(sizeof(lierre_pthread_wrapper_ctx_t));
    if (!ctx) {
        return ENOMEM;
    }

    ctx->start_routine = start_routine;
    ctx->arg = arg;

    handle = CreateThread(NULL, 0, win32_thread_wrapper, ctx, 0, NULL);
    if (handle == NULL) {
        free(ctx);
        return EAGAIN;
    }

    *thread = handle;
    return 0;
}

extern int lierre_thread_join(lierre_thread_t thread, void **retval)
{
    DWORD result;

    (void)retval;

    if (thread == NULL || thread == INVALID_HANDLE_VALUE) {
        return EINVAL;
    }

    result = WaitForSingleObject(thread, INFINITE);
    if (result != WAIT_OBJECT_0) {
        return EINVAL;
    }

    CloseHandle(thread);
    return 0;
}

extern uint32_t lierre_get_cpu_count(void)
{
    SYSTEM_INFO sysinfo;

    GetSystemInfo(&sysinfo);
    if (sysinfo.dwNumberOfProcessors == 0) {
        return 1;
    }
    return (uint32_t)sysinfo.dwNumberOfProcessors;
}

#else

#include <unistd.h>

extern int lierre_thread_create(lierre_thread_t *thread, void *(*start_routine)(void *), void *arg)
{
    return pthread_create(thread, NULL, start_routine, arg);
}

extern int lierre_thread_join(lierre_thread_t thread, void **retval)
{
    return pthread_join(thread, retval);
}

extern uint32_t lierre_get_cpu_count(void)
{
    long nprocs;

    nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    if (nprocs <= 0) {
        return 1;
    }
    if (nprocs > INT32_MAX) {
        return INT32_MAX;
    }
    return (uint32_t)nprocs;
}

#endif

/* LCOV_EXCL_STOP */

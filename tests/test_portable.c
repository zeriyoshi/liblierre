/*
 * liblierre - test_portable.c
 *
 * This file is part of liblierre.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <string.h>

#include <lierre/portable.h>

#include "unity.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void *thread_func(void *arg)
{
    int *value = (int *)arg;
    if (value) {
        *value = 42;
    }
    return NULL;
}

void test_thread_create_basic(void)
{
    lierre_thread_t thread;
    int value = 0, result;

    result = lierre_thread_create(&thread, thread_func, &value);
    TEST_ASSERT_EQUAL(0, result);

    lierre_thread_join(thread, NULL);
    TEST_ASSERT_EQUAL(42, value);
}

void test_thread_create_null_arg(void)
{
    lierre_thread_t thread;
    int result;

    result = lierre_thread_create(&thread, thread_func, NULL);
    TEST_ASSERT_EQUAL(0, result);

    lierre_thread_join(thread, NULL);
}

void test_thread_create_multiple(void)
{
    lierre_thread_t threads[5];
    int values[5] = {0}, i, result;

    for (i = 0; i < 5; i++) {
        result = lierre_thread_create(&threads[i], thread_func, &values[i]);
        TEST_ASSERT_EQUAL(0, result);
    }

    for (i = 0; i < 5; i++) {
        lierre_thread_join(threads[i], NULL);
        TEST_ASSERT_EQUAL(42, values[i]);
    }
}

void test_thread_join_basic(void)
{
    lierre_thread_t thread;
    int value = 0, result;

    lierre_thread_create(&thread, thread_func, &value);
    result = lierre_thread_join(thread, NULL);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(42, value);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_thread_create_basic);
    RUN_TEST(test_thread_create_null_arg);
    RUN_TEST(test_thread_create_multiple);

    RUN_TEST(test_thread_join_basic);

    return UNITY_END();
}

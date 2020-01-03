// The MIT License (MIT)
// Copyright (c) 2020 Maksim Andrianov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
#include "test-common.h"

#include "ccache/lru.h"

#include <stdarg.h>

#include <CUnit/Basic.h>
#include <cdcontainers/cdc.h>

static struct cdc_pair a = {CDC_FROM_INT(0), CDC_FROM_INT(0)};
static struct cdc_pair b = {CDC_FROM_INT(1), CDC_FROM_INT(1)};
static struct cdc_pair c = {CDC_FROM_INT(2), CDC_FROM_INT(2)};
static struct cdc_pair d = {CDC_FROM_INT(3), CDC_FROM_INT(3)};
static struct cdc_pair e = {CDC_FROM_INT(4), CDC_FROM_INT(4)};
static struct cdc_pair f = {CDC_FROM_INT(5), CDC_FROM_INT(5)};
static struct cdc_pair g = {CDC_FROM_INT(6), CDC_FROM_INT(6)};
static struct cdc_pair h = {CDC_FROM_INT(7), CDC_FROM_INT(7)};

static int eq(const void *l, const void *r)
{
  return CDC_TO_INT(l) == CDC_TO_INT(r);
}

static size_t hash(const void *val) { return cdc_hash_int(CDC_TO_INT(val)); }

static bool lru_cache_kv_int_eq(struct cc_lru_cache *c, size_t count, ...)
{
  if (count != cc_lru_cache_size(c)) {
    return false;
  }

  va_list args;
  va_start(args, count);
  for (size_t i = 0; i < count; ++i) {
    struct cdc_pair *val = va_arg(args, struct cdc_pair *);
    struct cc_lru_list_node *node = NULL;
    if (cdc_hash_table_get(c->table, val->first, (void **)&node) !=
        CDC_STATUS_OK) {
      va_end(args);
      return false;
    }

    if (node->kv.first != val->first || node->kv.second != val->second) {
      va_end(args);
      return false;
    }
  }

  va_end(args);
  return true;
}

void test_lru_cache_ctor()
{
  struct cdc_data_info info = CDC_INIT_STRUCT;
  info.eq = eq;
  info.hash = hash;

  struct cc_lru_cache *cache = NULL;

  CU_ASSERT_EQUAL(cc_lru_cache_ctor(&cache, 10 /* max_size */, &info),
                  CDC_STATUS_OK);
  CU_ASSERT(cc_lru_cache_empty(cache));
  cc_lru_cache_dtor(cache);
}

void test_lru_cache_get()
{
  struct cdc_data_info info = CDC_INIT_STRUCT;
  info.eq = eq;
  info.hash = hash;

  struct cc_lru_cache *cache = NULL;

  CU_ASSERT_EQUAL(cc_lru_cache_ctor(&cache, 2 /* max_size */, &info),
                  CDC_STATUS_OK);

  CU_ASSERT_EQUAL(
      cc_lru_cache_insert(cache, a.first, a.second, NULL /*inserted */),
      CDC_STATUS_OK);
  CU_ASSERT_EQUAL(
      cc_lru_cache_insert(cache, b.first, b.second, NULL /*inserted */),
      CDC_STATUS_OK);

  void *value = NULL;
  CU_ASSERT_EQUAL(cc_lru_cache_get(cache, b.first, &value), CDC_STATUS_OK);
  CU_ASSERT_EQUAL(value, b.second);

  CU_ASSERT_EQUAL(cc_lru_cache_get(cache, a.first, &value), CDC_STATUS_OK);
  CU_ASSERT_EQUAL(value, a.second);

  CU_ASSERT_EQUAL(cc_lru_cache_get(cache, d.first, &value),
                  CDC_STATUS_NOT_FOUND);

  CU_ASSERT_EQUAL(
      cc_lru_cache_insert(cache, c.first, c.second, NULL /*inserted */),
      CDC_STATUS_OK);
  CU_ASSERT(lru_cache_kv_int_eq(cache, 2, &a, &c));
  cc_lru_cache_dtor(cache);
}

void test_lru_cache_contains()
{
  struct cdc_data_info info = CDC_INIT_STRUCT;
  info.eq = eq;
  info.hash = hash;

  struct cc_lru_cache *cache = NULL;

  CU_ASSERT_EQUAL(cc_lru_cache_ctor(&cache, 2 /* max_size */, &info),
                  CDC_STATUS_OK);
  CU_ASSERT_EQUAL(
      cc_lru_cache_insert(cache, a.first, a.second, NULL /*inserted */),
      CDC_STATUS_OK);
  CU_ASSERT_EQUAL(
      cc_lru_cache_insert(cache, b.first, b.second, NULL /*inserted */),
      CDC_STATUS_OK);

  CU_ASSERT(cc_lru_cache_contains(cache, a.first));
  CU_ASSERT(cc_lru_cache_contains(cache, b.first));
  CU_ASSERT(!cc_lru_cache_contains(cache, c.first));
  cc_lru_cache_dtor(cache);
}

void test_lru_cache_capacity()
{
  struct cdc_data_info info = CDC_INIT_STRUCT;
  info.eq = eq;
  info.hash = hash;

  struct cc_lru_cache *cache = NULL;

  CU_ASSERT_EQUAL(cc_lru_cache_ctor(&cache, 2 /* max_size */, &info),
                  CDC_STATUS_OK);

  CU_ASSERT_EQUAL(cc_lru_cache_max_size(cache), 2);
  CU_ASSERT_EQUAL(cc_lru_cache_size(cache), 0);
  CU_ASSERT(cc_lru_cache_empty(cache));

  CU_ASSERT_EQUAL(
      cc_lru_cache_insert(cache, a.first, a.second, NULL /*inserted */),
      CDC_STATUS_OK);

  CU_ASSERT_EQUAL(cc_lru_cache_max_size(cache), 2);
  CU_ASSERT_EQUAL(cc_lru_cache_size(cache), 1);
  CU_ASSERT(!cc_lru_cache_empty(cache));
  cc_lru_cache_dtor(cache);
}

void test_lru_cache_insert()
{
  struct cdc_data_info info = CDC_INIT_STRUCT;
  info.eq = eq;
  info.hash = hash;

  struct cc_lru_cache *cache = NULL;

  CU_ASSERT_EQUAL(cc_lru_cache_ctor(&cache, 2 /* max_size */, &info),
                  CDC_STATUS_OK);

  bool inserted = false;
  CU_ASSERT_EQUAL(cc_lru_cache_insert(cache, a.first, a.second, &inserted),
                  CDC_STATUS_OK);
  CU_ASSERT(inserted);

  inserted = false;
  CU_ASSERT_EQUAL(cc_lru_cache_insert(cache, b.first, b.second, &inserted),
                  CDC_STATUS_OK);
  CU_ASSERT(inserted);

  inserted = true;
  CU_ASSERT_EQUAL(cc_lru_cache_insert(cache, b.first, b.second, &inserted),
                  CDC_STATUS_OK);
  CU_ASSERT(!inserted);

  CU_ASSERT(lru_cache_kv_int_eq(cache, 2, &a, &b));

  inserted = false;
  CU_ASSERT_EQUAL(cc_lru_cache_insert(cache, c.first, c.second, &inserted),
                  CDC_STATUS_OK);
  CU_ASSERT(inserted);
  CU_ASSERT(lru_cache_kv_int_eq(cache, 2, &b, &c));
  cc_lru_cache_dtor(cache);
}

void test_lru_cache_erase()
{
  struct cdc_data_info info = CDC_INIT_STRUCT;
  info.eq = eq;
  info.hash = hash;

  struct cc_lru_cache *cache = NULL;

  CU_ASSERT_EQUAL(cc_lru_cache_ctor(&cache, 2 /* max_size */, &info),
                  CDC_STATUS_OK);
  CU_ASSERT_EQUAL(
      cc_lru_cache_insert(cache, a.first, a.second, NULL /*inserted */),
      CDC_STATUS_OK);
  CU_ASSERT_EQUAL(
      cc_lru_cache_insert(cache, b.first, b.second, NULL /*inserted */),
      CDC_STATUS_OK);

  CU_ASSERT(lru_cache_kv_int_eq(cache, 2, &a, &b));

  cc_lru_cache_erase(cache, c.first);
  CU_ASSERT(lru_cache_kv_int_eq(cache, 2, &a, &b));

  cc_lru_cache_erase(cache, b.first);
  CU_ASSERT(lru_cache_kv_int_eq(cache, 1, &a));

  cc_lru_cache_erase(cache, a.first);
  CU_ASSERT_EQUAL(cc_lru_cache_size(cache), 0);
  CU_ASSERT(cc_lru_cache_empty(cache));
  cc_lru_cache_dtor(cache);
}

void test_lru_cache_clear()
{
  struct cdc_data_info info = CDC_INIT_STRUCT;
  info.eq = eq;
  info.hash = hash;

  struct cc_lru_cache *cache = NULL;

  CU_ASSERT_EQUAL(cc_lru_cache_ctor(&cache, 2 /* max_size */, &info),
                  CDC_STATUS_OK);
  CU_ASSERT_EQUAL(
      cc_lru_cache_insert(cache, a.first, a.second, NULL /*inserted */),
      CDC_STATUS_OK);
  CU_ASSERT_EQUAL(
      cc_lru_cache_insert(cache, b.first, b.second, NULL /*inserted */),
      CDC_STATUS_OK);

  cc_lru_cache_clear(cache);

  CU_ASSERT_EQUAL(cc_lru_cache_max_size(cache), 2);
  CU_ASSERT_EQUAL(cc_lru_cache_size(cache), 0);
  CU_ASSERT(cc_lru_cache_empty(cache));
  cc_lru_cache_dtor(cache);
}

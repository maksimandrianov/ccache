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
#ifndef CCACHE_INCLUDE_CCACHE_FIFO_H
#define CCACHE_INCLUDE_CCACHE_FIFO_H
#include <cdcontainers/common.h>
#include <cdcontainers/status.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

struct cc_list;
struct cdc_hash_table;
struct cdc_data_info;

struct cc_fifo_cache {
  size_t max_size;
  // Stores pairs of key and value.
  struct cc_list *list;
  // Stores pairs of key and list iterator.
  struct cdc_hash_table *table;
};

// Base
enum cdc_stat cc_fifo_cache_ctor(struct cc_fifo_cache **c, size_t max_size,
                                 struct cdc_data_info *info);
void cc_fifo_cache_dtor(struct cc_fifo_cache *c);

// Lookup
enum cdc_stat cc_fifo_cache_get(struct cc_fifo_cache *c, void *key,
                                void **value);
bool cc_fifo_cache_contains(struct cc_fifo_cache *c, void *key);

// Capacity
static inline size_t cc_fifo_cache_max_size(struct cc_fifo_cache *c)
{
  assert(c != NULL);

  return c->max_size;
}

size_t cc_fifo_cache_size(struct cc_fifo_cache *c);
bool cc_fifo_cache_empty(struct cc_fifo_cache *c);

// Modifiers
enum cdc_stat cc_fifo_cache_insert(struct cc_fifo_cache *c, void *key,
                                   void *value, bool *inserted);
enum cdc_stat cc_fifo_cache_insert_or_assign(struct cc_fifo_cache *c, void *key,
                                             void *value, bool *inserted);

void cc_fifo_cache_erase(struct cc_fifo_cache *c, void *key);
void cc_fifo_cache_take(struct cc_fifo_cache *c, void *key,
                        struct cdc_pair *kv);
void cc_fifo_cache_clear(struct cc_fifo_cache *c);

// Short names
#ifdef CDC_USE_SHORT_NAMES
typedef struct cc_fifo_cache fifo_cache_t;

// Base
#define fifo_cache_ctor(...) cc_fifo_cache_ctor(__VA_ARGS__)
#define fifo_cache_dtor(...) cc_fifo_cache_dtor(__VA_ARGS__)

// Lookup
#define fifo_cache_get(...) cc_fifo_cache_get(__VA_ARGS__)
#define fifo_cache_contains(...) cc_fifo_cache_contains(__VA_ARGS__)

// Capacity
#define fifo_cache_max_size(...) cc_fifo_cache_max_size(__VA_ARGS__)
#define fifo_cache_size(...) cc_fifo_cache_size(__VA_ARGS__)
#define fifo_cache_empty(...) cc_fifo_cache_empty(__VA_ARGS__)

// Modifiers
#define fifo_cache_insert(...) cc_fifo_cache_insert(__VA_ARGS__)
#define fifo_cache_insert_or_assign(...) \
  cc_fifo_cache_insert_or_assign(__VA_ARGS__)
#define fifo_cache_erase(...) cc_fifo_cache_erase(__VA_ARGS__)
#define fifo_cache_take(...) cc_fifo_cache_take(__VA_ARGS__)
#define fifo_cache_clear(...) cc_fifo_cache_clear(__VA_ARGS__)
#endif
#endif  // CCACHE_INCLUDE_CCACHE_FIFO_H

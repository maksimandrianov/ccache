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
#ifndef CCACHE_INCLUDE_CCACHE_LRU_H
#define CCACHE_INCLUDE_CCACHE_LRU_H
#include <cdcontainers/status.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

struct cdc_list;
struct cdc_hash_table;
struct cdc_data_info;

struct cc_lru_cache {
  size_t max_size;
  // Stores pairs of key and value.
  struct cdc_list *list;
  // Stores pairs of key and list iterator.
  struct cdc_hash_table *map;
  struct cdc_data_info *dinfo;
};

// Base
enum cdc_stat cc_lru_cache_ctor(struct cc_lru_cache **c, size_t max_size,
                                struct cdc_data_info *info);
void cc_lru_cache_dtor(struct cc_lru_cache *c);

// Lookup
enum cdc_stat cc_lru_cache_get(struct cc_lru_cache *c, void *key, void **value);
bool cc_lru_cache_contains(struct cc_lru_cache *c, void *key);

// Capacity
size_t cc_lru_cache_max_size(struct cc_lru_cache *c)
{
  assert(c != NULL);

  return c->max_size;
}

size_t cc_lru_cache_size(struct cc_lru_cache *c);
bool cc_lru_cache_empty(struct cc_lru_cache *c);

// Modifiers
enum cdc_stat cc_lru_cache_insert(struct cc_lru_cache *c, void *key, void *value);
void cc_lru_cache_erase(struct cc_lru_cache *c, void *key);
void cc_lru_cache_clear(struct cc_lru_cache *c);

#endif  // CCACHE_INCLUDE_CCACHE_LRU_H

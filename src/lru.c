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
#include "ccache/lru.h"

#include <cdcontainers/data-info.h>
#include <cdcontainers/hash-table.h>
#include <cdcontainers/list.h>

static void free_map_entry(void *data)
{
  struct cdc_pair *pair = (struct cdc_pair *)data;
  free(pair->second);
}

static void free_list_item(struct cc_lru_cache *c, struct cdc_pair *kv)
{
  if(CDC_HAS_DFREE(c->dinfo)) {
    c->dinfo->dfree(kv);
  }

  free(kv);
}

static void free_list_items(struct cc_lru_cache *c)
{
  CDC_LIST_FOR_EACH(item, c->list) {
    free_list_item(c, (struct cdc_pair *)item->data);
  }
}

enum cdc_stat cc_lru_cache_ctor(struct cc_lru_cache **c, size_t max_size,
                                struct cdc_data_info *info)
{
  assert(c != NULL);
  assert(info != NULL);
  assert(max_size > 0);

  struct cc_lru_cache *tmp =
      (struct cc_lru_cache *)malloc(sizeof(struct cc_lru_cache));
  if (!tmp) {
    return CDC_STATUS_BAD_ALLOC;
  }

  enum cdc_stat stat = CDC_STATUS_OK;
  if (info && !(tmp->dinfo = cdc_di_shared_ctorc(info))) {
    stat = CDC_STATUS_BAD_ALLOC;
    goto free_cache;
  }

  stat = cdc_list_ctor(&tmp->list, NULL);
  if (stat != CDC_STATUS_OK) {
    goto free_di;
  }

  struct cdc_data_info ht_info = CDC_INIT_STRUCT;
  ht_info.hash = info->hash;
  ht_info.eq = info->eq;
  ht_info.dfree = free_map_entry;
  stat = cdc_hash_table_ctor(&tmp->map, &ht_info);
  if (stat != CDC_STATUS_OK) {
    goto free_list;
  }

  tmp->max_size = max_size;
  *c = tmp;
  return CDC_STATUS_OK;

free_list:
  cdc_list_dtor(tmp->list);
free_di:
  cdc_di_shared_dtor(tmp->dinfo);
free_cache:
  free(tmp);
  return stat;
}

void cc_lru_cache_dtor(struct cc_lru_cache *c)
{
  assert(c != NULL);

  cdc_hash_table_dtor(c->map);
  free_list_items(c);
  cdc_list_dtor(c->list);
  cdc_di_shared_dtor(c->dinfo);
  free(c);
}

enum cdc_stat cc_lru_cache_get(struct cc_lru_cache *c, void *key, void **value)
{
  assert(c != NULL);
  assert(value != NULL);

  struct cdc_list_iter *iter = NULL;
  enum cdc_stat stat = cdc_hash_table_get(c->map, key, (void **)&iter);
  if (stat != CDC_STATUS_OK) {
    return stat;
  }

  struct cdc_pair *kv = (struct cdc_pair *)cdc_list_iter_data(iter);
  *value = kv->second;

  struct cdc_list_iter it_begin = CDC_INIT_STRUCT;
  cdc_list_begin(c->list, &it_begin);

  struct cdc_list_iter it_last = *iter;
  cdc_list_iter_next(&it_last);
  cdc_list_splice(&it_begin, iter, &it_last);
  return CDC_STATUS_OK;
}

bool cc_lru_cache_contains(struct cc_lru_cache *c, void *key)
{
  assert(c != NULL);

  return cdc_hash_table_count(c->map, key) != 0;
}

size_t cc_lru_cache_size(struct cc_lru_cache *c)
{
  assert(c != NULL);
  assert(cdc_list_size(c->list) == cdc_hash_table_size(c->map));

  return cdc_list_size(c->list);
}

bool cc_lru_cache_empty(struct cc_lru_cache *c)
{
  assert(c != NULL);
  assert(cdc_list_empty(c->list) == cdc_hash_table_empty(c->map));

  return cdc_list_empty(c->list);
}

enum cdc_stat cc_lru_cache_insert(struct cc_lru_cache *c, void *key,
                                  void *value)
{
  assert(c != NULL);

  if (cc_lru_cache_size(c) + 1 > cc_lru_cache_max_size(c)) {
    struct cdc_pair *kv = (struct cdc_pair *)cdc_list_back(c->list);
    cc_lru_cache_erase(c, kv->first);
  }

  struct cdc_pair *kv = (struct cdc_pair *)malloc(sizeof(struct cdc_pair));
  if (!kv) {
    return CDC_STATUS_BAD_ALLOC;
  }

  kv->first = key;
  kv->second = value;
  struct cdc_list_iter *iter =
      (struct cdc_list_iter *)malloc(sizeof(struct cdc_list_iter));
  if (!iter) {
    free(kv);
    return CDC_STATUS_BAD_ALLOC;
  }

  enum cdc_stat stat = cdc_list_push_front(c->list, kv);
  if (stat != CDC_STATUS_OK) {
    goto free_data;
  }

  cdc_list_begin(c->list, iter);
  stat = cdc_hash_table_insert(c->map, key, iter, NULL /* it */,
                               NULL /* inserted */);
  if (stat != CDC_STATUS_OK) {
    cdc_list_pop_front(c->list);
    goto free_data;
  }

  return CDC_STATUS_OK;

free_data:
  free(kv);
  free(iter);
  return stat;
}

void cc_lru_cache_erase(struct cc_lru_cache *c, void *key)
{
  assert(c != NULL);
  assert(cc_lru_cache_contains(c, key));

  struct cdc_list_iter *iter = NULL;
  enum cdc_stat stat = cdc_hash_table_get(c->map, key, (void **)&iter);
  if (stat != CDC_STATUS_OK) {
    return;
  }

  struct cdc_pair *kv = cdc_list_iter_data(iter);
  cdc_list_ierase(iter);
  cdc_hash_table_erase(c->map, key);
  free_list_item(c, kv);
}

void cc_lru_cache_clear(struct cc_lru_cache *c)
{
  assert(c != NULL);

  cdc_hash_table_clear(c->map);
  free_list_items(c);
  cdc_list_clear(c->list);
}

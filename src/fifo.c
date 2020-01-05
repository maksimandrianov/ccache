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
#include "ccache/fifo.h"

#include "list.h"

#include <cdcontainers/data-info.h>
#include <cdcontainers/hash-table.h>

static enum cdc_stat insert_new(struct cc_fifo_cache *c, void *key, void *value)
{
  if (cc_fifo_cache_size(c) + 1 > cc_fifo_cache_max_size(c)) {
    cc_fifo_cache_erase(c, c->list->tail->kv.first);
  }

  struct cc_list_node *node = cc_list_new_node(key, value);
  if (!node) {
    return CDC_STATUS_BAD_ALLOC;
  }

  enum cdc_stat stat = cdc_hash_table_insert(c->table, key, node, NULL /* it */,
                                             NULL /* inserted */);
  if (stat != CDC_STATUS_OK) {
    cc_list_free_node(c->list, node, true /* remove_data */);
    return stat;
  }

  cc_list_push_front_node(c->list, node);
  return CDC_STATUS_OK;
}

enum cdc_stat cc_fifo_cache_ctor(struct cc_fifo_cache **c, size_t max_size,
                                 struct cdc_data_info *info)
{
  assert(c != NULL);
  assert(info != NULL);
  assert(max_size > 0);

  struct cc_fifo_cache *tmp =
      (struct cc_fifo_cache *)malloc(sizeof(struct cc_fifo_cache));
  if (!tmp) {
    return CDC_STATUS_BAD_ALLOC;
  }

  enum cdc_stat stat = CDC_STATUS_OK;
  if (CDC_HAS_DFREE(info)) {
    struct cdc_data_info list_info = CDC_INIT_STRUCT;
    list_info.dfree = info->dfree;
    stat = cc_list_ctor(&tmp->list, &list_info);
  } else {
    stat = cc_list_ctor(&tmp->list, NULL);
  }

  if (stat != CDC_STATUS_OK) {
    goto free_cache;
  }

  struct cdc_data_info ht_info = CDC_INIT_STRUCT;
  ht_info.hash = info->hash;
  ht_info.eq = info->eq;
  stat = cdc_hash_table_ctor(&tmp->table, &ht_info);
  if (stat != CDC_STATUS_OK) {
    goto free_list;
  }

  tmp->max_size = max_size;
  *c = tmp;
  return CDC_STATUS_OK;

free_list:
  cc_list_dtor(tmp->list);
free_cache:
  free(tmp);
  return stat;
}

void cc_fifo_cache_dtor(struct cc_fifo_cache *c)
{
  assert(c != NULL);

  cdc_hash_table_dtor(c->table);
  cc_list_dtor(c->list);
  free(c);
}

enum cdc_stat cc_fifo_cache_get(struct cc_fifo_cache *c, void *key,
                                void **value)
{
  assert(c != NULL);
  assert(value != NULL);

  struct cc_list_node *node = NULL;
  enum cdc_stat stat = cdc_hash_table_get(c->table, key, (void **)&node);
  if (stat != CDC_STATUS_OK) {
    return stat;
  }

  *value = node->kv.second;
  return CDC_STATUS_OK;
}

bool cc_fifo_cache_contains(struct cc_fifo_cache *c, void *key)
{
  assert(c != NULL);

  return cdc_hash_table_count(c->table, key) != 0;
}

size_t cc_fifo_cache_size(struct cc_fifo_cache *c)
{
  assert(c != NULL);

  return cdc_hash_table_size(c->table);
}

bool cc_fifo_cache_empty(struct cc_fifo_cache *c)
{
  assert(c != NULL);

  return cdc_hash_table_empty(c->table);
}

enum cdc_stat cc_fifo_cache_insert(struct cc_fifo_cache *c, void *key,
                                   void *value, bool *inserted)
{
  assert(c != NULL);

  if (cdc_hash_table_count(c->table, key) != 0) {
    if (inserted) {
      *inserted = false;
    }

    return CDC_STATUS_OK;
  }

  enum cdc_stat stat = insert_new(c, key, value);
  if (stat == CDC_STATUS_OK && inserted) {
    *inserted = true;
  }

  return stat;
}

enum cdc_stat cc_fifo_cache_insert_or_assign(struct cc_fifo_cache *c, void *key,
                                             void *value, bool *inserted)
{
  struct cc_list_node *node = NULL;
  if (cdc_hash_table_get(c->table, key, (void **)&node) == CDC_STATUS_OK) {
    // Try to remove old value.
    if (CDC_HAS_DFREE(c->list->dinfo)) {
      struct cdc_pair kv = {NULL, node->kv.second};
      c->list->dinfo->dfree(&kv);
    }

    node->kv.second = value;
    if (inserted) {
      *inserted = false;
    }

    return CDC_STATUS_OK;
  }

  enum cdc_stat stat = insert_new(c, key, value);
  if (stat == CDC_STATUS_OK && inserted) {
    *inserted = true;
  }

  return stat;
}

void cc_fifo_cache_erase(struct cc_fifo_cache *c, void *key)
{
  assert(c != NULL);

  struct cc_list_node *node = NULL;
  enum cdc_stat stat = cdc_hash_table_get(c->table, key, (void **)&node);
  if (stat != CDC_STATUS_OK) {
    return;
  }

  cc_list_unlink_node(c->list, node);
  cdc_hash_table_erase(c->table, key);
  cc_list_free_node(c->list, node, true /* remove_data */);
}

void cc_fifo_cache_take(struct cc_fifo_cache *c, void *key, struct cdc_pair *kv)
{
  assert(c != NULL);

  struct cc_list_node *node = NULL;
  enum cdc_stat stat = cdc_hash_table_get(c->table, key, (void **)&node);
  if (stat != CDC_STATUS_OK) {
    return;
  }

  *kv = node->kv;
  cc_list_unlink_node(c->list, node);
  cdc_hash_table_erase(c->table, key);
  cc_list_free_node(c->list, node, false /* remove_data */);
}

void cc_fifo_cache_clear(struct cc_fifo_cache *c)
{
  assert(c != NULL);

  cdc_hash_table_clear(c->table);
  cc_list_clear(c->list);
}

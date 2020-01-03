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

static struct cc_lru_list_node *list_new_node(void *key, void *value)
{
  struct cc_lru_list_node *node =
      (struct cc_lru_list_node *)malloc(sizeof(struct cc_lru_list_node));
  if (node) {
    node->kv.first = key;
    node->kv.second = value;
  }

  return node;
}

static void list_free_node(struct cc_lru_list *l, struct cc_lru_list_node *node)
{
  if (CDC_HAS_DFREE(l->dinfo)) {
    l->dinfo->dfree(&node->kv);
  }

  free(node);
}

static void list_free_items(struct cc_lru_list *l)
{
  struct cc_lru_list_node *next = NULL;
  while (l->head) {
    next = l->head->next;
    list_free_node(l, l->head);
    l->head = next;
  }
}

static enum cdc_stat list_ctor(struct cc_lru_list *l,
                               struct cdc_data_info *info)
{
  l->head = NULL;
  l->tail = NULL;
  if (info) {
    l->dinfo = cdc_di_shared_ctorc(info);
    if (!l->dinfo) {
      return CDC_STATUS_BAD_ALLOC;
    }
  } else {
    l->dinfo = NULL;
  }

  return CDC_STATUS_OK;
}

static void list_dtor(struct cc_lru_list *l)
{
  list_free_items(l);
  cdc_di_shared_dtor(l->dinfo);
}

static void list_push_front_node(struct cc_lru_list *l,
                                 struct cc_lru_list_node *node)
{
  if (l->head) {
    node->next = l->head;
    l->head->prev = node;
  } else {
    node->next = NULL;
    l->tail = node;
  }

  node->prev = NULL;
  l->head = node;
}

static void list_unlink_node(struct cc_lru_list *l,
                             struct cc_lru_list_node *node)
{
  if (l->head == node) {
    struct cc_lru_list_node *new_head = l->head->next;
    if (new_head) {
      new_head->prev = NULL;
      l->head = new_head;
    } else {
      l->tail = NULL;
      l->head = NULL;
    }
  } else if (l->tail == node) {
    struct cc_lru_list_node *new_tail = l->tail->prev;
    if (new_tail) {
      new_tail->next = NULL;
      l->tail = new_tail;
    } else {
      l->tail = NULL;
      l->head = NULL;
    }
  } else {
    node->next->prev = node->prev;
    node->prev->next = node->next;
  }
}

static enum cdc_stat insert_new(struct cc_lru_cache *c, void *key, void *value)
{
  if (cc_lru_cache_size(c) + 1 > cc_lru_cache_max_size(c)) {
    cc_lru_cache_erase(c, c->list.tail->kv.first);
  }

  struct cc_lru_list_node *node = list_new_node(key, value);
  if (!node) {
    return CDC_STATUS_BAD_ALLOC;
  }

  enum cdc_stat stat = cdc_hash_table_insert(c->table, key, node, NULL /* it */,
                                             NULL /* inserted */);
  if (stat != CDC_STATUS_OK) {
    list_free_node(&c->list, node);
    return stat;
  }

  list_push_front_node(&c->list, node);
  return CDC_STATUS_OK;
}

static void update_position(struct cc_lru_cache *c,
                            struct cc_lru_list_node *node)
{
  list_unlink_node(&c->list, node);
  list_push_front_node(&c->list, node);
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
  if (CDC_HAS_DFREE(info)) {
    struct cdc_data_info list_info = CDC_INIT_STRUCT;
    list_info.dfree = info->dfree;
    stat = list_ctor(&tmp->list, &list_info);
  } else {
    stat = list_ctor(&tmp->list, NULL);
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
  list_dtor(&tmp->list);
free_cache:
  free(tmp);
  return stat;
}

void cc_lru_cache_dtor(struct cc_lru_cache *c)
{
  assert(c != NULL);

  cdc_hash_table_dtor(c->table);
  list_dtor(&c->list);
  free(c);
}

enum cdc_stat cc_lru_cache_get(struct cc_lru_cache *c, void *key, void **value)
{
  assert(c != NULL);
  assert(value != NULL);

  struct cc_lru_list_node *node = NULL;
  enum cdc_stat stat = cdc_hash_table_get(c->table, key, (void **)&node);
  if (stat != CDC_STATUS_OK) {
    return stat;
  }

  update_position(c, node);
  *value = node->kv.second;
  return CDC_STATUS_OK;
}

bool cc_lru_cache_contains(struct cc_lru_cache *c, void *key)
{
  assert(c != NULL);

  struct cc_lru_list_node *node = NULL;
  enum cdc_stat stat = cdc_hash_table_get(c->table, key, (void **)&node);
  if (stat == CDC_STATUS_NOT_FOUND) {
    return false;
  }

  update_position(c, node);
  return true;
}

size_t cc_lru_cache_size(struct cc_lru_cache *c)
{
  assert(c != NULL);

  return cdc_hash_table_size(c->table);
}

bool cc_lru_cache_empty(struct cc_lru_cache *c)
{
  assert(c != NULL);

  return cdc_hash_table_empty(c->table);
}

enum cdc_stat cc_lru_cache_insert(struct cc_lru_cache *c, void *key,
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

enum cdc_stat cc_lru_cache_insert_or_assign(struct cc_lru_cache *c, void *key,
                                            void *value, bool *inserted)
{
  struct cc_lru_list_node *node = NULL;
  if (cdc_hash_table_get(c->table, key, (void **)&node) == CDC_STATUS_OK) {
    // Try to remove old value.
    if (CDC_HAS_DFREE(c->list.dinfo)) { 
      struct cdc_pair kv = {NULL, node->kv.second};
      c->list.dinfo->dfree(&kv);
    }

    node->kv.second = value;
    update_position(c, node);
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

void cc_lru_cache_erase(struct cc_lru_cache *c, void *key)
{
  assert(c != NULL);

  struct cc_lru_list_node *node = NULL;
  enum cdc_stat stat = cdc_hash_table_get(c->table, key, (void **)&node);
  if (stat != CDC_STATUS_OK) {
    return;
  }

  list_unlink_node(&c->list, node);
  cdc_hash_table_erase(c->table, key);
  list_free_node(&c->list, node);
}

void cc_lru_cache_clear(struct cc_lru_cache *c)
{
  assert(c != NULL);

  cdc_hash_table_clear(c->table);
  list_free_items(&c->list);
  c->list.head = NULL;
  c->list.tail = NULL;
}

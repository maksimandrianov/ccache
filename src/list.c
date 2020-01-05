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
#include "list.h"

#include <cdcontainers/data-info.h>

#include <stdlib.h>

static void cc_list_free_items(struct cc_list *l)
{
  struct cc_list_node *next = NULL;
  while (l->head) {
    next = l->head->next;
    cc_list_free_node(l, l->head, true /* remove_data */);
    l->head = next;
  }
}

struct cc_list_node *cc_list_new_node(void *key, void *value)
{
  struct cc_list_node *node =
      (struct cc_list_node *)malloc(sizeof(struct cc_list_node));
  if (node) {
    node->kv.first = key;
    node->kv.second = value;
  }

  return node;
}

void cc_list_free_node(struct cc_list *l, struct cc_list_node *node,
                       bool remove_data)
{
  if (remove_data && CDC_HAS_DFREE(l->dinfo)) {
    l->dinfo->dfree(&node->kv);
  }

  free(node);
}

enum cdc_stat cc_list_ctor(struct cc_list **l, struct cdc_data_info *info)
{
  struct cc_list *tmp = calloc(sizeof(struct cc_list), 1);
  if (!tmp) {
    return CDC_STATUS_BAD_ALLOC;
  }

  if (info) {
    tmp->dinfo = cdc_di_shared_ctorc(info);
    if (!tmp->dinfo) {
      free(tmp);
      return CDC_STATUS_BAD_ALLOC;
    }
  }

  *l = tmp;
  return CDC_STATUS_OK;
}

void cc_list_dtor(struct cc_list *l)
{
  cc_list_free_items(l);
  cdc_di_shared_dtor(l->dinfo);
}

void cc_list_push_front_node(struct cc_list *l, struct cc_list_node *node)
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

void cc_list_unlink_node(struct cc_list *l, struct cc_list_node *node)
{
  if (l->head == node) {
    struct cc_list_node *new_head = l->head->next;
    if (new_head) {
      new_head->prev = NULL;
      l->head = new_head;
    } else {
      l->tail = NULL;
      l->head = NULL;
    }
  } else if (l->tail == node) {
    struct cc_list_node *new_tail = l->tail->prev;
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

void cc_list_clear(struct cc_list *l)
{
  cc_list_free_items(l);
  l->head = NULL;
  l->tail = NULL;
}

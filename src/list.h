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
#ifndef CCACHE_SRC_LRU_H
#define CCACHE_SRC_LRU_H
#include <cdcontainers/common.h>
#include <cdcontainers/status.h>

struct cc_list_node {
  struct cc_list_node *next;
  struct cc_list_node *prev;
  struct cdc_pair kv;
};

struct cc_list {
  struct cc_list_node *head;
  struct cc_list_node *tail;
  struct cdc_data_info *dinfo;
};

struct cc_list_node *cc_list_new_node(void *key, void *value);
void cc_list_free_node(struct cc_list *l, struct cc_list_node *node,
                       bool remove_data);

enum cdc_stat cc_list_ctor(struct cc_list **l, struct cdc_data_info *info);

void cc_list_dtor(struct cc_list *l);

void cc_list_push_front_node(struct cc_list *l, struct cc_list_node *node);
void cc_list_unlink_node(struct cc_list *l, struct cc_list_node *node);
void cc_list_clear(struct cc_list *l);

#endif  // CCACHE_SRC_LRU_H

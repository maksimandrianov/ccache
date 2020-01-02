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

#include <cdcontainers/global.h>

#include <CUnit/Basic.h>

int main(int argc, char** argv)
{
  CDC_UNUSED(argc);
  CDC_UNUSED(argv);

  CU_pSuite p_suite = NULL;

  if (CUE_SUCCESS != CU_initialize_registry()) {
    return CU_get_error();
  }


  p_suite = CU_add_suite("LRU CACHE", NULL, NULL);
  if (p_suite == NULL) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  if (CU_add_test(p_suite, "test_ctor", test_lru_cache_ctor) == NULL ||
      CU_add_test(p_suite, "test_get", test_lru_cache_get) == NULL ||
      CU_add_test(p_suite, "test_contains", test_lru_cache_contains) == NULL ||
      CU_add_test(p_suite, "test_capacity", test_lru_cache_capacity) == NULL ||
      CU_add_test(p_suite, "test_insert", test_lru_cache_insert) == NULL ||
      CU_add_test(p_suite, "test_erase", test_lru_cache_erase) == NULL ||
      CU_add_test(p_suite, "test_clear", test_lru_cache_clear) == NULL) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return CU_get_error();
}

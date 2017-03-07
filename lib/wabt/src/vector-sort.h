/*
 * Copyright 2016 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef WABT_VECTOR_SORT_H_
#define WABT_VECTOR_SORT_H_

#include "vector.h"

/*
 * Define algorithm to sort wabt vectors (defined by macro WABT_VECTOR).
 *
 */

#define WABT_DEFINE_VECTOR_SORT(name, type)                                \
  static void sort_##name##_vector(type##Vector* in_vec,                   \
                                   type##Vector* out_vec,                  \
                                   void (*swap_fcn)(type * v1, type * v2), \
                                   int (*lt_fcn)(type * v1, type * v2)) {  \
    /* TODO(karlschimpf) Use a faster sort. */                             \
    size_t i;                                                              \
    size_t j;                                                              \
    if (in_vec->size == 0)                                                 \
      return;                                                              \
    for (i = 0; i < in_vec->size; ++i) {                                   \
      append_##name##_value(out_vec, &in_vec->data[i]);                    \
      if (out_vec->size < 2)                                               \
        continue;                                                          \
      for (j = out_vec->size; j >= 2; --j) {                               \
        type* v1 = &out_vec->data[j - 1];                                  \
        type* v2 = &out_vec->data[j - 2];                                  \
        if (lt_fcn(v1, v2))                                                \
          swap_fcn(v1, v2);                                                \
      }                                                                    \
    }                                                                      \
  }

#endif // WABT_VECTOR_SORT_H_

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

#ifndef WABT_ARRAY_H_
#define WABT_ARRAY_H_

#include <stddef.h>

#include "common.h"

#define WABT_DEFINE_ARRAY(name, type)                                         \
  struct type##Array {                                                        \
    type* data;                                                               \
    size_t size;                                                              \
  };                                                                          \
                                                                              \
  static WABT_INLINE void destroy_##name##_array(type##Array* array)          \
      WABT_UNUSED;                                                            \
  static WABT_INLINE void new_##name##_array(type##Array* array, size_t size) \
      WABT_UNUSED;                                                            \
                                                                              \
  void destroy_##name##_array(type##Array* array) { delete[] array->data; }   \
  void new_##name##_array(type##Array* array, size_t size) {                  \
    array->size = size;                                                       \
    array->data = new type[size]();                                           \
  }

#define WABT_DESTROY_ARRAY_AND_ELEMENTS(v, name) \
  {                                              \
    size_t i;                                    \
    for (i = 0; i < (v).size; ++i)               \
      destroy_##name(&((v).data[i]));            \
    destroy_##name##_array(&(v));                \
  }

#endif /* WABT_ARRAY_H_ */

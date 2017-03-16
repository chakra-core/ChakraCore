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

#ifndef WABT_VECTOR_H_
#define WABT_VECTOR_H_

#include <stddef.h>

#include "common.h"
#include "config.h"

/*
 * WABT_DEFINE_VECTOR(widget, Widget) defines struct and functions like the
 * following:
 *
 * struct WidgetVector {
 *   Widget* data;
 *   size_t size;
 *   size_t capacity;
 * };
 *
 * void destroy_widget_vector(WidgetVector* vec);
 * Widget* append_widget(WidgetVector* vec);
 * void resize_widget_vector(WidgetVector* vec, size_t size);
 * void reserve_widgets(WidgetVector* vec, size_t desired);
 * void append_widget_value(WidgetVector* vec, const Widget* value);
 * void extend_widgets(WidgetVector* dst, const WidgetVector* src);
 */

#define WABT_DEFINE_VECTOR(name, type)                                         \
  struct type##Vector {                                                        \
    type* data;                                                                \
    size_t size;                                                               \
    size_t capacity;                                                           \
  };                                                                           \
                                                                               \
  static WABT_INLINE void destroy_##name##_vector(type##Vector* vec)           \
      WABT_UNUSED;                                                             \
  static WABT_INLINE void resize_##name##_vector(type##Vector* vec,            \
                                                 size_t desired) WABT_UNUSED;  \
  static WABT_INLINE void reserve_##name##s(type##Vector* vec, size_t desired) \
      WABT_UNUSED;                                                             \
  static WABT_INLINE type* append_##name(type##Vector* vec) WABT_UNUSED;       \
  static WABT_INLINE void append_##name##_value(                               \
      type##Vector* vec, const type* value) WABT_UNUSED;                       \
  static WABT_INLINE void extend_##name##s(                                    \
      type##Vector* dst, const type##Vector* src) WABT_UNUSED;                 \
                                                                               \
  void destroy_##name##_vector(type##Vector* vec) {                            \
    delete[] vec->data;                                                        \
    vec->data = nullptr;                                                       \
    vec->size = 0;                                                             \
    vec->capacity = 0;                                                         \
  }                                                                            \
  void resize_##name##_vector(type##Vector* vec, size_t size) {                \
    resize_vector(reinterpret_cast<char**>(&vec->data), &vec->size,            \
                  &vec->capacity, size, sizeof(type));                         \
  }                                                                            \
  void reserve_##name##s(type##Vector* vec, size_t desired) {                  \
    ensure_capacity(reinterpret_cast<char**>(&vec->data), &vec->capacity,      \
                    desired, sizeof(type));                                    \
  }                                                                            \
  type* append_##name(type##Vector* vec) {                                     \
    return static_cast<type*>(                                                 \
        append_element(reinterpret_cast<char**>(&vec->data), &vec->size,       \
                       &vec->capacity, sizeof(type)));                         \
  }                                                                            \
  void append_##name##_value(type##Vector* vec, const type* value) {           \
    type* slot = append_##name(vec);                                           \
    *slot = *value;                                                            \
  }                                                                            \
  void extend_##name##s(type##Vector* dst, const type##Vector* src) {          \
    extend_elements(                                                           \
        reinterpret_cast<char**>(&dst->data), &dst->size, &dst->capacity,      \
        reinterpret_cast<char* const*>(&src->data), src->size, sizeof(type));  \
  }

#define WABT_DESTROY_VECTOR_AND_ELEMENTS(v, name) \
  {                                               \
    size_t i;                                     \
    for (i = 0; i < (v).size; ++i)                \
      destroy_##name(&((v).data[i]));             \
    destroy_##name##_vector(&(v));                \
  }

namespace wabt {

void ensure_capacity(char** data,
                     size_t* capacity,
                     size_t desired_size,
                     size_t elt_byte_size);

void resize_vector(char** data,
                   size_t* size,
                   size_t* capacity,
                   size_t desired_size,
                   size_t elt_byte_size);

void* append_element(char** data,
                     size_t* size,
                     size_t* capacity,
                     size_t elt_byte_size);

void extend_elements(char** dst,
                     size_t* dst_size,
                     size_t* dst_capacity,
                     char* const* src,
                     size_t src_size,
                     size_t elt_byte_size);

}  // namespace wabt

#endif /* WABT_VECTOR_H_ */

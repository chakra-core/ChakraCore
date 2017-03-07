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

#include "vector.h"

#define INITIAL_VECTOR_CAPACITY 8

namespace wabt {

void ensure_capacity(char** data,
                     size_t* capacity,
                     size_t desired_size,
                     size_t elt_byte_size) {
  if (desired_size > *capacity) {
    size_t new_capacity = *capacity ? *capacity * 2 : INITIAL_VECTOR_CAPACITY;
    while (new_capacity < desired_size)
      new_capacity *= 2;
    size_t new_byte_size = new_capacity * elt_byte_size;
    size_t old_byte_capacity = *capacity * elt_byte_size;
    char* new_data = new char [new_byte_size];
    memcpy(new_data, *data, old_byte_capacity);
    memset(new_data + old_byte_capacity, 0, new_byte_size - old_byte_capacity);
    delete[] *data;
    *data = new_data;
    *capacity = new_capacity;
  }
}

void resize_vector(char** data,
                   size_t* size,
                   size_t* capacity,
                   size_t desired_size,
                   size_t elt_byte_size) {
  size_t old_size = *size;
  ensure_capacity(data, capacity, desired_size, elt_byte_size);
  if (desired_size > old_size) {
    memset(*data + old_size * elt_byte_size, 0,
           (desired_size - old_size) * elt_byte_size);
  }
  *size = desired_size;
}

void* append_element(char** data,
                     size_t* size,
                     size_t* capacity,
                     size_t elt_byte_size) {
  ensure_capacity(data, capacity, *size + 1, elt_byte_size);
  char* p = *data + (*size)++ * elt_byte_size;
  memset(p, 0, elt_byte_size);
  return p;
}

void extend_elements(char** dst,
                     size_t* dst_size,
                     size_t* dst_capacity,
                     char* const* src,
                     size_t src_size,
                     size_t elt_byte_size) {
  ensure_capacity(dst, dst_capacity, *dst_size + src_size, elt_byte_size);
  memcpy(*dst + (*dst_size * elt_byte_size), *src, src_size * elt_byte_size);
  *dst_size += src_size;
}

}  // namespace wabt

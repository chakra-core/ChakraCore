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

#ifndef WABT_TYPE_VECTOR_H_
#define WABT_TYPE_VECTOR_H_

#include "common.h"
#include "vector.h"

namespace wabt {

WABT_DEFINE_VECTOR(type, Type)

static WABT_INLINE bool type_vectors_are_equal(const TypeVector* types1,
                                               const TypeVector* types2) {
  if (types1->size != types2->size)
    return false;
  size_t i;
  for (i = 0; i < types1->size; ++i) {
    if (types1->data[i] != types2->data[i])
      return false;
  }
  return true;
}

}  // namespace wabt

#endif /* WABT_TYPE_VECTOR_H_ */

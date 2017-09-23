/*
 * Copyright 2017 WebAssembly Community Group participants
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

#include "src/hash-util.h"

#include "config.h"

namespace wabt {

// Hash combiner from:
// http://stackoverflow.com/questions/4948780/magic-number-in-boosthash-combine

hash_code HashCombine(hash_code seed, hash_code y) {
#if SIZEOF_SIZE_T == 4
  constexpr hash_code magic = 0x9e3779b9;
#elif SIZEOF_SIZE_T == 8
  constexpr hash_code magic = 0x9e3779b97f4a7c16;
#else
#error "weird sizeof size_t"
#endif
  seed ^= y + magic + (seed << 6) + (seed >> 2);
  return seed;
}

}  // namespace wabt

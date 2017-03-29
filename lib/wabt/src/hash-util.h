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

#ifndef WABT_HASH_UTIL_H_
#define WABT_HASH_UTIL_H_

#include <cstdlib>
#include <functional>

namespace wabt {

typedef std::size_t hash_code;

inline hash_code hash_combine() { return 0; }
inline hash_code hash_combine(hash_code seed) { return seed; }
hash_code hash_combine(hash_code x, hash_code y);

template <typename T, typename... U>
inline hash_code hash_combine(const T& first, const U&... rest) {
  return hash_combine(hash_combine(rest...), std::hash<T>()(first));
}

template <typename It>
inline hash_code hash_range(It first, It last) {
  hash_code result = 0;
  for (auto iter = first; iter != last; ++iter) {
    result = hash_combine(result, *iter);
  }
  return result;
}

}  // namespace wabt

#endif  // WABT_HASH_UTIL_H_

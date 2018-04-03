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

#ifndef WABT_MAKE_UNIQUE_H_
#define WABT_MAKE_UNIQUE_H_

#include <memory>

namespace wabt {

// This is named MakeUnique instead of make_unique because make_unique has the
// potential to conflict with std::make_unique if it is defined.
//
// On gcc/clang, we currently compile with c++11, which doesn't define
// std::make_unique, but on MSVC the newest C++ version is always used, which
// includes std::make_unique. If an argument from the std namespace is used, it
// will cause ADL to find std::make_unique, and an unqualified call to
// make_unique will be ambiguous. We can work around this by fully qualifying
// the call (i.e. wabt::make_unique), but it's simpler to just use a different
// name. It's also more consistent with other names in the wabt namespace,
// which use CamelCase.
template <typename T, typename... Args>
std::unique_ptr<T> MakeUnique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

}  // namespace wabt

#endif  // WABT_MAKE_UNIQUE_H_

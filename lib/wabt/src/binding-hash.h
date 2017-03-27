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

#ifndef WABT_BINDING_HASH_H_
#define WABT_BINDING_HASH_H_

#include <string>
#include <vector>
#include <unordered_map>

#include "common.h"

namespace wabt {

struct Binding {
  explicit Binding(int index) : index(index) {
    WABT_ZERO_MEMORY(loc);
  }
  Binding(const Location& loc, int index) : loc(loc), index(index) {}

  Location loc;
  int index;
};

// This class derives from a C++ container, which is usually not advisable
// because they don't have virtual destructors. So don't delete a BindingHash
// object through a pointer to std::unordered_multimap.
class BindingHash : public std::unordered_multimap<std::string, Binding> {
 public:
  typedef void (*DuplicateCallback)(const value_type& a,
                                    const value_type& b,
                                    void* user_data);

  void find_duplicates(DuplicateCallback callback, void* user_data) const;

  int find_index(const StringSlice& name) const {
    auto iter = find(string_slice_to_string(name));
    if (iter != end())
      return iter->second.index;
    return -1;
  }

 private:
  typedef std::vector<const value_type*> ValueTypeVector;

  void create_duplicates_vector(ValueTypeVector* out_duplicates) const;
  void sort_duplicates_vector_by_location(ValueTypeVector* duplicates) const;
  void call_callbacks(const ValueTypeVector& duplicates,
                      DuplicateCallback callback,
                      void* user_data) const;
};

}  // namespace wabt

#endif /* WABT_BINDING_HASH_H_ */

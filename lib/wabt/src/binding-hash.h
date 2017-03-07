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

#include "common.h"
#include "vector.h"

namespace wabt {

struct Binding {
  Location loc;
  StringSlice name;
  int index;
};

struct BindingHashEntry {
  Binding binding;
  struct BindingHashEntry* next;
  struct BindingHashEntry* prev; /* only valid when this entry is unused */
};
WABT_DEFINE_VECTOR(binding_hash_entry, BindingHashEntry);

struct BindingHash {
  BindingHashEntryVector entries;
  BindingHashEntry* free_head;
};

Binding* insert_binding(BindingHash*, const StringSlice*);
void remove_binding(BindingHash*, const StringSlice*);
bool hash_entry_is_free(const BindingHashEntry*);
/* returns -1 if the name is not in the hash */
int find_binding_index_by_name(const BindingHash*, const StringSlice* name);
void destroy_binding_hash(BindingHash*);

}  // namespace wabt

#endif /* WABT_BINDING_HASH_H_ */

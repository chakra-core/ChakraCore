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

#include "binding-hash.h"

#define INITIAL_HASH_CAPACITY 8

namespace wabt {

static size_t hash_name(const StringSlice* name) {
  // FNV-1a hash
  const uint32_t fnv_prime = 0x01000193;
  const uint8_t* bp = reinterpret_cast<const uint8_t*>(name->start);
  const uint8_t* be = bp + name->length;
  uint32_t hval = 0x811c9dc5;
  while (bp < be) {
    hval ^= static_cast<uint32_t>(*bp++);
    hval *= fnv_prime;
  }
  return hval;
}

static BindingHashEntry* hash_main_entry(const BindingHash* hash,
                                         const StringSlice* name) {
  return &hash->entries.data[hash_name(name) % hash->entries.capacity];
}

bool hash_entry_is_free(const BindingHashEntry* entry) {
  return !entry->binding.name.start;
}

static BindingHashEntry* hash_new_entry(BindingHash* hash,
                                        const StringSlice* name) {
  BindingHashEntry* entry = hash_main_entry(hash, name);
  if (!hash_entry_is_free(entry)) {
    assert(hash->free_head);
    BindingHashEntry* free_entry = hash->free_head;
    hash->free_head = free_entry->next;
    if (free_entry->next)
      free_entry->next->prev = nullptr;

    /* our main position is already claimed. Check to see if the entry in that
     * position is in its main position */
    BindingHashEntry* other_entry = hash_main_entry(hash, &entry->binding.name);
    if (other_entry == entry) {
      /* yes, so add this new entry to the chain, even if it is already there */
      /* add as the second entry in the chain */
      free_entry->next = entry->next;
      entry->next = free_entry;
      entry = free_entry;
    } else {
      /* no, move the entry to the free entry */
      assert(!hash_entry_is_free(other_entry));
      while (other_entry->next != entry)
        other_entry = other_entry->next;

      other_entry->next = free_entry;
      *free_entry = *entry;
      entry->next = nullptr;
    }
  } else {
    /* remove from the free list */
    if (entry->next)
      entry->next->prev = entry->prev;
    if (entry->prev)
      entry->prev->next = entry->next;
    else
      hash->free_head = entry->next;
    entry->next = nullptr;
  }

  WABT_ZERO_MEMORY(entry->binding);
  entry->binding.name = *name;
  entry->prev = nullptr;
  /* entry->next is set above */
  return entry;
}

static void hash_resize(BindingHash* hash, size_t desired_capacity) {
  BindingHash new_hash;
  WABT_ZERO_MEMORY(new_hash);
  /* TODO(binji): better plural */
  reserve_binding_hash_entrys(&new_hash.entries, desired_capacity);

  /* update the free list */
  size_t i;
  for (i = 0; i < new_hash.entries.capacity; ++i) {
    BindingHashEntry* entry = &new_hash.entries.data[i];
    if (new_hash.free_head)
      new_hash.free_head->prev = entry;

    WABT_ZERO_MEMORY(entry->binding.name);
    entry->next = new_hash.free_head;
    new_hash.free_head = entry;
  }
  new_hash.free_head->prev = nullptr;

  /* copy from the old hash to the new hash */
  for (i = 0; i < hash->entries.capacity; ++i) {
    BindingHashEntry* old_entry = &hash->entries.data[i];
    if (hash_entry_is_free(old_entry))
      continue;

    StringSlice* name = &old_entry->binding.name;
    BindingHashEntry* new_entry = hash_new_entry(&new_hash, name);
    new_entry->binding = old_entry->binding;
  }

  /* we are sharing the StringSlices, so we only need to destroy the old
   * binding vector */
  destroy_binding_hash_entry_vector(&hash->entries);
  *hash = new_hash;
}

Binding* insert_binding(BindingHash* hash, const StringSlice* name) {
  if (hash->entries.size == 0)
    hash_resize(hash, INITIAL_HASH_CAPACITY);

  if (!hash->free_head) {
    /* no more free space, allocate more */
    hash_resize(hash, hash->entries.capacity * 2);
  }

  BindingHashEntry* entry = hash_new_entry(hash, name);
  assert(entry);
  hash->entries.size++;
  return &entry->binding;
}

int find_binding_index_by_name(const BindingHash* hash,
                               const StringSlice* name) {
  if (hash->entries.capacity == 0)
    return -1;

  BindingHashEntry* entry = hash_main_entry(hash, name);
  do {
    if (string_slices_are_equal(&entry->binding.name, name))
      return entry->binding.index;

    entry = entry->next;
  } while (entry && !hash_entry_is_free(entry));
  return -1;
}

void remove_binding(BindingHash* hash, const StringSlice* name) {
  int index = find_binding_index_by_name(hash, name);
  if (index == -1)
    return;

  BindingHashEntry* entry = &hash->entries.data[index];
  destroy_string_slice(&entry->binding.name);
  WABT_ZERO_MEMORY(*entry);
}

static void destroy_binding_hash_entry(BindingHashEntry* entry) {
  destroy_string_slice(&entry->binding.name);
}

void destroy_binding_hash(BindingHash* hash) {
  /* Can't use WABT_DESTROY_VECTOR_AND_ELEMENTS, because it loops over size, not
   * capacity. */
  size_t i;
  for (i = 0; i < hash->entries.capacity; ++i)
    destroy_binding_hash_entry(&hash->entries.data[i]);
  destroy_binding_hash_entry_vector(&hash->entries);
}

}  // namespace wabt

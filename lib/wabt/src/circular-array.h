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

#ifndef WABT_CIRCULAR_ARRAY_H_
#define WABT_CIRCULAR_ARRAY_H_

#include <array>
#include <cassert>
#include <cstddef>
#include <type_traits>

namespace wabt {

// TODO(karlschimpf) Complete the API
// Note: Capacity must be a power of 2.
template<class T, size_t kCapacity>
class CircularArray {
 public:
  typedef T value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  CircularArray() {
    static_assert(kCapacity && ((kCapacity & (kCapacity - 1)) == 0),
                  "Capacity must be a power of 2.");
  }

  CircularArray(const CircularArray&) = default;
  CircularArray& operator =(const CircularArray&) = default;

  CircularArray(CircularArray&&) = default;
  CircularArray& operator =(CircularArray&&) = default;

  ~CircularArray() {
    clear();
  }

  reference at(size_type index) {
    assert(index < size_);
    return (*this)[index];
  }

  const_reference at(size_type index) const {
    assert(index < size_);
    return (*this)[index];
  }

  reference operator[](size_type index) {
    return contents_[position(index)];
  }

  const_reference operator[](size_type index) const {
    return contents_[position(index)];
  }

  reference back() {
    return at(size_ - 1);
  }

  const_reference back() const {
    return at(size_  - 1);
  }

  bool empty() const { return size_ == 0; }

  reference front() {
    return at(0);
  }

  const_reference front() const {
    return at(0);
  }

  size_type max_size() const { return kCapacity; }

  void pop_back() {
    assert(size_ > 0);
    SetElement(back());
    --size_;
  }

  void pop_front() {
    assert(size_ > 0);
    SetElement(front());
    front_ = (front_ + 1) & kMask;
    --size_;
  }

  void push_back(const value_type& value) {
    assert(size_ < kCapacity);
    SetElement(at(size_++), value);
  }

  size_type size() const { return size_; }

  void clear() {
    while (!empty()) {
      pop_back();
    }
  }

 private:
  static const size_type kMask = kCapacity - 1;

  size_t position(size_t index) const { return (front_ + index) & kMask; }

  template <typename... Args>
  void SetElement(reference element, Args&&... args) {
    element.~T();
    new (&element) T(std::forward<Args>(args)...);
  }

  std::array<T, kCapacity> contents_;
  size_type size_ = 0;
  size_type front_ = 0;
};

}

#endif // WABT_CIRCULAR_ARRAY_H_

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

#ifndef WABT_STRING_VIEW_H_
#define WABT_STRING_VIEW_H_

#include <cassert>
#include <functional>
#include <iterator>
#include <string>

#include "src/hash-util.h"

namespace wabt {

// This is a simplified implemention of C++17's basic_string_view template.
//
// Missing features:
// * Not a template
// * No allocator support
// * Some functions are not implemented
// * Asserts instead of exceptions
// * Some functions are not constexpr because we don't compile in C++17 mode

class string_view {
 public:
  typedef std::char_traits<char> traits_type;
  typedef char value_type;
  typedef char* pointer;
  typedef const char* const_pointer;
  typedef char& reference;
  typedef const char& const_reference;
  typedef const char* const_iterator;
  typedef const_iterator iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef const_reverse_iterator reverse_iterator;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  static const size_type npos = size_type(-1);

  // construction and assignment
  constexpr string_view() noexcept;
  constexpr string_view(const string_view&) noexcept = default;
  string_view& operator=(const string_view&) noexcept = default;
  string_view(const std::string& str) noexcept;
  /*constexpr*/ string_view(const char* str);
  constexpr string_view(const char* str, size_type len);

  // iterator support
  constexpr const_iterator begin() const noexcept;
  constexpr const_iterator end() const noexcept;
  constexpr const_iterator cbegin() const noexcept;
  constexpr const_iterator cend() const noexcept;
  const_reverse_iterator rbegin() const noexcept;
  const_reverse_iterator rend() const noexcept;
  const_reverse_iterator crbegin() const noexcept;
  const_reverse_iterator crend() const noexcept;

  // capacity
  constexpr size_type size() const noexcept;
  constexpr size_type length() const noexcept;
  constexpr size_type max_size() const noexcept;
  constexpr bool empty() const noexcept;

  // element access
  constexpr const_reference operator[](size_type pos) const;
  /*constexpr*/ const_reference at(size_type pos) const;
  /*constexpr*/ const_reference front() const;
  /*constexpr*/ const_reference back() const;
  constexpr const_pointer data() const noexcept;

  // modifiers
  /*constexpr*/ void remove_prefix(size_type n);
  /*constexpr*/ void remove_suffix(size_type n);
  /*constexpr*/ void swap(string_view& s) noexcept;

  // string operations
  explicit operator std::string() const;
  std::string to_string() const;
  size_type copy(char* s, size_type n, size_type pos = 0) const;
  /*constexpr*/ string_view substr(size_type pos = 0, size_type n = npos) const;
  /*constexpr*/ int compare(string_view s) const noexcept;
  /*constexpr*/ int compare(size_type pos1, size_type n1, string_view s) const;
  /*constexpr*/ int compare(size_type pos1,
                            size_type n1,
                            string_view s,
                            size_type pos2,
                            size_type n2) const;
  /*constexpr*/ int compare(const char* s) const;
  /*constexpr*/ int compare(size_type pos1, size_type n1, const char* s) const;
  /*constexpr*/ int compare(size_type pos1,
                            size_type n1,
                            const char* s,
                            size_type n2) const;
  /*constexpr*/ size_type find(string_view s, size_type pos = 0) const noexcept;
  /*constexpr*/ size_type find(char c, size_type pos = 0) const noexcept;
  /*constexpr*/ size_type find(const char* s, size_type pos, size_type n) const;
  /*constexpr*/ size_type find(const char* s, size_type pos = 0) const;
  /*constexpr*/ size_type rfind(string_view s, size_type pos = npos) const
      noexcept;
  /*constexpr*/ size_type rfind(char c, size_type pos = npos) const noexcept;
  /*constexpr*/ size_type rfind(const char* s,
                                size_type pos,
                                size_type n) const;
  /*constexpr*/ size_type rfind(const char* s, size_type pos = npos) const;
  /*constexpr*/ size_type find_first_of(string_view s, size_type pos = 0) const
      noexcept;
  /*constexpr*/ size_type find_first_of(char c, size_type pos = 0) const
      noexcept;
  /*constexpr*/ size_type find_first_of(const char* s,
                                        size_type pos,
                                        size_type n) const;
  /*constexpr*/ size_type find_first_of(const char* s, size_type pos = 0) const;
  /*constexpr*/ size_type find_last_of(string_view s,
                                       size_type pos = npos) const noexcept;
  /*constexpr*/ size_type find_last_of(char c, size_type pos = npos) const
      noexcept;
  /*constexpr*/ size_type find_last_of(const char* s,
                                       size_type pos,
                                       size_type n) const;
  /*constexpr*/ size_type find_last_of(const char* s,
                                       size_type pos = npos) const;

  // TODO(binji): These are defined by C++17 basic_string_view but not
  // implemented here.
#if 0
  constexpr size_type find_first_not_of(string_view s, size_type pos = 0) const
      noexcept;
  constexpr size_type find_first_not_of(char c, size_type pos = 0) const
      noexcept;
  constexpr size_type find_first_not_of(const char* s,
                                        size_type pos,
                                        size_type n) const;
  constexpr size_type find_first_not_of(const char* s, size_type pos = 0) const;
  constexpr size_type find_last_not_of(string_view s,
                                       size_type pos = npos) const noexcept;
  constexpr size_type find_last_not_of(char c, size_type pos = npos) const
      noexcept;
  constexpr size_type find_last_not_of(const char* s,
                                       size_type pos,
                                       size_type n) const;
  constexpr size_type find_last_not_of(const char* s,
                                       size_type pos = npos) const;
#endif

 private:
  const char* data_;
  size_type size_;
};

// non-member comparison functions
inline bool operator==(string_view x, string_view y) noexcept {
  return x.compare(y) == 0;
}

inline bool operator!=(string_view x, string_view y) noexcept {
  return x.compare(y) != 0;
}

inline bool operator<(string_view x, string_view y) noexcept {
  return x.compare(y) < 0;
}

inline bool operator>(string_view x, string_view y) noexcept {
  return x.compare(y) > 0;
}

inline bool operator<=(string_view x, string_view y) noexcept {
  return x.compare(y) <= 0;
}

inline bool operator>=(string_view x, string_view y) noexcept {
  return x.compare(y) >= 0;
}

inline constexpr string_view::string_view() noexcept
    : data_(nullptr), size_(0) {}

inline string_view::string_view(const std::string& str) noexcept
    : data_(str.data()), size_(str.size()) {}

inline string_view::string_view(const char* str)
    : data_(str), size_(traits_type::length(str)) {}

inline constexpr string_view::string_view(const char* str, size_type len)
    : data_(str), size_(len) {}

inline constexpr string_view::const_iterator string_view::begin() const
    noexcept {
  return data_;
}

inline constexpr string_view::const_iterator string_view::end() const noexcept {
  return data_ + size_;
}

inline constexpr string_view::const_iterator string_view::cbegin() const
    noexcept {
  return data_;
}

inline constexpr string_view::const_iterator string_view::cend() const
    noexcept {
  return data_ + size_;
}

inline string_view::const_reverse_iterator string_view::rbegin() const
    noexcept {
  return const_reverse_iterator(end());
}

inline string_view::const_reverse_iterator string_view::rend() const noexcept {
  return const_reverse_iterator(begin());
}

inline string_view::const_reverse_iterator string_view::crbegin() const
    noexcept {
  return const_reverse_iterator(cend());
}

inline string_view::const_reverse_iterator string_view::crend() const noexcept {
  return const_reverse_iterator(cbegin());
}

constexpr inline string_view::size_type string_view::size() const noexcept {
  return size_;
}

constexpr inline string_view::size_type string_view::length() const noexcept {
  return size_;
}

constexpr inline bool string_view::empty() const noexcept {
  return size_ == 0;
}

constexpr inline string_view::const_reference string_view::operator[](
    size_type pos) const {
  return data_[pos];
}

inline string_view::const_reference string_view::at(size_type pos) const {
  assert(pos < size_);
  return data_[pos];
}

inline string_view::const_reference string_view::front() const {
  assert(!empty());
  return *data_;
}

inline string_view::const_reference string_view::back() const {
  assert(!empty());
  return data_[size_ - 1];
}

constexpr inline string_view::const_pointer string_view::data() const noexcept {
  return data_;
}

} // namespace wabt

namespace std {

// hash support
template <>
struct hash<::wabt::string_view> {
  ::wabt::hash_code operator()(const ::wabt::string_view& sv) {
    return ::wabt::HashRange(sv.begin(), sv.end());
  }
};

}

#endif  // WABT_STRING_VIEW_H_

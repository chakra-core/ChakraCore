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

#include "string-view.h"

#include <algorithm>
#include <limits>

namespace wabt {

void string_view::remove_prefix(size_type n) {
  assert(n <= size_);
  data_ += n;
  size_ -= n;
}

void string_view::remove_suffix(size_type n) {
  assert(n <= size_);
  size_ -= n;
}

void string_view::swap(string_view& s) noexcept {
  std::swap(data_, s.data_);
  std::swap(size_, s.size_);
}

string_view::operator std::string() const {
  return std::string(data_, size_);
}

std::string string_view::to_string() const {
  return std::string(data_, size_);
}

constexpr string_view::size_type string_view::max_size() const noexcept {
  return std::numeric_limits<size_type>::max();
}

string_view::size_type string_view::copy(char* s,
                                         size_type n,
                                         size_type pos) const {
  assert(pos <= size_);
  size_t count = std::min(n, size_ - pos);
  traits_type::copy(s, data_ + pos, count);
  return count;
}

string_view string_view::substr(size_type pos, size_type n) const {
  assert(pos <= size_);
  size_t count = std::min(n, size_ - pos);
  return string_view(data_ + pos, count);
}

int string_view::compare(string_view s) const noexcept {
  size_type rlen = std::min(size_, s.size_);
  int result = traits_type::compare(data_, s.data_, rlen);
  if (result != 0 || size_ == s.size_) {
    return result;
  }
  return size_ < s.size_ ? -1 : 1;
}

int string_view::compare(size_type pos1, size_type n1, string_view s) const {
  return substr(pos1, n1).compare(s);
}

int string_view::compare(size_type pos1,
                         size_type n1,
                         string_view s,
                         size_type pos2,
                         size_type n2) const {
  return substr(pos1, n1).compare(s.substr(pos2, n2));
}

int string_view::compare(const char* s) const {
  return compare(string_view(s));
}

int string_view::compare(size_type pos1, size_type n1, const char* s) const {
  return substr(pos1, n1).compare(string_view(s));
}

int string_view::compare(size_type pos1,
                         size_type n1,
                         const char* s,
                         size_type n2) const {
  return substr(pos1, n1).compare(string_view(s, n2));
}

string_view::size_type string_view::find(string_view s, size_type pos) const
    noexcept {
  pos = std::min(pos, size_);
  const_iterator iter = std::search(begin() + pos, end(), s.begin(), s.end());
  return iter == end() ? npos : iter - begin();
}

string_view::size_type string_view::find(char c, size_type pos) const noexcept {
  return find(string_view(&c, 1), pos);
}

string_view::size_type string_view::find(const char* s,
                                         size_type pos,
                                         size_type n) const {
  return find(string_view(s, n), pos);
}

string_view::size_type string_view::find(const char* s, size_type pos) const {
  return find(string_view(s), pos);
}

string_view::size_type string_view::rfind(string_view s, size_type pos) const
    noexcept {
  pos = std::min(std::min(pos, size_  - s.size_) + s.size_, size_);
  reverse_iterator iter = std::search(reverse_iterator(begin() + pos), rend(),
                                      s.rbegin(), s.rend());
  return iter == rend() ? npos : (rend() - iter - s.size_);
}

string_view::size_type string_view::rfind(char c, size_type pos) const
    noexcept {
  return rfind(string_view(&c, 1), pos);
}

string_view::size_type string_view::rfind(const char* s,
                                          size_type pos,
                                          size_type n) const {
  return rfind(string_view(s, n), pos);
}

string_view::size_type string_view::rfind(const char* s, size_type pos) const {
  return rfind(string_view(s), pos);
}

string_view::size_type string_view::find_first_of(string_view s,
                                                  size_type pos) const
    noexcept {
  pos = std::min(pos, size_);
  const_iterator iter =
      std::find_first_of(begin() + pos, end(), s.begin(), s.end());
  return iter == end() ? npos : iter - begin();
}

string_view::size_type string_view::find_first_of(char c, size_type pos) const
    noexcept {
  return find_first_of(string_view(&c, 1), pos);
}

string_view::size_type string_view::find_first_of(const char* s,
                                                  size_type pos,
                                                  size_type n) const {
  return find_first_of(string_view(s, n), pos);
}

string_view::size_type string_view::find_first_of(const char* s,
                                                  size_type pos) const {
  return find_first_of(string_view(s), pos);
}

string_view::size_type string_view::find_last_of(string_view s,
                                                 size_type pos) const noexcept {
  pos = std::min(pos, size_ - 1);
  reverse_iterator iter = std::find_first_of(
      reverse_iterator(begin() + pos + 1), rend(), s.begin(), s.end());
  return iter == rend() ? npos : (rend() - iter - 1);
}

string_view::size_type string_view::find_last_of(char c, size_type pos) const
    noexcept {
  return find_last_of(string_view(&c, 1), pos);
}

string_view::size_type string_view::find_last_of(const char* s,
                                                 size_type pos,
                                                 size_type n) const {
  return find_last_of(string_view(s, n), pos);
}

string_view::size_type string_view::find_last_of(const char* s,
                                                 size_type pos) const {
  return find_last_of(string_view(s), pos);
}

}  // namespace wabt

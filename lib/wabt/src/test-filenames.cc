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

#include "gtest/gtest.h"

#include "src/filenames.h"

using namespace wabt;

namespace {

void assert_string_view_eq(const char* s, string_view sv) {
  size_t len = std::strlen(s);
  ASSERT_EQ(len, sv.size());
  for (size_t i = 0; i < len; ++i) {
    ASSERT_EQ(s[i], sv[i]);
  }
}

}  // end anonymous namespace

TEST(filenames, GetBasename) {
  assert_string_view_eq("foo.txt", GetBasename("/bar/dir/foo.txt"));
  assert_string_view_eq("foo.txt", GetBasename("bar/dir/foo.txt"));
  assert_string_view_eq("foo.txt", GetBasename("/foo.txt"));
  assert_string_view_eq("foo.txt", GetBasename("\\bar\\dir\\foo.txt"));
  assert_string_view_eq("foo.txt", GetBasename("bar\\dir\\foo.txt"));
  assert_string_view_eq("foo.txt", GetBasename("\\foo.txt"));
  assert_string_view_eq("foo.txt", GetBasename("foo.txt"));
  assert_string_view_eq("foo", GetBasename("foo"));
  assert_string_view_eq("", GetBasename(""));
}

TEST(filenames, GetExtension) {
  assert_string_view_eq(".txt", GetExtension("/bar/dir/foo.txt"));
  assert_string_view_eq(".txt", GetExtension("bar/dir/foo.txt"));
  assert_string_view_eq(".txt", GetExtension("foo.txt"));
  assert_string_view_eq("", GetExtension("foo"));
  assert_string_view_eq("", GetExtension(""));
}

TEST(filenames, StripExtension) {
  assert_string_view_eq("/bar/dir/foo", StripExtension("/bar/dir/foo.txt"));
  assert_string_view_eq("bar/dir/foo", StripExtension("bar/dir/foo.txt"));
  assert_string_view_eq("foo", StripExtension("foo.txt"));
  assert_string_view_eq("foo", StripExtension("foo"));
  assert_string_view_eq("", StripExtension(""));
}

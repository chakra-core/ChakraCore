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

#include "src/string-view.h"

#include <cstring>
#include <functional>

using namespace wabt;

namespace {

void assert_string_view_eq(const char* s, string_view sv) {
  size_t len = std::strlen(s);
  ASSERT_EQ(len, sv.size());
  for (size_t i = 0; i < len; ++i) {
    ASSERT_EQ(s[i], sv[i]);
  }
}

constexpr string_view::size_type npos = string_view::npos;

}  // end anonymous namespace

TEST(string_view, default_constructor) {
  assert_string_view_eq("", string_view());
}

TEST(string_view, copy_constructor) {
  string_view sv1("copy");
  assert_string_view_eq("copy", string_view(sv1));

  string_view sv2;
  assert_string_view_eq("", string_view(sv2));
}

TEST(string_view, assignment_operator) {
  string_view sv1;
  sv1 = string_view("assign");
  assert_string_view_eq("assign", sv1);

  string_view sv2;
  sv2 = string_view();
  assert_string_view_eq("", sv2);
}

TEST(string_view, string_constructor) {
  assert_string_view_eq("", string_view(std::string()));
  assert_string_view_eq("string", string_view(std::string("string")));
}

TEST(string_view, cstr_constructor) {
  assert_string_view_eq("", string_view(""));
  assert_string_view_eq("cstr", string_view("cstr"));
}

TEST(string_view, cstr_len_constructor) {
  assert_string_view_eq("", string_view("foo-bar-baz", 0));
  assert_string_view_eq("foo", string_view("foo-bar-baz", 3));
  assert_string_view_eq("foo-bar", string_view("foo-bar-baz", 7));
}

TEST(string_view, begin_end) {
  string_view sv("012345");

  char count = 0;
  for (auto iter = sv.begin(), end = sv.end(); iter != end; ++iter) {
    ASSERT_EQ('0' + count, *iter);
    ++count;
  }
  ASSERT_EQ(6, count);
}

TEST(string_view, cbegin_cend) {
  const string_view sv("012345");

  char count = 0;
  for (auto iter = sv.cbegin(), end = sv.cend(); iter != end; ++iter) {
    ASSERT_EQ('0' + count, *iter);
    ++count;
  }
  ASSERT_EQ(6, count);
}

TEST(string_view, rbegin_rend) {
  string_view sv("012345");

  char count = 0;
  for (auto iter = sv.rbegin(), end = sv.rend(); iter != end; ++iter) {
    ASSERT_EQ('5' - count, *iter);
    ++count;
  }
  ASSERT_EQ(6, count);
}

TEST(string_view, crbegin_crend) {
  const string_view sv("012345");

  char count = 0;
  for (auto iter = sv.crbegin(), end = sv.crend(); iter != end; ++iter) {
    ASSERT_EQ('5' - count, *iter);
    ++count;
  }
  ASSERT_EQ(6, count);
}

TEST(string_view, size) {
  string_view sv1;
  ASSERT_EQ(0U, sv1.size());

  string_view sv2("");
  ASSERT_EQ(0U, sv2.size());

  string_view sv3("hello");
  ASSERT_EQ(5U, sv3.size());
}

TEST(string_view, length) {
  string_view sv1;
  ASSERT_EQ(0U, sv1.length());

  string_view sv2("hello");
  ASSERT_EQ(5U, sv2.length());
}

TEST(string_view, empty) {
  string_view sv1;
  ASSERT_TRUE(sv1.empty());

  string_view sv2("bye");
  ASSERT_FALSE(sv2.empty());
}

TEST(string_view, operator_bracket) {
  string_view sv("words");
  ASSERT_EQ('w', sv[0]);
  ASSERT_EQ('o', sv[1]);
  ASSERT_EQ('r', sv[2]);
  ASSERT_EQ('d', sv[3]);
  ASSERT_EQ('s', sv[4]);
}

TEST(string_view, at) {
  string_view sv("words");
  ASSERT_EQ('w', sv.at(0));
  ASSERT_EQ('o', sv.at(1));
  ASSERT_EQ('r', sv.at(2));
  ASSERT_EQ('d', sv.at(3));
  ASSERT_EQ('s', sv.at(4));
}

TEST(string_view, front) {
  string_view sv("words");
  ASSERT_EQ('w', sv.front());
}

TEST(string_view, back) {
  string_view sv("words");
  ASSERT_EQ('s', sv.back());
}

TEST(string_view, data) {
  const char* cstr = "words";
  string_view sv(cstr);
  ASSERT_EQ(cstr, sv.data());
}

TEST(string_view, remove_prefix) {
  string_view sv("words");
  sv.remove_prefix(2);
  assert_string_view_eq("rds", sv);
}

TEST(string_view, remove_suffix) {
  string_view sv("words");
  sv.remove_suffix(2);
  assert_string_view_eq("wor", sv);
}

TEST(string_view, swap) {
  string_view sv1("hello");
  string_view sv2("bye");

  sv1.swap(sv2);

  assert_string_view_eq("bye", sv1);
  assert_string_view_eq("hello", sv2);
}

TEST(string_view, operator_std_string) {
  string_view sv1("hi");
  std::string s(sv1);

  ASSERT_EQ(2U, s.size());
  ASSERT_EQ('h', s[0]);
  ASSERT_EQ('i', s[1]);
}

TEST(string_view, copy) {
  string_view sv("words");
  char buffer[10] = {0};

  sv.copy(buffer, 10, 2);
  ASSERT_EQ('r', buffer[0]);
  ASSERT_EQ('d', buffer[1]);
  ASSERT_EQ('s', buffer[2]);
  for (int i = 3; i < 10; ++i) {
    ASSERT_EQ(0, buffer[i]);
  }
}

TEST(string_view, substr) {
  string_view sv1("abcdefghij");
  string_view sv2 = sv1.substr(2, 3);
  assert_string_view_eq("cde", sv2);
}

TEST(string_view, compare0) {
  ASSERT_TRUE(string_view("meat").compare(string_view("meet")) < 0);
  ASSERT_TRUE(string_view("rest").compare(string_view("rate")) > 0);
  ASSERT_TRUE(string_view("equal").compare(string_view("equal")) == 0);
  ASSERT_TRUE(string_view("star").compare(string_view("start")) < 0);
  ASSERT_TRUE(string_view("finished").compare(string_view("fin")) > 0);
}

TEST(string_view, compare1) {
  ASSERT_TRUE(string_view("abcdef").compare(2, 2, string_view("ca")) > 0);
  ASSERT_TRUE(string_view("abcdef").compare(2, 2, string_view("cd")) == 0);
  ASSERT_TRUE(string_view("abcdef").compare(2, 2, string_view("cz")) < 0);
}

TEST(string_view, compare2) {
  ASSERT_TRUE(string_view("abcdef").compare(2, 2, string_view("_ca__"), 1, 2) >
              0);
  ASSERT_TRUE(string_view("abcdef").compare(2, 2, string_view("_cd__"), 1, 2) ==
              0);
  ASSERT_TRUE(string_view("abcdef").compare(2, 2, string_view("_cz__"), 1, 2) <
              0);
}

TEST(string_view, compare3) {
  ASSERT_TRUE(string_view("abcdef").compare("aaaa") > 0);
  ASSERT_TRUE(string_view("abcdef").compare("abcdef") == 0);
  ASSERT_TRUE(string_view("abcdef").compare("zzzz") < 0);
}

TEST(string_view, compare4) {
  ASSERT_TRUE(string_view("abcdef").compare(2, 2, "ca") > 0);
  ASSERT_TRUE(string_view("abcdef").compare(2, 2, "cd") == 0);
  ASSERT_TRUE(string_view("abcdef").compare(2, 2, "cz") < 0);
}

TEST(string_view, compare5) {
  ASSERT_TRUE(string_view("abcdef").compare(2, 2, "ca____", 2) > 0);
  ASSERT_TRUE(string_view("abcdef").compare(2, 2, "cd___", 2) == 0);
  ASSERT_TRUE(string_view("abcdef").compare(2, 2, "cz__", 2) < 0);
}

TEST(string_view, find0) {
  ASSERT_EQ(0U, string_view("find fins").find(string_view("fin")));
  ASSERT_EQ(5U, string_view("find fins").find(string_view("fin"), 1));
  ASSERT_EQ(npos, string_view("find fins").find(string_view("fin"), 6));
}

TEST(string_view, find1) {
  ASSERT_EQ(0U, string_view("012340123").find('0'));
  ASSERT_EQ(5U, string_view("012340123").find('0', 2));
  ASSERT_EQ(npos, string_view("012340123").find('0', 6));
}

TEST(string_view, find2) {
  ASSERT_EQ(1U, string_view("012340123").find("12345", 0, 2));
  ASSERT_EQ(6U, string_view("012340123").find("12345", 3, 2));
  ASSERT_EQ(npos, string_view("012340123").find("12345", 10, 2));
}

TEST(string_view, find3) {
  ASSERT_EQ(1U, string_view("012340123").find("12"));
  ASSERT_EQ(6U, string_view("012340123").find("12", 2));
  ASSERT_EQ(npos, string_view("012340123").find("12", 10));
}

TEST(string_view, rfind0) {
  ASSERT_EQ(5U, string_view("find fins").rfind(string_view("fin")));
  ASSERT_EQ(0U, string_view("find fins").rfind(string_view("fin"), 4));
  ASSERT_EQ(npos, string_view("find fins").rfind(string_view("no")));
  ASSERT_EQ(npos, string_view("foo").rfind(string_view("foobar")));
}

TEST(string_view, rfind1) {
  ASSERT_EQ(5U, string_view("012340123").rfind('0'));
  ASSERT_EQ(0U, string_view("012340123").rfind('0', 2));
  ASSERT_EQ(npos, string_view("012340123").rfind('9'));
}

TEST(string_view, rfind2) {
  ASSERT_EQ(6U, string_view("012340123").rfind("12345", npos, 2));
  ASSERT_EQ(1U, string_view("012340123").rfind("12345", 4, 2));
  ASSERT_EQ(npos, string_view("012340123").rfind("12345", npos, 5));
  ASSERT_EQ(npos, string_view("012").rfind("12345", npos, 5));
}

TEST(string_view, rfind3) {
  ASSERT_EQ(6U, string_view("012340123").rfind("12"));
  ASSERT_EQ(1U, string_view("012340123").rfind("12", 2));
  ASSERT_EQ(npos, string_view("012340123").rfind("12", 0));
  ASSERT_EQ(npos, string_view("012").rfind("12345"));
}

TEST(string_view, find_first_of0) {
  ASSERT_EQ(0U, string_view("0123abc").find_first_of(string_view("0a")));
  ASSERT_EQ(4U, string_view("0123abc").find_first_of(string_view("0a"), 1));
  ASSERT_EQ(npos, string_view("0123abc").find_first_of(string_view("xyz")));
}

TEST(string_view, find_first_of1) {
  ASSERT_EQ(1U, string_view("ahellohi").find_first_of('h'));
  ASSERT_EQ(6U, string_view("ahellohi").find_first_of('h', 2));
  ASSERT_EQ(npos, string_view("ahellohi").find_first_of('z', 2));
}

TEST(string_view, find_first_of2) {
  ASSERT_EQ(0U, string_view("0123abc").find_first_of("0a1b", 0, 2));
  ASSERT_EQ(4U, string_view("0123abc").find_first_of("0a1b", 1, 2));
  ASSERT_EQ(npos, string_view("0123abc").find_first_of("0a1b", 5, 2));
}

TEST(string_view, find_first_of3) {
  ASSERT_EQ(0U, string_view("0123abc").find_first_of("0a"));
  ASSERT_EQ(0U, string_view("0123abc").find_first_of("0a", 0));
  ASSERT_EQ(4U, string_view("0123abc").find_first_of("0a", 1));
  ASSERT_EQ(npos, string_view("0123abc").find_first_of("0a", 5));
}

TEST(string_view, find_last_of0) {
  ASSERT_EQ(4U, string_view("0123abc").find_last_of(string_view("0a")));
  ASSERT_EQ(0U, string_view("0123abc").find_last_of(string_view("0a"), 1));
  ASSERT_EQ(npos, string_view("0123abc").find_last_of(string_view("xyz")));
}

TEST(string_view, find_last_of1) {
  ASSERT_EQ(6U, string_view("ahellohi").find_last_of('h'));
  ASSERT_EQ(1U, string_view("ahellohi").find_last_of('h', 2));
  ASSERT_EQ(npos, string_view("ahellohi").find_last_of('z', 2));
}

TEST(string_view, find_last_of2) {
  ASSERT_EQ(4U, string_view("0123abc").find_last_of("0a1b", npos, 2));
  ASSERT_EQ(0U, string_view("0123abc").find_last_of("0a1b", 1, 2));
  ASSERT_EQ(npos, string_view("0123abc").find_last_of("a1b", 0, 2));
  ASSERT_EQ(npos, string_view("0123abc").find_last_of("xyz", npos, 0));
}

TEST(string_view, find_last_of3) {
  ASSERT_EQ(4U, string_view("0123abc").find_last_of("0a"));
  ASSERT_EQ(4U, string_view("0123abc").find_last_of("0a", npos));
  ASSERT_EQ(0U, string_view("0123abc").find_last_of("0a", 1));
  ASSERT_EQ(npos, string_view("0123abc").find_last_of("a1", 0));
}

TEST(string_view, operator_equal) {
  ASSERT_TRUE(string_view("this") == string_view("this"));
  ASSERT_FALSE(string_view("this") == string_view("that"));
}

TEST(string_view, operator_not_equal) {
  ASSERT_FALSE(string_view("here") != string_view("here"));
  ASSERT_TRUE(string_view("here") != string_view("there"));
}

TEST(string_view, operator_less_than) {
  ASSERT_TRUE(string_view("abc") < string_view("xyz"));
  ASSERT_FALSE(string_view("later") < string_view("earlier"));
  ASSERT_FALSE(string_view("one") < string_view("one"));
}

TEST(string_view, operator_greater_than) {
  ASSERT_TRUE(string_view("much") > string_view("little"));
  ASSERT_FALSE(string_view("future") > string_view("past"));
  ASSERT_FALSE(string_view("now") > string_view("now"));
}

TEST(string_view, operator_less_than_or_equal) {
  ASSERT_TRUE(string_view("abc") <= string_view("xyz"));
  ASSERT_FALSE(string_view("later") <= string_view("earlier"));
  ASSERT_TRUE(string_view("one") <= string_view("one"));
}

TEST(string_view, operator_greater_than_or_equal) {
  ASSERT_TRUE(string_view("much") >= string_view("little"));
  ASSERT_FALSE(string_view("future") >= string_view("past"));
  ASSERT_TRUE(string_view("now") >= string_view("now"));
}

TEST(string_view, hash) {
  std::hash<string_view> hasher;

  ASSERT_NE(hasher(string_view("hello")), hasher(string_view("goodbye")));
  ASSERT_EQ(hasher(string_view("same")), hasher(string_view("same")));
}

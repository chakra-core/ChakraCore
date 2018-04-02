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

#include <memory>

#include "src/circular-array.h"

using namespace wabt;

namespace {

struct TestObject {
  static int construct_count;
  static int destruct_count;

  TestObject(int data = 0, int data2 = 0) : data(data), data2(data2) {
    construct_count++;
  }

  TestObject(const TestObject& other) {
    *this = other;
    construct_count++;
  }

  TestObject& operator=(const TestObject& other) {
    data = other.data;
    data2 = other.data2;
    return *this;
  }

  TestObject(TestObject&& other) { *this = std::move(other); }

  TestObject& operator=(TestObject&& other) {
    data = other.data;
    data2 = other.data2;
    other.moved = true;
    return *this;
  }

  ~TestObject() {
    if (!moved) {
      destruct_count++;
    }
  }

  int data = 0;
  int data2 = 0;
  bool moved = false;
};

// static
int TestObject::construct_count = 0;
// static
int TestObject::destruct_count = 0;

class CircularArrayTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    // Reset to 0 even if the previous test leaked objects to keep the tests
    // independent.
    TestObject::construct_count = 0;
    TestObject::destruct_count = 0;
  }

  virtual void TearDown() {
    ASSERT_EQ(0, TestObject::construct_count - TestObject::destruct_count)
        << "construct count: " << TestObject::construct_count
        << ", destruct_count: " << TestObject::destruct_count;
  }

  template <size_t N>
  void AssertCircularArrayEq(const CircularArray<TestObject, N>& ca,
                             const std::vector<int>& expected) {
    if (expected.empty()) {
      ASSERT_EQ(0U, ca.size());
      ASSERT_TRUE(ca.empty());
    } else {
      ASSERT_EQ(expected.size(), ca.size());
      ASSERT_FALSE(ca.empty());

      ASSERT_EQ(expected.front(), ca.front().data);
      ASSERT_EQ(expected.back(), ca.back().data);

      for (size_t i = 0; i < ca.size(); ++i) {
        ASSERT_EQ(expected[i], ca.at(i).data);
        ASSERT_EQ(expected[i], ca[i].data);
      }
    }
  }
};

}  // end anonymous namespace

// Basic API tests

TEST_F(CircularArrayTest, default_constructor) {
  CircularArray<TestObject, 2> ca;
}

TEST_F(CircularArrayTest, at) {
  CircularArray<TestObject, 2> ca;
  ca.push_back(TestObject(1));
  ca.push_back(TestObject(2));

  ASSERT_EQ(1, ca.at(0).data);
  ASSERT_EQ(2, ca.at(1).data);
}

TEST_F(CircularArrayTest, const_at) {
  CircularArray<TestObject, 2> ca;
  const auto& cca = ca;

  ca.push_back(TestObject(1));
  ca.push_back(TestObject(2));

  ASSERT_EQ(1, cca.at(0).data);
  ASSERT_EQ(2, cca.at(1).data);
}

TEST_F(CircularArrayTest, operator_brackets) {
  CircularArray<TestObject, 2> ca;
  ca.push_back(TestObject(1));
  ca.push_back(TestObject(2));

  ASSERT_EQ(1, ca[0].data);
  ASSERT_EQ(2, ca[1].data);
}

TEST_F(CircularArrayTest, const_operator_brackets) {
  CircularArray<TestObject, 2> ca;
  const auto& cca = ca;

  ca.push_back(TestObject(1));
  ca.push_back(TestObject(2));

  ASSERT_EQ(1, cca[0].data);
  ASSERT_EQ(2, cca[1].data);
}

TEST_F(CircularArrayTest, back) {
  CircularArray<TestObject, 2> ca;

  ca.push_back(TestObject(1));
  ASSERT_EQ(1, ca.back().data);

  ca.push_back(TestObject(2));
  ASSERT_EQ(2, ca.back().data);
}

TEST_F(CircularArrayTest, const_back) {
  CircularArray<TestObject, 2> ca;
  const auto& cca = ca;

  ca.push_back(TestObject(1));
  ASSERT_EQ(1, cca.back().data);

  ca.push_back(TestObject(2));
  ASSERT_EQ(2, cca.back().data);
}

TEST_F(CircularArrayTest, empty) {
  CircularArray<TestObject, 2> ca;

  ASSERT_TRUE(ca.empty());

  ca.push_back(TestObject(1));
  ASSERT_FALSE(ca.empty());

  ca.push_back(TestObject(2));
  ASSERT_FALSE(ca.empty());

  ca.pop_back();
  ASSERT_FALSE(ca.empty());

  ca.pop_back();
  ASSERT_TRUE(ca.empty());
}

TEST_F(CircularArrayTest, front) {
  CircularArray<TestObject, 2> ca;

  ca.push_back(TestObject(1));
  ASSERT_EQ(1, ca.front().data);

  ca.push_back(TestObject(2));
  ASSERT_EQ(1, ca.front().data);
}

TEST_F(CircularArrayTest, const_front) {
  CircularArray<TestObject, 2> ca;
  const auto& cca = ca;

  ca.push_back(TestObject(1));
  ASSERT_EQ(1, cca.front().data);

  ca.push_back(TestObject(2));
  ASSERT_EQ(1, cca.front().data);
}

TEST_F(CircularArrayTest, size) {
  CircularArray<TestObject, 2> ca;

  ASSERT_EQ(0U, ca.size());

  ca.push_back(TestObject(1));
  ASSERT_EQ(1U, ca.size());

  ca.push_back(TestObject(2));
  ASSERT_EQ(2U, ca.size());

  ca.pop_back();
  ASSERT_EQ(1U, ca.size());

  ca.pop_back();
  ASSERT_EQ(0U, ca.size());
}

TEST_F(CircularArrayTest, clear) {
  CircularArray<TestObject, 2> ca;

  ca.push_back(TestObject(1));
  ca.push_back(TestObject(2));
  ASSERT_EQ(2U, ca.size());

  ca.clear();
  ASSERT_EQ(0U, ca.size());
}

// More involved tests

TEST_F(CircularArrayTest, circular) {
  CircularArray<TestObject, 4> ca;

  ca.push_back(TestObject(1));
  AssertCircularArrayEq(ca, {1});

  ca.push_back(TestObject(2));
  AssertCircularArrayEq(ca, {1, 2});

  ca.push_back(TestObject(3));
  AssertCircularArrayEq(ca, {1, 2, 3});

  ca.pop_front();
  AssertCircularArrayEq(ca, {2, 3});

  ca.push_back(TestObject(4));
  AssertCircularArrayEq(ca, {2, 3, 4});

  ca.pop_front();
  AssertCircularArrayEq(ca, {3, 4});

  ca.pop_front();
  AssertCircularArrayEq(ca, {4});

  ca.push_back(TestObject(5));
  AssertCircularArrayEq(ca, {4, 5});

  ca.push_back(TestObject(6));
  AssertCircularArrayEq(ca, {4, 5, 6});

  ca.pop_back();
  AssertCircularArrayEq(ca, {4, 5});

  ca.pop_back();
  AssertCircularArrayEq(ca, {4});

  ca.pop_front();
  AssertCircularArrayEq(ca, {});
}

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

#include "src/intrusive-list.h"
#include "src/make-unique.h"

using namespace wabt;

namespace {

struct TestObject : intrusive_list_base<TestObject> {
  static int creation_count;

  TestObject(int data = 0, int data2 = 0) : data(data), data2(data2) {
    ++creation_count;
  }

  // Allow move.
  TestObject(TestObject&& other) {
    // Don't increment creation_count; we're moving from other.
    *this = std::move(other);
  }

  TestObject& operator=(TestObject&& other) {
    data = other.data;
    data2 = other.data2;
    other.moved = true;
    return *this;
  }

  // Disallow copy.
  TestObject(const TestObject&) = delete;
  TestObject& operator=(const TestObject&) = delete;

  ~TestObject() {
    if (!moved) {
      creation_count--;
    }
  }

  int data;
  int data2;
  bool moved = false;
};

// static
int TestObject::creation_count = 0;

typedef intrusive_list<TestObject> TestObjectList;

class IntrusiveListTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    // Reset to 0 even if the previous test leaked objects to keep the tests
    // independent.
    TestObject::creation_count = 0;
  }

  virtual void TearDown() { ASSERT_EQ(0, TestObject::creation_count); }

  TestObjectList NewList(const std::vector<int>& data_values) {
    TestObjectList result;
    for (auto data_value : data_values)
      result.emplace_back(data_value);
    return result;
  }

  void AssertListEq(const TestObjectList& list,
                    const std::vector<int>& expected) {
    size_t count = 0;
    for (const TestObject& node : list) {
      ASSERT_EQ(expected[count++], node.data);
    }
    ASSERT_EQ(count, expected.size());
  }

  void AssertListEq(const TestObjectList& list,
                    const std::vector<int>& expected,
                    const std::vector<int>& expected2) {
    assert(expected.size() == expected2.size());
    size_t count = 0;
    for (const TestObject& node : list) {
      ASSERT_EQ(expected[count], node.data);
      ASSERT_EQ(expected2[count], node.data2);
      count++;
    }
    ASSERT_EQ(count, expected.size());
  }
};

}  // end anonymous namespace

TEST_F(IntrusiveListTest, default_constructor) {
  TestObjectList list;
}

TEST_F(IntrusiveListTest, node_constructor) {
  TestObjectList list(MakeUnique<TestObject>(1));
  AssertListEq(list, {1});
}

TEST_F(IntrusiveListTest, node_move_constructor) {
  TestObjectList list(TestObject(1));
  AssertListEq(list, {1});
}

TEST_F(IntrusiveListTest, move_constructor) {
  TestObjectList list1 = NewList({1, 2, 3});
  TestObjectList list2(std::move(list1));
  AssertListEq(list1, {});
  AssertListEq(list2, {1, 2, 3});
}

TEST_F(IntrusiveListTest, move_assignment_operator) {
  TestObjectList list1 = NewList({1, 2, 3});
  TestObjectList list2;

  list2 = std::move(list1);
  AssertListEq(list1, {});
  AssertListEq(list2, {1, 2, 3});
}

namespace {

class IntrusiveListIteratorTest : public IntrusiveListTest {
 protected:
  virtual void SetUp() {
    IntrusiveListTest::SetUp();

    list_.emplace_back(1);
    list_.emplace_back(2);
    list_.emplace_back(3);
  }

  virtual void TearDown() {
    list_.clear();

    IntrusiveListTest::TearDown();
  }

  template <typename Iter>
  void TestForward(Iter first, Iter last, const std::vector<int>& expected) {
    size_t count = 0;
    while (first != last) {
      ASSERT_EQ(expected[count], first->data);
      ++first;
      ++count;
    }
    ASSERT_EQ(count, expected.size());
  }

  template <typename Iter>
  void TestBackward(Iter first, Iter last, const std::vector<int>& expected) {
    size_t count = 0;
    while (first != last) {
      --last;
      ASSERT_EQ(expected[count], last->data);
      ++count;
    }
    ASSERT_EQ(count, expected.size());
  }

  TestObjectList list_;
  const TestObjectList& clist_ = list_;
};

}  // end of anonymous namespace

TEST_F(IntrusiveListIteratorTest, begin_end_forward) {
  TestForward(list_.begin(), list_.end(), {1, 2, 3});
  TestForward(clist_.begin(), clist_.end(), {1, 2, 3});
}

TEST_F(IntrusiveListIteratorTest, rbegin_rend_forward) {
  TestForward(list_.rbegin(), list_.rend(), {3, 2, 1});
  TestForward(clist_.rbegin(), clist_.rend(), {3, 2, 1});
}

TEST_F(IntrusiveListIteratorTest, cbegin_cend_forward) {
  TestForward(list_.cbegin(), list_.cend(), {1, 2, 3});
  TestForward(clist_.cbegin(), clist_.cend(), {1, 2, 3});
}

TEST_F(IntrusiveListIteratorTest, crbegin_crend_forward) {
  TestForward(list_.crbegin(), list_.crend(), {3, 2, 1});
  TestForward(clist_.crbegin(), clist_.crend(), {3, 2, 1});
}

TEST_F(IntrusiveListIteratorTest, begin_end_backward) {
  TestBackward(list_.begin(), list_.end(), {3, 2, 1});
  TestBackward(clist_.begin(), clist_.end(), {3, 2, 1});
}

TEST_F(IntrusiveListIteratorTest, rbegin_rend_backward) {
  TestBackward(list_.rbegin(), list_.rend(), {1, 2, 3});
  TestBackward(clist_.rbegin(), clist_.rend(), {1, 2, 3});
}

TEST_F(IntrusiveListIteratorTest, cbegin_cend_backward) {
  TestBackward(list_.cbegin(), list_.cend(), {3, 2, 1});
  TestBackward(clist_.cbegin(), clist_.cend(), {3, 2, 1});
}

TEST_F(IntrusiveListIteratorTest, crbegin_crend_backward) {
  TestBackward(list_.crbegin(), list_.crend(), {1, 2, 3});
  TestBackward(clist_.crbegin(), clist_.crend(), {1, 2, 3});
}

TEST_F(IntrusiveListTest, size_empty) {
  TestObjectList list;
  ASSERT_EQ(0U, list.size());
  ASSERT_TRUE(list.empty());

  list.emplace_back(1);
  ASSERT_EQ(1U, list.size());
  ASSERT_FALSE(list.empty());
}

TEST_F(IntrusiveListTest, front_back) {
  TestObjectList list;

  list.emplace_back(1);
  ASSERT_EQ(1, list.front().data);
  ASSERT_EQ(1, list.back().data);

  list.emplace_back(2);
  ASSERT_EQ(1, list.front().data);
  ASSERT_EQ(2, list.back().data);

  const TestObjectList& clist = list;
  ASSERT_EQ(1, clist.front().data);
  ASSERT_EQ(2, clist.back().data);
}

TEST_F(IntrusiveListTest, emplace_front) {
  TestObjectList list;

  // Pass an additional arg to show that forwarding works properly.
  list.emplace_front(1, 100);
  list.emplace_front(2, 200);
  list.emplace_front(3, 300);
  list.emplace_front(4, 400);

  AssertListEq(list, {4, 3, 2, 1}, {400, 300, 200, 100});
}

TEST_F(IntrusiveListTest, emplace_back) {
  TestObjectList list;

  // Pass an additional arg to show that forwarding works properly.
  list.emplace_back(1, 100);
  list.emplace_back(2, 200);
  list.emplace_back(3, 300);
  list.emplace_back(4, 400);

  AssertListEq(list, {1, 2, 3, 4}, {100, 200, 300, 400});
}

TEST_F(IntrusiveListTest, push_front_pointer) {
  TestObjectList list;

  list.push_front(MakeUnique<TestObject>(1));
  list.push_front(MakeUnique<TestObject>(2));
  list.push_front(MakeUnique<TestObject>(3));

  AssertListEq(list, {3, 2, 1});
}

TEST_F(IntrusiveListTest, push_back_pointer) {
  TestObjectList list;

  list.push_back(MakeUnique<TestObject>(1));
  list.push_back(MakeUnique<TestObject>(2));
  list.push_back(MakeUnique<TestObject>(3));

  AssertListEq(list, {1, 2, 3});
}

TEST_F(IntrusiveListTest, push_front_move) {
  TestObjectList list;

  list.push_front(TestObject(1));
  list.push_front(TestObject(2));
  list.push_front(TestObject(3));

  AssertListEq(list, {3, 2, 1});
}

TEST_F(IntrusiveListTest, push_back_move) {
  TestObjectList list;

  list.push_back(TestObject(1));
  list.push_back(TestObject(2));
  list.push_back(TestObject(3));

  AssertListEq(list, {1, 2, 3});
}

TEST_F(IntrusiveListTest, pop_front) {
  TestObjectList list = NewList({1, 2, 3, 4});

  list.pop_front();
  AssertListEq(list, {2, 3, 4});
  list.pop_front();
  AssertListEq(list, {3, 4});
  list.pop_front();
  AssertListEq(list, {4});
  list.pop_front();
  AssertListEq(list, {});
}

TEST_F(IntrusiveListTest, pop_back) {
  TestObjectList list = NewList({1, 2, 3, 4});

  list.pop_back();
  AssertListEq(list, {1, 2, 3});
  list.pop_back();
  AssertListEq(list, {1, 2});
  list.pop_back();
  AssertListEq(list, {1});
  list.pop_back();
  AssertListEq(list, {});
}

TEST_F(IntrusiveListTest, extract_front) {
  TestObjectList list = NewList({1, 2, 3});

  std::unique_ptr<TestObject> t1(list.extract_front());
  ASSERT_EQ(1, t1->data);
  AssertListEq(list, {2, 3});

  std::unique_ptr<TestObject> t2(list.extract_front());
  ASSERT_EQ(2, t2->data);
  AssertListEq(list, {3});

  std::unique_ptr<TestObject> t3(list.extract_front());
  ASSERT_EQ(3, t3->data);
  AssertListEq(list, {});
}

TEST_F(IntrusiveListTest, extract_back) {
  TestObjectList list = NewList({1, 2, 3});

  std::unique_ptr<TestObject> t1(list.extract_back());
  ASSERT_EQ(3, t1->data);
  AssertListEq(list, {1, 2});

  std::unique_ptr<TestObject> t2(list.extract_back());
  ASSERT_EQ(2, t2->data);
  AssertListEq(list, {1});

  std::unique_ptr<TestObject> t3(list.extract_back());
  ASSERT_EQ(1, t3->data);
  AssertListEq(list, {});
}

TEST_F(IntrusiveListTest, emplace) {
  TestObjectList list;

  // Pass an additional arg to show that forwarding works properly.
  list.emplace(list.begin(), 2, 200);
  list.emplace(list.end(), 4, 400);
  list.emplace(std::next(list.begin()), 3, 300);
  list.emplace(list.begin(), 1, 100);

  AssertListEq(list, {1, 2, 3, 4}, {100, 200, 300, 400});
}

TEST_F(IntrusiveListTest, insert_pointer) {
  TestObjectList list;

  list.insert(list.begin(), MakeUnique<TestObject>(2));
  list.insert(list.end(), MakeUnique<TestObject>(4));
  list.insert(std::next(list.begin()), MakeUnique<TestObject>(3));
  list.insert(list.begin(), MakeUnique<TestObject>(1));

  AssertListEq(list, {1, 2, 3, 4});
}

TEST_F(IntrusiveListTest, insert_move) {
  TestObjectList list;

  list.insert(list.begin(), TestObject(2));
  list.insert(list.end(), TestObject(4));
  list.insert(std::next(list.begin()), TestObject(3));
  list.insert(list.begin(), TestObject(1));

  AssertListEq(list, {1, 2, 3, 4});
}

TEST_F(IntrusiveListTest, extract) {
  TestObjectList list = NewList({1, 2, 3, 4});

  TestObjectList::iterator t1_iter = std::next(list.begin(), 0);
  TestObjectList::iterator t2_iter = std::next(list.begin(), 1);
  TestObjectList::iterator t3_iter = std::next(list.begin(), 2);
  TestObjectList::iterator t4_iter = std::next(list.begin(), 3);

  std::unique_ptr<TestObject> t2(list.extract(t2_iter));
  ASSERT_EQ(2, t2->data);
  AssertListEq(list, {1, 3, 4});

  std::unique_ptr<TestObject> t4(list.extract(t4_iter));
  ASSERT_EQ(4, t4->data);
  AssertListEq(list, {1, 3});

  std::unique_ptr<TestObject> t1(list.extract(t1_iter));
  ASSERT_EQ(1, t1->data);
  AssertListEq(list, {3});

  std::unique_ptr<TestObject> t3(list.extract(t3_iter));
  ASSERT_EQ(3, t3->data);
  AssertListEq(list, {});
}

TEST_F(IntrusiveListTest, erase) {
  TestObjectList list = NewList({1, 2, 3, 4});

  TestObjectList::iterator t1_iter = std::next(list.begin(), 0);
  TestObjectList::iterator t2_iter = std::next(list.begin(), 1);
  TestObjectList::iterator t3_iter = std::next(list.begin(), 2);
  TestObjectList::iterator t4_iter = std::next(list.begin(), 3);

  // erase returns an iterator to the following node.
  ASSERT_EQ(3, list.erase(t2_iter)->data);
  AssertListEq(list, {1, 3, 4});

  ASSERT_EQ(list.end(), list.erase(t4_iter));
  AssertListEq(list, {1, 3});

  ASSERT_EQ(3, list.erase(t1_iter)->data);
  AssertListEq(list, {3});

  ASSERT_EQ(list.end(), list.erase(t3_iter));
  AssertListEq(list, {});
}

TEST_F(IntrusiveListTest, erase_range) {
  TestObjectList list = NewList({1, 2, 3, 4, 5, 6});

  // OK to erase an empty range.
  list.erase(list.begin(), list.begin());
  list.erase(list.end(), list.end());

  // Erase the first element (1).
  list.erase(list.begin(), std::next(list.begin()));
  AssertListEq(list, {2, 3, 4, 5, 6});

  // Erase the last element (6).
  list.erase(std::prev(list.end()), list.end());
  AssertListEq(list, {2, 3, 4, 5});

  // Erase [3, 4] => [2, 5]
  list.erase(std::next(list.begin()), std::prev(list.end()));
  AssertListEq(list, {2, 5});

  // Erase the rest.
  list.erase(list.begin(), list.end());
  AssertListEq(list, {});
}

TEST_F(IntrusiveListTest, swap) {
  TestObjectList list1 = NewList({1, 2, 3, 4});

  TestObjectList list2 = NewList({100, 200, 300});

  AssertListEq(list1, {1, 2, 3, 4});
  AssertListEq(list2, {100, 200, 300});

  list1.swap(list2);

  AssertListEq(list1, {100, 200, 300});
  AssertListEq(list2, {1, 2, 3, 4});
}

TEST_F(IntrusiveListTest, clear) {
  TestObjectList list = NewList({1, 2, 3, 4});

  ASSERT_FALSE(list.empty());

  list.clear();

  ASSERT_EQ(0U, list.size());
  ASSERT_TRUE(list.empty());
}

TEST_F(IntrusiveListTest, splice_list) {
  TestObjectList list1 = NewList({1, 2, 3});

  // Splice at beginning.
  TestObjectList list2 = NewList({100, 200});
  list1.splice(list1.begin(), list2);
  AssertListEq(list1, {100, 200, 1, 2, 3});
  AssertListEq(list2, {});

  // Splice at end.
  TestObjectList list3 = NewList({500, 600, 700});
  list1.splice(list1.end(), list3);
  AssertListEq(list1, {100, 200, 1, 2, 3, 500, 600, 700});
  AssertListEq(list3, {});

  // Splice in the middle.
  TestObjectList list4 = NewList({400});
  list1.splice(std::next(list1.begin(), 4), list4);
  AssertListEq(list1, {100, 200, 1, 2, 400, 3, 500, 600, 700});
  AssertListEq(list4, {});
}

TEST_F(IntrusiveListTest, splice_list_move) {
  TestObjectList list1 = NewList({1, 2, 3});

  // Splice at beginning.
  list1.splice(list1.begin(), NewList({100, 200}));
  AssertListEq(list1, {100, 200, 1, 2, 3});

  // Splice at end.
  list1.splice(list1.end(), NewList({500, 600, 700}));
  AssertListEq(list1, {100, 200, 1, 2, 3, 500, 600, 700});

  // Splice in the middle.
  list1.splice(std::next(list1.begin(), 4), NewList({400}));
  AssertListEq(list1, {100, 200, 1, 2, 400, 3, 500, 600, 700});
}

TEST_F(IntrusiveListTest, splice_node) {
  TestObjectList list1 = NewList({1, 2, 3});

  // Splice at beginning.
  TestObjectList list2 = NewList({100, 200});
  list1.splice(list1.begin(), list2, list2.begin());
  AssertListEq(list1, {100, 1, 2, 3});
  AssertListEq(list2, {200});

  // Splice at end.
  TestObjectList list3 = NewList({500, 600, 700});
  list1.splice(list1.end(), list3, std::next(list3.begin(), 2));
  AssertListEq(list1, {100, 1, 2, 3, 700});
  AssertListEq(list3, {500, 600});

  // Splice in the middle.
  TestObjectList list4 = NewList({400});
  list1.splice(std::next(list1.begin(), 3), list4, list4.begin());
  AssertListEq(list1, {100, 1, 2, 400, 3, 700});
  AssertListEq(list4, {});
}

TEST_F(IntrusiveListTest, splice_range) {
  TestObjectList list1 = NewList({1, 2, 3});

  // Splice at beginning.
  TestObjectList list2 = NewList({100, 200, 300});
  list1.splice(list1.begin(), list2, list2.begin(), std::prev(list2.end()));
  AssertListEq(list1, {100, 200, 1, 2, 3});
  AssertListEq(list2, {300});

  // Splice at end.
  TestObjectList list3 = NewList({500, 600, 700});
  list1.splice(list1.end(), list3, std::next(list3.begin()), list3.end());
  AssertListEq(list1, {100, 200, 1, 2, 3, 600, 700});
  AssertListEq(list3, {500});

  // Splice in the middle.
  TestObjectList list4 = NewList({400});
  list1.splice(std::next(list1.begin(), 4), list4, list4.begin(), list4.end());
  AssertListEq(list1, {100, 200, 1, 2, 400, 3, 600, 700});
  AssertListEq(list4, {});
}

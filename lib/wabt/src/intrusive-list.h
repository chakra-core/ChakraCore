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

#ifndef WABT_INTRUSIVE_LIST_H_
#define WABT_INTRUSIVE_LIST_H_

#include <cassert>
#include <iterator>
#include <memory>

#include "src/make-unique.h"

// This uses a similar interface as std::list, but is missing the following
// features:
//
// * Add "extract_" functions that remove an element from the list and return
//   it.
// * Only supports move-only operations
// * No allocator support
// * No initializer lists
// * Asserts instead of exceptions
// * Some functions are not implemented (merge, remove, remove_if, reverse,
//   unique, sort, non-member comparison operators)

namespace wabt {

template <typename T>
class intrusive_list;

template <typename T>
class intrusive_list_base {
 private:
  friend class intrusive_list<T>;

  mutable T* next_ = nullptr;
  mutable T* prev_ = nullptr;
};

template <typename T>
class intrusive_list {
 public:
  // types:
  typedef T value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  class iterator;
  class const_iterator;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  // construct/copy/destroy:
  intrusive_list();
  explicit intrusive_list(std::unique_ptr<T> node);
  explicit intrusive_list(T&& node);
  intrusive_list(const intrusive_list&) = delete;
  intrusive_list(intrusive_list&&);
  ~intrusive_list();
  intrusive_list& operator=(const intrusive_list& other) = delete;
  intrusive_list& operator=(intrusive_list&& other);

  // iterators:
  iterator begin() noexcept;
  const_iterator begin() const noexcept;
  iterator end() noexcept;
  const_iterator end() const noexcept;

  reverse_iterator rbegin() noexcept;
  const_reverse_iterator rbegin() const noexcept;
  reverse_iterator rend() noexcept;
  const_reverse_iterator rend() const noexcept;

  const_iterator cbegin() const noexcept;
  const_iterator cend() const noexcept;
  const_reverse_iterator crbegin() const noexcept;
  const_reverse_iterator crend() const noexcept;

  // capacity:
  size_type size() const noexcept;
  bool empty() const noexcept;

  // element access:
  reference front();
  const_reference front() const;
  reference back();
  const_reference back() const;

  // modifiers:
  template <class... Args>
  void emplace_front(Args&&... args);
  template <class... Args>
  void emplace_back(Args&&... args);
  void push_front(std::unique_ptr<T> node);
  void push_front(T&& node);
  void push_back(std::unique_ptr<T> node);
  void push_back(T&& node);
  void pop_front();
  void pop_back();
  std::unique_ptr<T> extract_front();
  std::unique_ptr<T> extract_back();

  template <class... Args>
  iterator emplace(iterator pos, Args&&... args);
  iterator insert(iterator pos, std::unique_ptr<T> node);
  iterator insert(iterator pos, T&& node);
  std::unique_ptr<T> extract(iterator it);

  iterator erase(iterator pos);
  iterator erase(iterator first, iterator last);
  void swap(intrusive_list&);
  void clear() noexcept;

  void splice(iterator pos, intrusive_list& node);
  void splice(iterator pos, intrusive_list&& node);
  void splice(iterator pos, intrusive_list& node, iterator it);
  void splice(iterator pos,
              intrusive_list& node,
              iterator first,
              iterator last);

 private:
  T* first_ = nullptr;
  T* last_ = nullptr;
  size_t size_ = 0;
};

/// iterator
template <typename T>
class intrusive_list<T>::iterator {
 public:
  typedef std::ptrdiff_t difference_type;
  typedef std::bidirectional_iterator_tag iterator_category;
  typedef T value_type;
  typedef T* pointer;
  typedef T& reference;

  iterator(const intrusive_list<T>& list, T* node)
      : list_(&list), node_(node) {}

  reference operator*() const {
    assert(node_);
    return *node_;
  }

  pointer operator->() const {
    assert(node_);
    return node_;
  }

  iterator& operator++() {
    assert(node_);
    node_ = node_->next_;
    return *this;
  }

  iterator operator++(int) {
    iterator tmp = *this;
    operator++();
    return tmp;
  }

  iterator& operator--() {
    node_ = node_ ? node_->prev_ : list_->last_;
    return *this;
  }

  iterator operator--(int) {
    iterator tmp = *this;
    operator--();
    return tmp;
  }

  bool operator==(iterator rhs) const {
    assert(list_ == rhs.list_);
    return node_ == rhs.node_;
  }

  bool operator!=(iterator rhs) const {
    assert(list_ == rhs.list_);
    return node_ != rhs.node_;
  }

 private:
  friend class const_iterator;

  const intrusive_list<T>* list_;
  T* node_;
};

/// const_iterator
template <typename T>
class intrusive_list<T>::const_iterator {
 public:
  typedef std::ptrdiff_t difference_type;
  typedef std::bidirectional_iterator_tag iterator_category;
  typedef T value_type;
  typedef const T* pointer;
  typedef const T& reference;

  const_iterator(const intrusive_list<T>& list, T* node)
      : list_(&list), node_(node) {}

  const_iterator(const iterator& other)
      : list_(other.list_), node_(other.node_) {}

  reference operator*() const {
    assert(node_);
    return *node_;
  }

  pointer operator->() const {
    assert(node_);
    return node_;
  }

  const_iterator& operator++() {
    assert(node_);
    node_ = node_->next_;
    return *this;
  }

  const_iterator operator++(int) {
    const_iterator tmp = *this;
    operator++();
    return tmp;
  }

  const_iterator& operator--() {
    node_ = node_ ? node_->prev_ : list_->last_;
    return *this;
  }

  const_iterator operator--(int) {
    const_iterator tmp = *this;
    operator--();
    return tmp;
  }

  bool operator==(const_iterator rhs) const {
    assert(list_ == rhs.list_);
    return node_ == rhs.node_;
  }

  bool operator!=(const_iterator rhs) const {
    assert(list_ == rhs.list_);
    return node_ != rhs.node_;
  }

 private:
  const intrusive_list<T>* list_;
  T* node_;
};

template <typename T>
inline intrusive_list<T>::intrusive_list() {}

template <typename T>
inline intrusive_list<T>::intrusive_list(std::unique_ptr<T> node) {
  push_back(std::move(node));
}

template <typename T>
inline intrusive_list<T>::intrusive_list(T&& node) {
  push_back(std::move(node));
}

template <typename T>
inline intrusive_list<T>::intrusive_list(intrusive_list&& other)
    : first_(other.first_), last_(other.last_), size_(other.size_) {
  other.first_ = other.last_ = nullptr;
  other.size_ = 0;
}

template <typename T>
inline intrusive_list<T>::~intrusive_list() {
  clear();
}

template <typename T>
inline intrusive_list<T>& intrusive_list<T>::operator=(
    intrusive_list<T>&& other) {
  clear();
  first_ = other.first_;
  last_ = other.last_;
  size_ = other.size_;
  other.first_ = other.last_ = nullptr;
  other.size_ = 0;
  return *this;
}

template <typename T>
inline typename intrusive_list<T>::iterator
intrusive_list<T>::begin() noexcept {
  return iterator(*this, first_);
}

template <typename T>
inline typename intrusive_list<T>::const_iterator intrusive_list<T>::begin()
    const noexcept {
  return const_iterator(*this, first_);
}

template <typename T>
inline typename intrusive_list<T>::iterator intrusive_list<T>::end() noexcept {
  return iterator(*this, nullptr);
}

template <typename T>
inline typename intrusive_list<T>::const_iterator intrusive_list<T>::end() const
    noexcept {
  return const_iterator(*this, nullptr);
}

template <typename T>
inline typename intrusive_list<T>::reverse_iterator
intrusive_list<T>::rbegin() noexcept {
  return reverse_iterator(iterator(*this, nullptr));
}

template <typename T>
inline typename intrusive_list<T>::const_reverse_iterator
intrusive_list<T>::rbegin() const noexcept {
  return const_reverse_iterator(const_iterator(*this, nullptr));
}

template <typename T>
inline typename intrusive_list<T>::reverse_iterator
intrusive_list<T>::rend() noexcept {
  return reverse_iterator(iterator(*this, first_));
}

template <typename T>
inline typename intrusive_list<T>::const_reverse_iterator
intrusive_list<T>::rend() const noexcept {
  return const_reverse_iterator(const_iterator(*this, first_));
}

template <typename T>
inline typename intrusive_list<T>::const_iterator intrusive_list<T>::cbegin()
    const noexcept {
  return const_iterator(*this, first_);
}

template <typename T>
inline typename intrusive_list<T>::const_iterator intrusive_list<T>::cend()
    const noexcept {
  return const_iterator(*this, nullptr);
}

template <typename T>
inline typename intrusive_list<T>::const_reverse_iterator
intrusive_list<T>::crbegin() const noexcept {
  return const_reverse_iterator(const_iterator(*this, nullptr));
}

template <typename T>
inline typename intrusive_list<T>::const_reverse_iterator
intrusive_list<T>::crend() const noexcept {
  return const_reverse_iterator(const_iterator(*this, first_));
}

template <typename T>
inline typename intrusive_list<T>::size_type intrusive_list<T>::size() const
    noexcept {
  return size_;
}

template <typename T>
inline bool intrusive_list<T>::empty() const noexcept {
  return size_ == 0;
}

template <typename T>
inline typename intrusive_list<T>::reference intrusive_list<T>::front() {
  assert(!empty());
  return *first_;
}

template <typename T>
inline typename intrusive_list<T>::const_reference intrusive_list<T>::front()
    const {
  assert(!empty());
  return *first_;
}

template <typename T>
inline typename intrusive_list<T>::reference intrusive_list<T>::back() {
  assert(!empty());
  return *last_;
}

template <typename T>
inline typename intrusive_list<T>::const_reference intrusive_list<T>::back()
    const {
  assert(!empty());
  return *last_;
}

template <typename T>
template <class... Args>
inline void intrusive_list<T>::emplace_front(Args&&... args) {
  push_front(MakeUnique<T>(std::forward<Args>(args)...));
}

template <typename T>
template <class... Args>
inline void intrusive_list<T>::emplace_back(Args&&... args) {
  push_back(MakeUnique<T>(std::forward<Args>(args)...));
}

template <typename T>
inline void intrusive_list<T>::push_front(std::unique_ptr<T> node) {
  assert(node->prev_ == nullptr &&
         node->next_ == nullptr);

  T* node_p = node.release();
  if (first_) {
    node_p->next_ = first_;
    first_->prev_ = node_p;
  } else {
    last_ = node_p;
  }
  first_ = node_p;
  size_++;
}

template <typename T>
inline void intrusive_list<T>::push_front(T&& node) {
  push_front(MakeUnique<T>(std::move(node)));
}

template <typename T>
inline void intrusive_list<T>::push_back(std::unique_ptr<T> node) {
  assert(node->prev_ == nullptr &&
         node->next_ == nullptr);

  T* node_p = node.release();
  if (last_) {
    node_p->prev_ = last_;
    last_->next_ = node_p;
  } else {
    first_ = node_p;
  }
  last_ = node_p;
  size_++;
}

template <typename T>
inline void intrusive_list<T>::push_back(T&& node) {
  push_back(MakeUnique<T>(std::move(node)));
}

template <typename T>
inline void intrusive_list<T>::pop_front() {
  extract_front();
}

template <typename T>
inline void intrusive_list<T>::pop_back() {
  extract_back();
}

template <typename T>
inline std::unique_ptr<T> intrusive_list<T>::extract_front() {
  assert(!empty());
  T* node = first_;
  if (first_ == last_) {
    first_ = last_ = nullptr;
  } else {
    first_ = first_->next_;
    first_->prev_ = nullptr;
  }
  node->next_ = node->prev_ = nullptr;
  size_--;
  return std::unique_ptr<T>(node);
}

template <typename T>
inline std::unique_ptr<T> intrusive_list<T>::extract_back() {
  assert(!empty());
  T* node = last_;
  if (first_ == last_) {
    first_ = last_ = nullptr;
  } else {
    last_ = last_->prev_;
    last_->next_ = nullptr;
  }
  node->next_ = node->prev_ = nullptr;
  size_--;
  return std::unique_ptr<T>(node);
}

template <typename T>
template <class... Args>
inline typename intrusive_list<T>::iterator intrusive_list<T>::emplace(
    iterator pos,
    Args&&... args) {
  return insert(pos, MakeUnique<T>(std::forward<Args>(args)...));
}

template <typename T>
inline typename intrusive_list<T>::iterator intrusive_list<T>::insert(
    iterator pos,
    std::unique_ptr<T> node) {
  assert(node->prev_ == nullptr &&
         node->next_ == nullptr);

  T* node_p;
  if (pos == end()) {
    push_back(std::move(node));
    node_p = &back();
  } else {
    node_p = node.release();
    node_p->prev_ = pos->prev_;
    node_p->next_ = &*pos;
    if (pos->prev_) {
      pos->prev_->next_ = node_p;
    } else {
      first_ = node_p;
    }
    pos->prev_ = node_p;
    size_++;
  }
  return iterator(*this, node_p);
}

template <typename T>
inline typename intrusive_list<T>::iterator intrusive_list<T>::insert(
    iterator pos,
    T&& node) {
  return insert(pos, MakeUnique<T>(std::move(node)));
}

template <typename T>
inline std::unique_ptr<T> intrusive_list<T>::extract(iterator pos) {
  assert(!empty());
  assert(pos != end());
  T* node = &*pos;
  if (first_ == last_) {
    first_ = last_ = nullptr;
  } else {
    if (node->prev_) {
      node->prev_->next_ = node->next_;
    } else {
      first_ = node->next_;
    }

    if (node->next_) {
      node->next_->prev_ = node->prev_;
    } else {
      last_ = node->prev_;
    }
  }
  node->next_ = node->prev_ = nullptr;
  size_--;
  return std::unique_ptr<T>(node);
}

template <typename T>
inline typename intrusive_list<T>::iterator intrusive_list<T>::erase(
    iterator pos) {
  iterator next = std::next(pos);
  extract(pos);
  return next;
}

template <typename T>
inline typename intrusive_list<T>::iterator intrusive_list<T>::erase(
    iterator first,
    iterator last) {
  while (first != last)
    first = erase(first);
  return first;
}

template <typename T>
inline void intrusive_list<T>::swap(intrusive_list& other) {
  std::swap(first_, other.first_);
  std::swap(last_, other.last_);
  std::swap(size_, other.size_);
}

template <typename T>
inline void intrusive_list<T>::clear() noexcept {
  for (T* iter = first_; iter;) {
    T* next = iter->next_;
    delete iter;
    iter = next;
  }
  first_ = last_ = nullptr;
  size_ = 0;
}

template <typename T>
inline void intrusive_list<T>::splice(iterator pos, intrusive_list& other) {
  splice(pos, other, other.begin(), other.end());
}

template <typename T>
inline void intrusive_list<T>::splice(iterator pos, intrusive_list&& other) {
  splice(pos, other, other.begin(), other.end());
}

template <typename T>
inline void intrusive_list<T>::splice(iterator pos,
                                      intrusive_list& other,
                                      iterator it) {
  insert(pos, other.extract(it));
}

template <typename T>
inline void intrusive_list<T>::splice(iterator pos,
                                      intrusive_list& other,
                                      iterator first,
                                      iterator last) {
  while (first != last)
    insert(pos, other.extract(first++));
}

}  // namespace wabt

#endif // WABT_INTRUSIVE_LIST_H_

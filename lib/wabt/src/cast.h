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

#ifndef WABT_CAST_H_
#define WABT_CAST_H_

#include <memory>
#include <type_traits>

#include "src/common.h"

// Modeled after LLVM's dynamic casts:
// http://llvm.org/docs/ProgrammersManual.html#the-isa-cast-and-dyn-cast-templates
//
// Use isa<T>(foo) to check whether foo is a T*:
//
//     if (isa<Minivan>(car)) {
//        ...
//     }
//
// Use cast<T>(foo) when you know that foo is a T* -- it will assert that the
// type matches:
//
//     switch (car.type) {
//       case CarType::Minivan: {
//         auto minivan = cast<Minivan>(car);
//         ...
//       }
//     }
//
// Use dyn_cast<T>(foo) as a combination if isa and cast, it will return
// nullptr if the type doesn't match:
//
//     if (auto minivan = dyn_cast<Minivan>(car)) {
//       ...
//     }
//
//
// To use these classes in a type hierarchy, you must implement classof:
//
//     enum CarType { Minivan, ... };
//     struct Car { CarType type; ... };
//     struct Minivan : Car {
//       static bool classof(const Car* car) { return car->type == Minivan; }
//       ...
//     };
//

namespace wabt {

template <typename Derived, typename Base>
bool isa(const Base* base) {
  WABT_STATIC_ASSERT((std::is_base_of<Base, Derived>::value));
  return Derived::classof(base);
}

template <typename Derived, typename Base>
const Derived* cast(const Base* base) {
  assert(isa<Derived>(base));
  return static_cast<const Derived*>(base);
};

template <typename Derived, typename Base>
Derived* cast(Base* base) {
  assert(isa<Derived>(base));
  return static_cast<Derived*>(base);
};

template <typename Derived, typename Base>
const Derived* dyn_cast(const Base* base) {
  return isa<Derived>(base) ? static_cast<const Derived*>(base) : nullptr;
};

template <typename Derived, typename Base>
Derived* dyn_cast(Base* base) {
  return isa<Derived>(base) ? static_cast<Derived*>(base) : nullptr;
};

// Cast functionality for unique_ptr. isa and dyn_cast are not included because
// they won't always pass ownership back to the caller.

template <typename Derived, typename Base>
std::unique_ptr<const Derived> cast(std::unique_ptr<const Base>&& base) {
  assert(isa<Derived>(base.get()));
  return std::unique_ptr<Derived>(static_cast<const Derived*>(base.release()));
};

template <typename Derived, typename Base>
std::unique_ptr<Derived> cast(std::unique_ptr<Base>&& base) {
  assert(isa<Derived>(base.get()));
  return std::unique_ptr<Derived>(static_cast<Derived*>(base.release()));
};

} // namespace wabt

#endif // WABT_CAST_H_

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

#ifndef WABT_RESULT_H_
#define WABT_RESULT_H_

namespace wabt {

struct Result {
  enum Enum {
    Ok,
    Error,
  };

  Result() : Result(Ok) {}
  Result(Enum e) : enum_(e) {}
  operator Enum() const { return enum_; }
  Result& operator|=(Result rhs);

 private:
  Enum enum_;
};

inline Result operator|(Result lhs, Result rhs) {
  return (lhs == Result::Error || rhs == Result::Error) ? Result::Error
                                                        : Result::Ok;
}

inline Result& Result::operator|=(Result rhs) {
  enum_ = *this | rhs;
  return *this;
}

inline bool Succeeded(Result result) { return result == Result::Ok; }
inline bool Failed(Result result) { return result == Result::Error; }

#define CHECK_RESULT(expr)          \
  do {                              \
    if (Failed(expr)) {             \
      return ::wabt::Result::Error; \
    }                               \
  } while (0)

}  // namespace wabt

#endif  // WABT_RESULT_H_

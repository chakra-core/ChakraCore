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

#ifndef WABT_COLOR_H_
#define WABT_COLOR_H_

#include <cstdio>

namespace wabt {

#define WABT_FOREACH_COLOR_CODE(V) \
  V(Default, "\x1b[0m")            \
  V(Bold, "\x1b[1m")               \
  V(NoBold, "\x1b[22m")            \
  V(Black, "\x1b[30m")             \
  V(Red, "\x1b[31m")               \
  V(Green, "\x1b[32m")             \
  V(Yellow, "\x1b[33m")            \
  V(Blue, "\x1b[34m")              \
  V(Magenta, "\x1b[35m")           \
  V(Cyan, "\x1b[36m")              \
  V(White, "\x1b[37m")

class Color {
 public:
  Color(FILE*, bool enabled = true);

// Write the given color to the file, if enabled.
#define WABT_COLOR(Name, code) \
  void Name() const { WriteCode(Name##Code()); }
  WABT_FOREACH_COLOR_CODE(WABT_COLOR)
#undef WABT_COLOR

// Get the color code as a string, if enabled.
#define WABT_COLOR(Name, code) \
  const char* Maybe##Name##Code() const { return enabled_ ? Name##Code() : ""; }
  WABT_FOREACH_COLOR_CODE(WABT_COLOR)
#undef WABT_COLOR

// Get the color code as a string.
#define WABT_COLOR(Name, code) \
  static const char* Name##Code() { return code; }
  WABT_FOREACH_COLOR_CODE(WABT_COLOR)
#undef WABT_COLOR

 private:
  static bool SupportsColor(FILE*);
  void WriteCode(const char*) const;

  FILE* file_;
  bool enabled_;
};

#undef WABT_FOREACH_COLOR_CODE

}  // namespace wabt

#endif  //  WABT_COLOR_H_

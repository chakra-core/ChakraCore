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

#ifndef WABT_TRACING_H_
#define WABT_TRACING_H_

// Provides a simple tracing class that automatically generates enter/exit
// messages using the scope of the instance.
//
// It also assumes that this file is only included in .cc files.
// Immediately before the inclusion of this file, there is a define of
// for WABT_TRACING, defining whether tracing should be compiled in for
// that source file.

#ifndef WABT_TRACING
#define WABT_TRACING 0
#endif

#include "common.h"

namespace wabt {

#if WABT_TRACING

// Scoped class that automatically prints enter("->") and exit("<-")
// lines, indented by trace level.
struct TraceScope {
  WABT_DISALLOW_COPY_AND_ASSIGN(TraceScope);
  TraceScope() = delete;
  TraceScope(const char* method);
  template<typename... Args>
  TraceScope(const char* method, const char* format, Args&&... args)
      : method_(method) {
    PrintEnter(method);
    fprintf(stderr, format, std::forward<Args>(args)...);
    PrintNewline();
  }
  ~TraceScope();

 private:
  const char* method_;
  void PrintEnter(const char* method);
  void PrintNewline();
};

#define WABT_TRACE(method_name) TraceScope _func_(#method_name)

#define WABT_TRACE_ARGS(method_name, format, ...) \
  TraceScope _func_(#method_name, format, __VA_ARGS__)

#else

#define WABT_TRACE(method)
#define WABT_TRACE_ARGS(method_name, format, ...)

#endif

}  // end namespace wabt

#endif // WABT_TRACING_H_

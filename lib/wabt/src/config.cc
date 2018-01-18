/*
 * Copyright 2016 WebAssembly Community Group participants
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

#include "config.h"

#include <cstdarg>
#include <cstdio>

/* c99-style vsnprintf for MSVC < 2015. See http://stackoverflow.com/a/8712996
 using _snprintf or vsnprintf will not-properly null-terminate, and will return
 -1 instead of the number of characters needed on overflow. */
#if COMPILER_IS_MSVC
int wabt_vsnprintf(char* str, size_t size, const char* format, va_list ap) {
  int result = -1;
  if (size != 0) {
    result = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
  }
  if (result == -1) {
    result = _vscprintf(format, ap);
  }
  return result;
}

#if !HAVE_SNPRINTF
int wabt_snprintf(char* str, size_t size, const char* format, ...) {
  va_list args;
  va_start(args, format);
  int result = wabt_vsnprintf(str, size, format, args);
  va_end(args);
  return result;
}
#endif
#endif

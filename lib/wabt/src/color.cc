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

#include "src/color.h"

#include <cstdlib>

#include "src/common.h"

#if _WIN32
#include <io.h>
#include <windows.h>
#elif HAVE_UNISTD_H
#include <unistd.h>
#endif

namespace wabt {

Color::Color(FILE* file, bool enabled) : file_(file) {
  enabled_ = enabled && SupportsColor(file_);
}

// static
bool Color::SupportsColor(FILE* file) {
  char* force = getenv("FORCE_COLOR");
  if (force) {
    return atoi(force) != 0;
  }

#if _WIN32

  {
#if HAVE_WIN32_VT100
    HANDLE handle;
    if (file == stdout) {
      handle = GetStdHandle(STD_OUTPUT_HANDLE);
    } else if (file == stderr) {
      handle = GetStdHandle(STD_ERROR_HANDLE);
    } else {
      return false;
    }
    DWORD mode;
    if (!_isatty(_fileno(file)) || !GetConsoleMode(handle, &mode) ||
        !SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
      return false;
    }
    return true;
#else
    // TODO(binji): Support older Windows by using SetConsoleTextAttribute?
    return false;
#endif
  }

#elif HAVE_UNISTD_H

  return isatty(fileno(file));

#else

  return false;

#endif
}

void Color::WriteCode(const char* code) const {
  if (enabled_) {
    fputs(code, file_);
  }
}

}  // namespace wabt

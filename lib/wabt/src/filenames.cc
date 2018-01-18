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

#include "src/filenames.h"

namespace wabt {

const char* kWasmExtension = ".wasm";

const char* kWatExtension = ".wat";

string_view StripExtension(string_view filename) {
  return filename.substr(0, filename.find_last_of('.'));
}

string_view GetBasename(string_view filename) {
  size_t last_slash = filename.find_last_of('/');
  size_t last_backslash = filename.find_last_of('\\');
  if (last_slash == string_view::npos && last_backslash == string_view::npos) {
    return filename;
  }

  if (last_slash == string_view::npos) {
    if (last_backslash == string_view::npos) {
      return filename;
    }
    last_slash = last_backslash;
  } else if (last_backslash != string_view::npos) {
    last_slash = std::max(last_slash, last_backslash);
  }

  return filename.substr(last_slash + 1);
}

string_view GetExtension(string_view filename) {
  size_t pos = filename.find_last_of('.');
  if (pos == string_view::npos) {
    return "";
  }
  return filename.substr(pos);
}

}  // namespace wabt

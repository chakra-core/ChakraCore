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

#ifndef WABT_FILENAMES_H_
#define WABT_FILENAMES_H_

#include "src/common.h"

namespace wabt {

extern const char* kWasmExtension;
extern const char* kWatExtension;

// Return only the file extension, e.g.:
//
// "foo.txt", => ".txt"
// "foo" => ""
// "/foo/bar/foo.wasm" => ".wasm"
string_view GetExtension(string_view filename);

// Strip extension, e.g.:
//
// "foo", => "foo"
// "foo.bar" => "foo"
// "/path/to/foo.bar" => "/path/to/foo"
// "\\path\\to\\foo.bar" => "\\path\\to\\foo"
string_view StripExtension(string_view s);

// Strip everything up to and including the last slash, e.g.:
//
// "/foo/bar/baz", => "baz"
// "/usr/local/include/stdio.h", => "stdio.h"
// "foo.bar", => "foo.bar"
string_view GetBasename(string_view filename);

}  // namespace wabt

#endif /* WABT_FILENAMES_H_ */

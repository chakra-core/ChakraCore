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

#ifndef WABT_BINARY_READER_LINKER_H_
#define WABT_BINARY_READER_LINKER_H_

#include "common.h"
#include "stream.h"

namespace wabt {

struct Stream;

namespace link {

struct LinkerInputBinary;

struct LinkOptions {
  struct Stream* log_stream;
};

Result read_binary_linker(struct LinkerInputBinary* input_info,
                          struct LinkOptions* options);

} // namespace link
}  // namespace wabt

#endif /* WABT_BINARY_READER_LINKER_H_ */

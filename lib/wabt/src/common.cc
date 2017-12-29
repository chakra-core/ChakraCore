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

#include "src/common.h"

#include <cassert>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if COMPILER_IS_MSVC
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#define PATH_MAX _MAX_PATH
#endif

namespace wabt {

Reloc::Reloc(RelocType type, Offset offset, Index index, int32_t addend)
    : type(type), offset(offset), index(index), addend(addend) {}

const char* g_kind_name[] = {"func", "table", "memory", "global", "except"};
WABT_STATIC_ASSERT(WABT_ARRAY_SIZE(g_kind_name) == kExternalKindCount);

const char* g_reloc_type_name[] = {
    "R_WEBASSEMBLY_FUNCTION_INDEX_LEB", "R_WEBASSEMBLY_TABLE_INDEX_SLEB",
    "R_WEBASSEMBLY_TABLE_INDEX_I32",    "R_WEBASSEMBLY_MEMORY_ADDR_LEB",
    "R_WEBASSEMBLY_MEMORY_ADDR_SLEB",   "R_WEBASSEMBLY_MEMORY_ADDR_I32",
    "R_WEBASSEMBLY_TYPE_INDEX_LEB",     "R_WEBASSEMBLY_GLOBAL_INDEX_LEB",
};
WABT_STATIC_ASSERT(WABT_ARRAY_SIZE(g_reloc_type_name) == kRelocTypeCount);

Result ReadFile(string_view filename, std::vector<uint8_t>* out_data) {
  FILE* infile = fopen(filename.to_string().c_str(), "rb");
  if (!infile) {
    const char format[] = "unable to read file %s";
    char msg[PATH_MAX + sizeof(format)];
    wabt_snprintf(msg, sizeof(msg), format, filename.to_string().c_str());
    perror(msg);
    return Result::Error;
  }

  if (fseek(infile, 0, SEEK_END) < 0) {
    perror("fseek to end failed");
    fclose(infile);
    return Result::Error;
  }

  long size = ftell(infile);
  if (size < 0) {
    perror("ftell failed");
    fclose(infile);
    return Result::Error;
  }

  if (fseek(infile, 0, SEEK_SET) < 0) {
    perror("fseek to beginning failed");
    fclose(infile);
    return Result::Error;
  }

  out_data->resize(size);
  if (size != 0 && fread(out_data->data(), size, 1, infile) != 1) {
    perror("fread failed");
    fclose(infile);
    return Result::Error;
  }

  fclose(infile);
  return Result::Ok;
}

void InitStdio() {
#if COMPILER_IS_MSVC
  int result = _setmode(_fileno(stdout), _O_BINARY);
  if (result == -1) {
    perror("Cannot set mode binary to stdout");
  }
  result = _setmode(_fileno(stderr), _O_BINARY);
  if (result == -1) {
    perror("Cannot set mode binary to stderr");
  }
#endif
}

}  // namespace wabt

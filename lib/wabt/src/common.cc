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

#include "common.h"

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#if COMPILER_IS_MSVC
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#define PATH_MAX _MAX_PATH
#endif

namespace wabt {

OpcodeInfo g_opcode_info[kOpcodeCount];

/* TODO(binji): It's annoying to have to have an initializer function, but it
 * seems to be necessary as g++ doesn't allow non-trival designated
 * initializers (e.g. [314] = "blah") */
void init_opcode_info(void) {
  static bool s_initialized = false;
  if (!s_initialized) {
#define V(rtype, type1, type2, mem_size, code, NAME, text) \
  g_opcode_info[code].name = text;                         \
  g_opcode_info[code].result_type = Type::rtype;           \
  g_opcode_info[code].param1_type = Type::type1;           \
  g_opcode_info[code].param2_type = Type::type2;           \
  g_opcode_info[code].memory_size = mem_size;

    WABT_FOREACH_OPCODE(V)

#undef V
  }
}

const char* g_kind_name[] = {"func", "table", "memory", "global"};
WABT_STATIC_ASSERT(WABT_ARRAY_SIZE(g_kind_name) == kExternalKindCount);

const char* g_reloc_type_name[] = {"R_FUNC_INDEX_LEB", "R_TABLE_INDEX_SLEB",
                                   "R_TABLE_INDEX_I32", "R_GLOBAL_INDEX_LEB",
                                   "R_DATA"};
WABT_STATIC_ASSERT(WABT_ARRAY_SIZE(g_reloc_type_name) == kRelocTypeCount);

bool is_naturally_aligned(Opcode opcode, uint32_t alignment) {
  uint32_t opcode_align = get_opcode_memory_size(opcode);
  return alignment == WABT_USE_NATURAL_ALIGNMENT || alignment == opcode_align;
}

uint32_t get_opcode_alignment(Opcode opcode, uint32_t alignment) {
  if (alignment == WABT_USE_NATURAL_ALIGNMENT)
    return get_opcode_memory_size(opcode);
  return alignment;
}

StringSlice empty_string_slice(void) {
  StringSlice result;
  result.start = "";
  result.length = 0;
  return result;
}

bool string_slice_eq_cstr(const StringSlice* s1, const char* s2) {
  size_t s2_len = strlen(s2);
  if (s2_len != s1->length)
    return false;

  return strncmp(s1->start, s2, s2_len) == 0;
}

bool string_slice_startswith(const StringSlice* s1, const char* s2) {
  size_t s2_len = strlen(s2);
  if (s2_len > s1->length)
    return false;

  return strncmp(s1->start, s2, s2_len) == 0;
}

StringSlice string_slice_from_cstr(const char* string) {
  StringSlice result;
  result.start = string;
  result.length = strlen(string);
  return result;
}

bool string_slice_is_empty(const StringSlice* str) {
  assert(str);
  return !str->start || str->length == 0;
}

bool string_slices_are_equal(const StringSlice* a, const StringSlice* b) {
  assert(a && b);
  return a->start && b->start && a->length == b->length &&
         memcmp(a->start, b->start, a->length) == 0;
}

void destroy_string_slice(StringSlice* str) {
  assert(str);
  delete [] str->start;
}

Result read_file(const char* filename, char** out_data, size_t* out_size) {
  FILE* infile = fopen(filename, "rb");
  if (!infile) {
    const char* format = "unable to read file %s";
    char msg[PATH_MAX + sizeof(format)];
    wabt_snprintf(msg, sizeof(msg), format, filename);
    perror(msg);
    return Result::Error;
  }

  if (fseek(infile, 0, SEEK_END) < 0) {
    perror("fseek to end failed");
    return Result::Error;
  }

  long size = ftell(infile);
  if (size < 0) {
    perror("ftell failed");
    return Result::Error;
  }

  if (fseek(infile, 0, SEEK_SET) < 0) {
    perror("fseek to beginning failed");
    return Result::Error;
  }

  char* data = new char [size];
  if (size != 0 && fread(data, size, 1, infile) != 1) {
    perror("fread failed");
    return Result::Error;
  }

  *out_data = data;
  *out_size = size;
  fclose(infile);
  return Result::Ok;
}

static void print_carets(FILE* out,
                         size_t num_spaces,
                         size_t num_carets,
                         size_t max_line) {
  /* print the caret */
  char* carets = static_cast<char*>(alloca(max_line));
  memset(carets, '^', max_line);
  if (num_carets > max_line - num_spaces)
    num_carets = max_line - num_spaces;
  /* always print at least one caret */
  if (num_carets == 0)
    num_carets = 1;
  fprintf(out, "%*s%.*s\n", static_cast<int>(num_spaces), "",
          static_cast<int>(num_carets), carets);
}

static void print_source_error(FILE* out,
                               const Location* loc,
                               const char* error,
                               const char* source_line,
                               size_t source_line_length,
                               size_t source_line_column_offset) {
  fprintf(out, "%s:%d:%d: %s\n", loc->filename, loc->line, loc->first_column,
          error);
  if (source_line && source_line_length > 0) {
    fprintf(out, "%s\n", source_line);
    size_t num_spaces = (loc->first_column - 1) - source_line_column_offset;
    size_t num_carets = loc->last_column - loc->first_column;
    print_carets(out, num_spaces, num_carets, source_line_length);
  }
}

static void print_error_header(FILE* out, DefaultErrorHandlerInfo* info) {
  if (info && info->header) {
    switch (info->print_header) {
      case PrintErrorHeader::Never:
        break;

      case PrintErrorHeader::Once:
        info->print_header = PrintErrorHeader::Never;
      /* Fallthrough. */

      case PrintErrorHeader::Always:
        fprintf(out, "%s:\n", info->header);
        break;
    }
    /* If there's a header, indent the following message. */
    fprintf(out, "  ");
  }
}

static FILE* get_default_error_handler_info_output_file(
    DefaultErrorHandlerInfo* info) {
  return info && info->out_file ? info->out_file : stderr;
}

void default_source_error_callback(const Location* loc,
                                   const char* error,
                                   const char* source_line,
                                   size_t source_line_length,
                                   size_t source_line_column_offset,
                                   void* user_data) {
  DefaultErrorHandlerInfo* info =
      static_cast<DefaultErrorHandlerInfo*>(user_data);
  FILE* out = get_default_error_handler_info_output_file(info);
  print_error_header(out, info);
  print_source_error(out, loc, error, source_line, source_line_length,
                     source_line_column_offset);
}

void default_binary_error_callback(uint32_t offset,
                                   const char* error,
                                   void* user_data) {
  DefaultErrorHandlerInfo* info =
      static_cast<DefaultErrorHandlerInfo*>(user_data);
  FILE* out = get_default_error_handler_info_output_file(info);
  print_error_header(out, info);
  if (offset == WABT_UNKNOWN_OFFSET)
    fprintf(out, "error: %s\n", error);
  else
    fprintf(out, "error: @0x%08x: %s\n", offset, error);
  fflush(out);
}

void init_stdio() {
#if COMPILER_IS_MSVC
  int result = _setmode(_fileno(stdout), _O_BINARY);
  if (result == -1)
    perror("Cannot set mode binary to stdout");
  result = _setmode(_fileno(stderr), _O_BINARY);
  if (result == -1)
    perror("Cannot set mode binary to stderr");
#endif
}

}  // namespace wabt

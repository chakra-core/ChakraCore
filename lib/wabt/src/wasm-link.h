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

#ifndef WABT_LINK_H_
#define WABT_LINK_H_

#include "binary.h"
#include "common.h"
#include "vector.h"

#define WABT_LINK_MODULE_NAME "__extern"

namespace wabt {

struct LinkerInputBinary;

struct FunctionImport {
  StringSlice name;
  uint32_t sig_index;
  bool active; /* Is this import present in the linked binary */
  struct LinkerInputBinary* foreign_binary;
  uint32_t foreign_index;
};
WABT_DEFINE_VECTOR(function_import, FunctionImport);

struct GlobalImport {
  StringSlice name;
  Type type;
  bool mutable_;
};
WABT_DEFINE_VECTOR(global_import, GlobalImport);

struct DataSegment {
  uint32_t memory_index;
  uint32_t offset;
  const uint8_t* data;
  size_t size;
};
WABT_DEFINE_VECTOR(data_segment, DataSegment);

struct Reloc {
  RelocType type;
  size_t offset;
};
WABT_DEFINE_VECTOR(reloc, Reloc);

struct Export {
  ExternalKind kind;
  StringSlice name;
  uint32_t index;
};
WABT_DEFINE_VECTOR(export, Export);

struct SectionDataCustom {
  /* Reference to string data stored in the containing InputBinary */
  StringSlice name;
};

struct Section {
  /* The binary to which this section belongs */
  struct LinkerInputBinary* binary;
  RelocVector relocations; /* The relocations for this section */

  BinarySection section_code;
  size_t size;
  size_t offset;

  size_t payload_size;
  size_t payload_offset;

  /* For known sections, the count of the number of elements in the section */
  uint32_t count;

  union {
    /* CUSTOM section data */
    SectionDataCustom data_custom;
    /* DATA section data */
    DataSegmentVector data_segments;
    /* MEMORY section data */
    Limits memory_limits;
  };

  /* The offset at which this section appears within the combined output
   * section. */
  size_t output_payload_offset;
};
WABT_DEFINE_VECTOR(section, Section);

typedef Section* SectionPtr;
WABT_DEFINE_VECTOR(section_ptr, SectionPtr);

WABT_DEFINE_VECTOR(string_slice, StringSlice);

struct LinkerInputBinary {
  const char* filename;
  uint8_t* data;
  size_t size;
  SectionVector sections;

  ExportVector exports;

  FunctionImportVector function_imports;
  uint32_t active_function_imports;
  GlobalImportVector global_imports;
  uint32_t active_global_imports;

  uint32_t type_index_offset;
  uint32_t function_index_offset;
  uint32_t imported_function_index_offset;
  uint32_t global_index_offset;
  uint32_t imported_global_index_offset;
  uint32_t table_index_offset;
  uint32_t memory_page_count;
  uint32_t memory_page_offset;

  uint32_t table_elem_count;

  StringSliceVector debug_names;
};
WABT_DEFINE_VECTOR(binary, LinkerInputBinary);

}  // namespace wabt

#endif /* WABT_LINK_H_ */

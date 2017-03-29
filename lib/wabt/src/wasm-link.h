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

#include <memory>
#include <vector>

#include "binary.h"
#include "common.h"

#define WABT_LINK_MODULE_NAME "__extern"

namespace wabt {
namespace link {

struct LinkerInputBinary;

struct FunctionImport {
  StringSlice name;
  uint32_t sig_index;
  bool active; /* Is this import present in the linked binary */
  struct LinkerInputBinary* foreign_binary;
  uint32_t foreign_index;
};

struct GlobalImport {
  StringSlice name;
  Type type;
  bool mutable_;
};

struct DataSegment {
  uint32_t memory_index;
  uint32_t offset;
  const uint8_t* data;
  size_t size;
};

struct Export {
  ExternalKind kind;
  StringSlice name;
  uint32_t index;
};

struct SectionDataCustom {
  /* Reference to string data stored in the containing InputBinary */
  StringSlice name;
};

struct Section {
  WABT_DISALLOW_COPY_AND_ASSIGN(Section);
  Section();
  ~Section();

  /* The binary to which this section belongs */
  struct LinkerInputBinary* binary;
  std::vector<Reloc> relocations; /* The relocations for this section */

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
    std::vector<DataSegment>* data_segments;
    /* MEMORY section data */
    Limits memory_limits;
  };

  /* The offset at which this section appears within the combined output
   * section. */
  size_t output_payload_offset;
};

typedef std::vector<Section*> SectionPtrVector;

struct LinkerInputBinary {
  WABT_DISALLOW_COPY_AND_ASSIGN(LinkerInputBinary);
  LinkerInputBinary(const char* filename, uint8_t* data, size_t size);
  ~LinkerInputBinary();

  const char* filename;
  uint8_t* data;
  size_t size;
  std::vector<std::unique_ptr<Section>> sections;
  std::vector<Export> exports;

  std::vector<FunctionImport> function_imports;
  uint32_t active_function_imports;
  std::vector<GlobalImport> global_imports;
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

  std::vector<std::string> debug_names;
};

}  // namespace link
}  // namespace wabt

#endif /* WABT_LINK_H_ */

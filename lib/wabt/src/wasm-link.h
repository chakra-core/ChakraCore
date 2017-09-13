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

#include "src/binary.h"
#include "src/common.h"

namespace wabt {
namespace link {

class LinkerInputBinary;

struct FunctionImport {
  std::string module_name;
  std::string name;
  Index sig_index;
  bool active; /* Is this import present in the linked binary */
  Index relocated_function_index;
  LinkerInputBinary* foreign_binary;
  Index foreign_index;
};

struct GlobalImport {
  std::string module_name;
  std::string name;
  Type type;
  bool mutable_;
};

struct DataSegment {
  Index memory_index;
  Address offset;
  const uint8_t* data;
  size_t size;
};

struct Export {
  ExternalKind kind;
  std::string name;
  Index index;
};

struct Section {
  WABT_DISALLOW_COPY_AND_ASSIGN(Section);
  Section();
  ~Section();

  /* The binary to which this section belongs */
  LinkerInputBinary* binary;
  std::vector<Reloc> relocations; /* The relocations for this section */

  BinarySection section_code;
  size_t size;
  size_t offset;

  size_t payload_size;
  size_t payload_offset;

  /* For known sections, the count of the number of elements in the section */
  Index count;

  union {
    /* DATA section data */
    std::vector<DataSegment>* data_segments;
    /* MEMORY section data */
    uint64_t initial;
  } data;

  /* The offset at which this section appears within the combined output
   * section. */
  size_t output_payload_offset;
};

typedef std::vector<Section*> SectionPtrVector;

class LinkerInputBinary {
 public:
  WABT_DISALLOW_COPY_AND_ASSIGN(LinkerInputBinary);
  LinkerInputBinary(const char* filename, const std::vector<uint8_t>& data);

  Index RelocateFuncIndex(Index findex);
  Index RelocateTypeIndex(Index index);
  Index RelocateGlobalIndex(Index index);

  bool IsValidFunctionIndex(Index index);
  bool IsFunctionImport(Index index);
  bool IsInactiveFunctionImport(Index index);

  const char* filename;
  std::vector<uint8_t> data;
  std::vector<std::unique_ptr<Section>> sections;
  std::vector<Export> exports;

  std::vector<FunctionImport> function_imports;
  Index active_function_imports;
  std::vector<GlobalImport> global_imports;
  Index active_global_imports;

  Index type_index_offset;
  Index function_index_offset;
  Index imported_function_index_offset;
  Index global_index_offset;
  Index imported_global_index_offset;
  Index table_index_offset;
  Index memory_page_count;
  Index memory_page_offset;

  Index table_elem_count = 0;
  Index function_count = 0;

  std::vector<std::string> debug_names;
};

}  // namespace link
}  // namespace wabt

#endif /* WABT_LINK_H_ */

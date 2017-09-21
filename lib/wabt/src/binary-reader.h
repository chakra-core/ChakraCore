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

#ifndef WABT_BINARY_READER_H_
#define WABT_BINARY_READER_H_

#include <stddef.h>
#include <stdint.h>

#include "src/binary.h"
#include "src/common.h"
#include "src/feature.h"
#include "src/opcode.h"
#include "src/string-view.h"

namespace wabt {

class Stream;

struct ReadBinaryOptions {
  ReadBinaryOptions() = default;
  ReadBinaryOptions(const Features& features,
                    Stream* log_stream,
                    bool read_debug_names)
      : features(features),
        log_stream(log_stream),
        read_debug_names(read_debug_names) {}

  Features features;
  Stream* log_stream = nullptr;
  bool read_debug_names = false;
};

class BinaryReaderDelegate {
 public:
  struct State {
    State(const uint8_t* data, Offset size)
        : data(data), size(size), offset(0) {}

    const uint8_t* data;
    Offset size;
    Offset offset;
  };

  virtual ~BinaryReaderDelegate() {}

  virtual bool OnError(const char* message) = 0;
  virtual void OnSetState(const State* s) { state = s; }

  /* Module */
  virtual Result BeginModule(uint32_t version) = 0;
  virtual Result EndModule() = 0;

  virtual Result BeginSection(BinarySection section_type, Offset size) = 0;

  /* Custom section */
  virtual Result BeginCustomSection(Offset size, string_view section_name) = 0;
  virtual Result EndCustomSection() = 0;

  /* Type section */
  virtual Result BeginTypeSection(Offset size) = 0;
  virtual Result OnTypeCount(Index count) = 0;
  virtual Result OnType(Index index,
                        Index param_count,
                        Type* param_types,
                        Index result_count,
                        Type* result_types) = 0;
  virtual Result EndTypeSection() = 0;

  /* Import section */
  virtual Result BeginImportSection(Offset size) = 0;
  virtual Result OnImportCount(Index count) = 0;
  virtual Result OnImport(Index index,
                          string_view module_name,
                          string_view field_name) = 0;
  virtual Result OnImportFunc(Index import_index,
                              string_view module_name,
                              string_view field_name,
                              Index func_index,
                              Index sig_index) = 0;
  virtual Result OnImportTable(Index import_index,
                               string_view module_name,
                               string_view field_name,
                               Index table_index,
                               Type elem_type,
                               const Limits* elem_limits) = 0;
  virtual Result OnImportMemory(Index import_index,
                                string_view module_name,
                                string_view field_name,
                                Index memory_index,
                                const Limits* page_limits) = 0;
  virtual Result OnImportGlobal(Index import_index,
                                string_view module_name,
                                string_view field_name,
                                Index global_index,
                                Type type,
                                bool mutable_) = 0;
  virtual Result OnImportException(Index import_index,
                                   string_view module_name,
                                   string_view field_name,
                                   Index except_index,
                                   TypeVector& sig) = 0;
  virtual Result EndImportSection() = 0;

  /* Function section */
  virtual Result BeginFunctionSection(Offset size) = 0;
  virtual Result OnFunctionCount(Index count) = 0;
  virtual Result OnFunction(Index index, Index sig_index) = 0;
  virtual Result EndFunctionSection() = 0;

  /* Table section */
  virtual Result BeginTableSection(Offset size) = 0;
  virtual Result OnTableCount(Index count) = 0;
  virtual Result OnTable(Index index,
                         Type elem_type,
                         const Limits* elem_limits) = 0;
  virtual Result EndTableSection() = 0;

  /* Memory section */
  virtual Result BeginMemorySection(Offset size) = 0;
  virtual Result OnMemoryCount(Index count) = 0;
  virtual Result OnMemory(Index index, const Limits* limits) = 0;
  virtual Result EndMemorySection() = 0;

  /* Global section */
  virtual Result BeginGlobalSection(Offset size) = 0;
  virtual Result OnGlobalCount(Index count) = 0;
  virtual Result BeginGlobal(Index index, Type type, bool mutable_) = 0;
  virtual Result BeginGlobalInitExpr(Index index) = 0;
  virtual Result EndGlobalInitExpr(Index index) = 0;
  virtual Result EndGlobal(Index index) = 0;
  virtual Result EndGlobalSection() = 0;

  /* Exports section */
  virtual Result BeginExportSection(Offset size) = 0;
  virtual Result OnExportCount(Index count) = 0;
  virtual Result OnExport(Index index,
                          ExternalKind kind,
                          Index item_index,
                          string_view name) = 0;
  virtual Result EndExportSection() = 0;

  /* Start section */
  virtual Result BeginStartSection(Offset size) = 0;
  virtual Result OnStartFunction(Index func_index) = 0;
  virtual Result EndStartSection() = 0;

  /* Code section */
  virtual Result BeginCodeSection(Offset size) = 0;
  virtual Result OnFunctionBodyCount(Index count) = 0;
  virtual Result BeginFunctionBody(Index index) = 0;
  virtual Result OnLocalDeclCount(Index count) = 0;
  virtual Result OnLocalDecl(Index decl_index, Index count, Type type) = 0;

  /* Function expressions; called between BeginFunctionBody and
   EndFunctionBody */
  virtual Result OnOpcode(Opcode Opcode) = 0;
  virtual Result OnOpcodeBare() = 0;
  virtual Result OnOpcodeUint32(uint32_t value) = 0;
  virtual Result OnOpcodeIndex(Index value) = 0;
  virtual Result OnOpcodeUint32Uint32(uint32_t value, uint32_t value2) = 0;
  virtual Result OnOpcodeUint64(uint64_t value) = 0;
  virtual Result OnOpcodeF32(uint32_t value) = 0;
  virtual Result OnOpcodeF64(uint64_t value) = 0;
  virtual Result OnOpcodeBlockSig(Index num_types, Type* sig_types) = 0;
  virtual Result OnAtomicLoadExpr(Opcode opcode,
                                  uint32_t alignment_log2,
                                  Address offset) = 0;
  virtual Result OnAtomicStoreExpr(Opcode opcode,
                                   uint32_t alignment_log2,
                                   Address offset) = 0;
  virtual Result OnAtomicRmwExpr(Opcode opcode,
                                 uint32_t alignment_log2,
                                 Address offset) = 0;
  virtual Result OnAtomicRmwCmpxchgExpr(Opcode opcode,
                                        uint32_t alignment_log2,
                                        Address offset) = 0;
  virtual Result OnBinaryExpr(Opcode opcode) = 0;
  virtual Result OnBlockExpr(Index num_types, Type* sig_types) = 0;
  virtual Result OnBrExpr(Index depth) = 0;
  virtual Result OnBrIfExpr(Index depth) = 0;
  virtual Result OnBrTableExpr(Index num_targets,
                               Index* target_depths,
                               Index default_target_depth) = 0;
  virtual Result OnCallExpr(Index func_index) = 0;
  virtual Result OnCallIndirectExpr(Index sig_index) = 0;
  virtual Result OnCatchExpr(Index except_index) = 0;
  virtual Result OnCatchAllExpr() = 0;
  virtual Result OnCompareExpr(Opcode opcode) = 0;
  virtual Result OnConvertExpr(Opcode opcode) = 0;
  virtual Result OnCurrentMemoryExpr() = 0;
  virtual Result OnDropExpr() = 0;
  virtual Result OnElseExpr() = 0;
  virtual Result OnEndExpr() = 0;
  virtual Result OnEndFunc() = 0;
  virtual Result OnF32ConstExpr(uint32_t value_bits) = 0;
  virtual Result OnF64ConstExpr(uint64_t value_bits) = 0;
  virtual Result OnGetGlobalExpr(Index global_index) = 0;
  virtual Result OnGetLocalExpr(Index local_index) = 0;
  virtual Result OnGrowMemoryExpr() = 0;
  virtual Result OnI32ConstExpr(uint32_t value) = 0;
  virtual Result OnI64ConstExpr(uint64_t value) = 0;
  virtual Result OnIfExpr(Index num_types, Type* sig_types) = 0;
  virtual Result OnLoadExpr(Opcode opcode,
                            uint32_t alignment_log2,
                            Address offset) = 0;
  virtual Result OnLoopExpr(Index num_types, Type* sig_types) = 0;
  virtual Result OnNopExpr() = 0;
  virtual Result OnRethrowExpr(Index depth) = 0;
  virtual Result OnReturnExpr() = 0;
  virtual Result OnSelectExpr() = 0;
  virtual Result OnSetGlobalExpr(Index global_index) = 0;
  virtual Result OnSetLocalExpr(Index local_index) = 0;
  virtual Result OnStoreExpr(Opcode opcode,
                             uint32_t alignment_log2,
                             Address offset) = 0;
  virtual Result OnTeeLocalExpr(Index local_index) = 0;
  virtual Result OnThrowExpr(Index except_index) = 0;
  virtual Result OnTryExpr(Index num_types, Type* sig_types) = 0;

  virtual Result OnUnaryExpr(Opcode opcode) = 0;
  virtual Result OnUnreachableExpr() = 0;
  virtual Result EndFunctionBody(Index index) = 0;
  virtual Result EndCodeSection() = 0;

  /* Elem section */
  virtual Result BeginElemSection(Offset size) = 0;
  virtual Result OnElemSegmentCount(Index count) = 0;
  virtual Result BeginElemSegment(Index index, Index table_index) = 0;
  virtual Result BeginElemSegmentInitExpr(Index index) = 0;
  virtual Result EndElemSegmentInitExpr(Index index) = 0;
  virtual Result OnElemSegmentFunctionIndexCount(Index index, Index count) = 0;
  virtual Result OnElemSegmentFunctionIndex(Index segment_index,
                                            Index func_index) = 0;
  virtual Result EndElemSegment(Index index) = 0;
  virtual Result EndElemSection() = 0;

  /* Data section */
  virtual Result BeginDataSection(Offset size) = 0;
  virtual Result OnDataSegmentCount(Index count) = 0;
  virtual Result BeginDataSegment(Index index, Index memory_index) = 0;
  virtual Result BeginDataSegmentInitExpr(Index index) = 0;
  virtual Result EndDataSegmentInitExpr(Index index) = 0;
  virtual Result OnDataSegmentData(Index index,
                                   const void* data,
                                   Address size) = 0;
  virtual Result EndDataSegment(Index index) = 0;
  virtual Result EndDataSection() = 0;

  /* Names section */
  virtual Result BeginNamesSection(Offset size) = 0;
  virtual Result OnFunctionNameSubsection(Index index,
                                          uint32_t name_type,
                                          Offset subsection_size) = 0;
  virtual Result OnFunctionNamesCount(Index num_functions) = 0;
  virtual Result OnFunctionName(Index function_index,
                                string_view function_name) = 0;
  virtual Result OnLocalNameSubsection(Index index,
                                       uint32_t name_type,
                                       Offset subsection_size) = 0;
  virtual Result OnLocalNameFunctionCount(Index num_functions) = 0;
  virtual Result OnLocalNameLocalCount(Index function_index,
                                       Index num_locals) = 0;
  virtual Result OnLocalName(Index function_index,
                             Index local_index,
                             string_view local_name) = 0;
  virtual Result EndNamesSection() = 0;

  /* Reloc section */
  virtual Result BeginRelocSection(Offset size) = 0;
  virtual Result OnRelocCount(Index count,
                              BinarySection section_code,
                              string_view section_name) = 0;
  virtual Result OnReloc(RelocType type,
                         Offset offset,
                         Index index,
                         uint32_t addend) = 0;
  virtual Result EndRelocSection() = 0;

  /* Linking section */
  virtual Result BeginLinkingSection(Offset size) = 0;
  virtual Result OnStackGlobal(Index stack_global) = 0;
  virtual Result OnSymbolInfoCount(Index count) = 0;
  virtual Result OnSymbolInfo(string_view name, uint32_t flags) = 0;
  virtual Result OnDataSize(uint32_t data_size) = 0;
  virtual Result OnDataAlignment(uint32_t data_alignment) = 0;
  virtual Result EndLinkingSection() = 0;

  /* Exception section */
  virtual Result BeginExceptionSection(Offset size) = 0;
  virtual Result OnExceptionCount(Index count) = 0;
  virtual Result OnExceptionType(Index index, TypeVector& sig) = 0;
  virtual Result EndExceptionSection() = 0;

  /* InitExpr - used by elem, data and global sections; these functions are
   * only called between calls to Begin*InitExpr and End*InitExpr */
  virtual Result OnInitExprF32ConstExpr(Index index, uint32_t value) = 0;
  virtual Result OnInitExprF64ConstExpr(Index index, uint64_t value) = 0;
  virtual Result OnInitExprGetGlobalExpr(Index index, Index global_index) = 0;
  virtual Result OnInitExprI32ConstExpr(Index index, uint32_t value) = 0;
  virtual Result OnInitExprI64ConstExpr(Index index, uint64_t value) = 0;

  const State* state = nullptr;
};

Result ReadBinary(const void* data,
                  size_t size,
                  BinaryReaderDelegate* reader,
                  const ReadBinaryOptions* options);

size_t ReadU32Leb128(const uint8_t* ptr,
                     const uint8_t* end,
                     uint32_t* out_value);

size_t ReadI32Leb128(const uint8_t* ptr,
                     const uint8_t* end,
                     uint32_t* out_value);

}  // namespace wabt

#endif /* WABT_BINARY_READER_H_ */

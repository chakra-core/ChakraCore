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

#ifndef WABT_BINARY_READER_NOP_H_
#define WABT_BINARY_READER_NOP_H_

#include "src/binary-reader.h"

namespace wabt {

class BinaryReaderNop : public BinaryReaderDelegate {
 public:
  bool OnError(const char* message) override { return false; }

  /* Module */
  Result BeginModule(uint32_t version) override { return Result::Ok; }
  Result EndModule() override { return Result::Ok; }

  Result BeginSection(BinarySection section_type, Offset size) override {
    return Result::Ok;
  }

  /* Custom section */
  Result BeginCustomSection(Offset size, string_view section_name) override {
    return Result::Ok;
  }
  Result EndCustomSection() override { return Result::Ok; }

  /* Type section */
  Result BeginTypeSection(Offset size) override { return Result::Ok; }
  Result OnTypeCount(Index count) override { return Result::Ok; }
  Result OnType(Index index,
                Index param_count,
                Type* param_types,
                Index result_count,
                Type* result_types) override {
    return Result::Ok;
  }
  Result EndTypeSection() override { return Result::Ok; }

  /* Import section */
  Result BeginImportSection(Offset size) override { return Result::Ok; }
  Result OnImportCount(Index count) override { return Result::Ok; }
  Result OnImport(Index index,
                  string_view module_name,
                  string_view field_name) override {
    return Result::Ok;
  }
  Result OnImportFunc(Index import_index,
                      string_view module_name,
                      string_view field_name,
                      Index func_index,
                      Index sig_index) override {
    return Result::Ok;
  }
  Result OnImportTable(Index import_index,
                       string_view module_name,
                       string_view field_name,
                       Index table_index,
                       Type elem_type,
                       const Limits* elem_limits) override {
    return Result::Ok;
  }
  Result OnImportMemory(Index import_index,
                        string_view module_name,
                        string_view field_name,
                        Index memory_index,
                        const Limits* page_limits) override {
    return Result::Ok;
  }
  Result OnImportGlobal(Index import_index,
                        string_view module_name,
                        string_view field_name,
                        Index global_index,
                        Type type,
                        bool mutable_) override {
    return Result::Ok;
  }
  Result OnImportException(Index import_index,
                           string_view module_name,
                           string_view field_name,
                           Index except_index,
                           TypeVector& sig) override {
    return Result::Ok;
  }
  Result EndImportSection() override { return Result::Ok; }

  /* Function section */
  Result BeginFunctionSection(Offset size) override { return Result::Ok; }
  Result OnFunctionCount(Index count) override { return Result::Ok; }
  Result OnFunction(Index index, Index sig_index) override {
    return Result::Ok;
  }
  Result EndFunctionSection() override { return Result::Ok; }

  /* Table section */
  Result BeginTableSection(Offset size) override { return Result::Ok; }
  Result OnTableCount(Index count) override { return Result::Ok; }
  Result OnTable(Index index,
                 Type elem_type,
                 const Limits* elem_limits) override {
    return Result::Ok;
  }
  Result EndTableSection() override { return Result::Ok; }

  /* Memory section */
  Result BeginMemorySection(Offset size) override { return Result::Ok; }
  Result OnMemoryCount(Index count) override { return Result::Ok; }
  Result OnMemory(Index index, const Limits* limits) override {
    return Result::Ok;
  }
  Result EndMemorySection() override { return Result::Ok; }

  /* Global section */
  Result BeginGlobalSection(Offset size) override { return Result::Ok; }
  Result OnGlobalCount(Index count) override { return Result::Ok; }
  Result BeginGlobal(Index index, Type type, bool mutable_) override {
    return Result::Ok;
  }
  Result BeginGlobalInitExpr(Index index) override { return Result::Ok; }
  Result EndGlobalInitExpr(Index index) override { return Result::Ok; }
  Result EndGlobal(Index index) override { return Result::Ok; }
  Result EndGlobalSection() override { return Result::Ok; }

  /* Exports section */
  Result BeginExportSection(Offset size) override { return Result::Ok; }
  Result OnExportCount(Index count) override { return Result::Ok; }
  Result OnExport(Index index,
                  ExternalKind kind,
                  Index item_index,
                  string_view name) override {
    return Result::Ok;
  }
  Result EndExportSection() override { return Result::Ok; }

  /* Start section */
  Result BeginStartSection(Offset size) override { return Result::Ok; }
  Result OnStartFunction(Index func_index) override { return Result::Ok; }
  Result EndStartSection() override { return Result::Ok; }

  /* Code section */
  Result BeginCodeSection(Offset size) override { return Result::Ok; }
  Result OnFunctionBodyCount(Index count) override { return Result::Ok; }
  Result BeginFunctionBody(Index index) override { return Result::Ok; }
  Result OnLocalDeclCount(Index count) override { return Result::Ok; }
  Result OnLocalDecl(Index decl_index, Index count, Type type) override {
    return Result::Ok;
  }

  /* Function expressions; called between BeginFunctionBody and
   EndFunctionBody */
  Result OnOpcode(Opcode Opcode) override { return Result::Ok; }
  Result OnOpcodeBare() override { return Result::Ok; }
  Result OnOpcodeIndex(Index value) override { return Result::Ok; }
  Result OnOpcodeUint32(uint32_t value) override { return Result::Ok; }
  Result OnOpcodeUint32Uint32(uint32_t value, uint32_t value2) override {
    return Result::Ok;
  }
  Result OnOpcodeUint64(uint64_t value) override { return Result::Ok; }
  Result OnOpcodeF32(uint32_t value) override { return Result::Ok; }
  Result OnOpcodeF64(uint64_t value) override { return Result::Ok; }
  Result OnOpcodeBlockSig(Index num_types, Type* sig_types) override {
    return Result::Ok;
  }
  Result OnBinaryExpr(Opcode opcode) override { return Result::Ok; }
  Result OnBlockExpr(Index num_types, Type* sig_types) override {
    return Result::Ok;
  }
  Result OnBrExpr(Index depth) override { return Result::Ok; }
  Result OnBrIfExpr(Index depth) override { return Result::Ok; }
  Result OnBrTableExpr(Index num_targets,
                       Index* target_depths,
                       Index default_target_depth) override {
    return Result::Ok;
  }
  Result OnCallExpr(Index func_index) override { return Result::Ok; }
  Result OnCallIndirectExpr(Index sig_index) override { return Result::Ok; }
  Result OnCatchExpr(Index except_index) override { return Result::Ok; }
  Result OnCatchAllExpr() override { return Result::Ok; }
  Result OnCompareExpr(Opcode opcode) override { return Result::Ok; }
  Result OnConvertExpr(Opcode opcode) override { return Result::Ok; }
  Result OnCurrentMemoryExpr() override { return Result::Ok; }
  Result OnDropExpr() override { return Result::Ok; }
  Result OnElseExpr() override { return Result::Ok; }
  Result OnEndExpr() override { return Result::Ok; }
  Result OnEndFunc() override { return Result::Ok; }
  Result OnF32ConstExpr(uint32_t value_bits) override { return Result::Ok; }
  Result OnF64ConstExpr(uint64_t value_bits) override { return Result::Ok; }
  Result OnGetGlobalExpr(Index global_index) override { return Result::Ok; }
  Result OnGetLocalExpr(Index local_index) override { return Result::Ok; }
  Result OnGrowMemoryExpr() override { return Result::Ok; }
  Result OnI32ConstExpr(uint32_t value) override { return Result::Ok; }
  Result OnI64ConstExpr(uint64_t value) override { return Result::Ok; }
  Result OnIfExpr(Index num_types, Type* sig_types) override {
    return Result::Ok;
  }
  Result OnLoadExpr(Opcode opcode,
                    uint32_t alignment_log2,
                    Address offset) override {
    return Result::Ok;
  }
  Result OnLoopExpr(Index num_types, Type* sig_types) override {
    return Result::Ok;
  }
  Result OnNopExpr() override { return Result::Ok; }
  Result OnRethrowExpr(Index depth) override { return Result::Ok; }
  Result OnReturnExpr() override { return Result::Ok; }
  Result OnSelectExpr() override { return Result::Ok; }
  Result OnSetGlobalExpr(Index global_index) override { return Result::Ok; }
  Result OnSetLocalExpr(Index local_index) override { return Result::Ok; }
  Result OnStoreExpr(Opcode opcode,
                     uint32_t alignment_log2,
                     Address offset) override {
    return Result::Ok;
  }
  Result OnTeeLocalExpr(Index local_index) override { return Result::Ok; }
  Result OnThrowExpr(Index depth) override { return Result::Ok; }
  Result OnTryExpr(Index num_types, Type* sig_types) override {
    return Result::Ok;
  }
  Result OnUnaryExpr(Opcode opcode) override { return Result::Ok; }
  Result OnUnreachableExpr() override { return Result::Ok; }
  Result EndFunctionBody(Index index) override { return Result::Ok; }
  Result EndCodeSection() override { return Result::Ok; }

  /* Elem section */
  Result BeginElemSection(Offset size) override { return Result::Ok; }
  Result OnElemSegmentCount(Index count) override { return Result::Ok; }
  Result BeginElemSegment(Index index, Index table_index) override {
    return Result::Ok;
  }
  Result BeginElemSegmentInitExpr(Index index) override { return Result::Ok; }
  Result EndElemSegmentInitExpr(Index index) override { return Result::Ok; }
  Result OnElemSegmentFunctionIndexCount(Index index, Index count) override {
    return Result::Ok;
  }
  Result OnElemSegmentFunctionIndex(Index index, Index func_index) override {
    return Result::Ok;
  }
  Result EndElemSegment(Index index) override { return Result::Ok; }
  Result EndElemSection() override { return Result::Ok; }

  /* Data section */
  Result BeginDataSection(Offset size) override { return Result::Ok; }
  Result OnDataSegmentCount(Index count) override { return Result::Ok; }
  Result BeginDataSegment(Index index, Index memory_index) override {
    return Result::Ok;
  }
  Result BeginDataSegmentInitExpr(Index index) override { return Result::Ok; }
  Result EndDataSegmentInitExpr(Index index) override { return Result::Ok; }
  Result OnDataSegmentData(Index index,
                           const void* data,
                           Address size) override {
    return Result::Ok;
  }
  Result EndDataSegment(Index index) override { return Result::Ok; }
  Result EndDataSection() override { return Result::Ok; }

  /* Names section */
  Result BeginNamesSection(Offset size) override { return Result::Ok; }
  Result OnFunctionNameSubsection(Index index,
                                  uint32_t name_type,
                                  Offset subsection_size) override {
    return Result::Ok;
  }
  Result OnFunctionNamesCount(Index num_functions) override {
    return Result::Ok;
  }
  Result OnFunctionName(Index function_index,
                        string_view function_name) override {
    return Result::Ok;
  }
  Result OnLocalNameSubsection(Index index,
                               uint32_t name_type,
                               Offset subsection_size) override {
    return Result::Ok;
  }
  Result OnLocalNameFunctionCount(Index num_functions) override {
    return Result::Ok;
  }
  Result OnLocalNameLocalCount(Index function_index,
                               Index num_locals) override {
    return Result::Ok;
  }
  Result OnLocalName(Index function_index,
                     Index local_index,
                     string_view local_name) override {
    return Result::Ok;
  }
  Result EndNamesSection() override { return Result::Ok; }

  /* Reloc section */
  Result BeginRelocSection(Offset size) override { return Result::Ok; }
  Result OnRelocCount(Index count,
                      BinarySection section_code,
                      string_view section_name) override {
    return Result::Ok;
  }
  Result OnReloc(RelocType type,
                 Offset offset,
                 Index index,
                 uint32_t addend) override {
    return Result::Ok;
  }
  Result EndRelocSection() override { return Result::Ok; }

  /* Exception section */
  Result BeginExceptionSection(Offset size) override { return Result::Ok; }
  Result OnExceptionCount(Index count) override { return Result::Ok; }
  Result OnExceptionType(Index index, TypeVector& sig) override {
    return Result::Ok;
  }
  Result EndExceptionSection() override { return Result::Ok; }

  /* Linking section */
  Result BeginLinkingSection(Offset size) override { return Result::Ok; }
  Result OnStackGlobal(Index stack_global) override { return Result::Ok; }
  Result OnSymbolInfoCount(Index count) override { return Result::Ok; }
  Result OnSymbolInfo(string_view name, uint32_t flags) override {
    return Result::Ok;
  }
  Result OnDataSize(uint32_t data_size) override { return Result::Ok; }
  Result OnDataAlignment(uint32_t data_alignment) override {
    return Result::Ok;
  }
  Result EndLinkingSection() override { return Result::Ok; }

  /* InitExpr - used by elem, data and global sections; these functions are
   * only called between calls to Begin*InitExpr and End*InitExpr */
  Result OnInitExprF32ConstExpr(Index index, uint32_t value) override {
    return Result::Ok;
  }
  Result OnInitExprF64ConstExpr(Index index, uint64_t value) override {
    return Result::Ok;
  }
  Result OnInitExprGetGlobalExpr(Index index, Index global_index) override {
    return Result::Ok;
  }
  Result OnInitExprI32ConstExpr(Index index, uint32_t value) override {
    return Result::Ok;
  }
  Result OnInitExprI64ConstExpr(Index index, uint64_t value) override {
    return Result::Ok;
  }
};

}  // namespace wabt

#endif /* WABT_BINARY_READER_H_ */

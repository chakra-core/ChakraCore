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

#include "binary-reader-ir.h"

#include <cassert>
#include <cinttypes>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "binary-error-handler.h"
#include "binary-reader-nop.h"
#include "common.h"
#include "ir.h"

#define CHECK_RESULT(expr) \
  do {                     \
    if (WABT_FAILED(expr)) \
      return Result::Error;   \
  } while (0)

namespace wabt {

namespace {

struct LabelNode {
  LabelNode(LabelType, Expr** first);

  LabelType label_type;
  Expr** first;
  Expr* last;
};

LabelNode::LabelNode(LabelType label_type, Expr** first)
    : label_type(label_type), first(first), last(nullptr) {}

class BinaryReaderIR : public BinaryReaderNop {
 public:
  BinaryReaderIR(Module* out_module, BinaryErrorHandler* error_handler);

  bool OnError(const char* message) override;

  Result OnTypeCount(Index count) override;
  Result OnType(Index index,
                Index param_count,
                Type* param_types,
                Index result_count,
                Type* result_types) override;

  Result OnImportCount(Index count) override;
  Result OnImport(Index index,
                  StringSlice module_name,
                  StringSlice field_name) override;
  Result OnImportFunc(Index import_index,
                      StringSlice module_name,
                      StringSlice field_name,
                      Index func_index,
                      Index sig_index) override;
  Result OnImportTable(Index import_index,
                       StringSlice module_name,
                       StringSlice field_name,
                       Index table_index,
                       Type elem_type,
                       const Limits* elem_limits) override;
  Result OnImportMemory(Index import_index,
                        StringSlice module_name,
                        StringSlice field_name,
                        Index memory_index,
                        const Limits* page_limits) override;
  Result OnImportGlobal(Index import_index,
                        StringSlice module_name,
                        StringSlice field_name,
                        Index global_index,
                        Type type,
                        bool mutable_) override;

  Result OnFunctionCount(Index count) override;
  Result OnFunction(Index index, Index sig_index) override;

  Result OnTableCount(Index count) override;
  Result OnTable(Index index,
                 Type elem_type,
                 const Limits* elem_limits) override;

  Result OnMemoryCount(Index count) override;
  Result OnMemory(Index index, const Limits* limits) override;

  Result OnGlobalCount(Index count) override;
  Result BeginGlobal(Index index, Type type, bool mutable_) override;
  Result BeginGlobalInitExpr(Index index) override;
  Result EndGlobalInitExpr(Index index) override;

  Result OnExportCount(Index count) override;
  Result OnExport(Index index,
                  ExternalKind kind,
                  Index item_index,
                  StringSlice name) override;

  Result OnStartFunction(Index func_index) override;

  Result OnFunctionBodyCount(Index count) override;
  Result BeginFunctionBody(Index index) override;
  Result OnLocalDecl(Index decl_index, Index count, Type type) override;

  Result OnBinaryExpr(Opcode opcode) override;
  Result OnBlockExpr(Index num_types, Type* sig_types) override;
  Result OnBrExpr(Index depth) override;
  Result OnBrIfExpr(Index depth) override;
  Result OnBrTableExpr(Index num_targets,
                       Index* target_depths,
                       Index default_target_depth) override;
  Result OnCallExpr(Index func_index) override;
  Result OnCallIndirectExpr(Index sig_index) override;
  Result OnCompareExpr(Opcode opcode) override;
  Result OnConvertExpr(Opcode opcode) override;
  Result OnDropExpr() override;
  Result OnElseExpr() override;
  Result OnEndExpr() override;
  Result OnF32ConstExpr(uint32_t value_bits) override;
  Result OnF64ConstExpr(uint64_t value_bits) override;
  Result OnGetGlobalExpr(Index global_index) override;
  Result OnGetLocalExpr(Index local_index) override;
  Result OnGrowMemoryExpr() override;
  Result OnI32ConstExpr(uint32_t value) override;
  Result OnI64ConstExpr(uint64_t value) override;
  Result OnIfExpr(Index num_types, Type* sig_types) override;
  Result OnLoadExpr(Opcode opcode,
                    uint32_t alignment_log2,
                    Address offset) override;
  Result OnLoopExpr(Index num_types, Type* sig_types) override;
  Result OnCurrentMemoryExpr() override;
  Result OnNopExpr() override;
  Result OnReturnExpr() override;
  Result OnSelectExpr() override;
  Result OnSetGlobalExpr(Index global_index) override;
  Result OnSetLocalExpr(Index local_index) override;
  Result OnStoreExpr(Opcode opcode,
                     uint32_t alignment_log2,
                     Address offset) override;
  Result OnTeeLocalExpr(Index local_index) override;
  Result OnUnaryExpr(Opcode opcode) override;
  Result OnUnreachableExpr() override;
  Result EndFunctionBody(Index index) override;

  Result OnElemSegmentCount(Index count) override;
  Result BeginElemSegment(Index index, Index table_index) override;
  Result BeginElemSegmentInitExpr(Index index) override;
  Result EndElemSegmentInitExpr(Index index) override;
  Result OnElemSegmentFunctionIndexCount(Index index,
                                         Index count) override;
  Result OnElemSegmentFunctionIndex(Index index,
                                    Index func_index) override;

  Result OnDataSegmentCount(Index count) override;
  Result BeginDataSegment(Index index, Index memory_index) override;
  Result BeginDataSegmentInitExpr(Index index) override;
  Result EndDataSegmentInitExpr(Index index) override;
  Result OnDataSegmentData(Index index,
                           const void* data,
                           Address size) override;

  Result OnFunctionNamesCount(Index num_functions) override;
  Result OnFunctionName(Index function_index,
                        StringSlice function_name) override;
  Result OnLocalNameLocalCount(Index function_index,
                               Index num_locals) override;
  Result OnLocalName(Index function_index,
                     Index local_index,
                     StringSlice local_name) override;

  Result OnInitExprF32ConstExpr(Index index, uint32_t value) override;
  Result OnInitExprF64ConstExpr(Index index, uint64_t value) override;
  Result OnInitExprGetGlobalExpr(Index index, Index global_index) override;
  Result OnInitExprI32ConstExpr(Index index, uint32_t value) override;
  Result OnInitExprI64ConstExpr(Index index, uint64_t value) override;

 private:
  bool HandleError(Offset offset, const char* message);
  void PrintError(const char* format, ...);
  void PushLabel(LabelType label_type, Expr** first);
  Result PopLabel();
  Result GetLabelAt(LabelNode** label, Index depth);
  Result TopLabel(LabelNode** label);
  Result AppendExpr(Expr* expr);

  BinaryErrorHandler* error_handler = nullptr;
  Module* module = nullptr;

  Func* current_func = nullptr;
  std::vector<LabelNode> label_stack;
  Expr** current_init_expr = nullptr;
};

BinaryReaderIR::BinaryReaderIR(Module* out_module,
                                 BinaryErrorHandler* error_handler)
    : error_handler(error_handler), module(out_module) {}

void WABT_PRINTF_FORMAT(2, 3) BinaryReaderIR::PrintError(const char* format,
                                                          ...) {
  WABT_SNPRINTF_ALLOCA(buffer, length, format);
  HandleError(kInvalidOffset, buffer);
}

void BinaryReaderIR::PushLabel(LabelType label_type, Expr** first) {
  label_stack.emplace_back(label_type, first);
}

Result BinaryReaderIR::PopLabel() {
  if (label_stack.size() == 0) {
    PrintError("popping empty label stack");
    return Result::Error;
  }

  label_stack.pop_back();
  return Result::Ok;
}

Result BinaryReaderIR::GetLabelAt(LabelNode** label, Index depth) {
  if (depth >= label_stack.size()) {
    PrintError("accessing stack depth: %" PRIindex " >= max: %" PRIzd, depth,
               label_stack.size());
    return Result::Error;
  }

  *label = &label_stack[label_stack.size() - depth - 1];
  return Result::Ok;
}

Result BinaryReaderIR::TopLabel(LabelNode** label) {
  return GetLabelAt(label, 0);
}

Result BinaryReaderIR::AppendExpr(Expr* expr) {
  LabelNode* label;
  if (WABT_FAILED(TopLabel(&label))) {
    delete expr;
    return Result::Error;
  }
  if (*label->first) {
    label->last->next = expr;
    label->last = expr;
  } else {
    *label->first = label->last = expr;
  }
  return Result::Ok;
}

bool BinaryReaderIR::HandleError(Offset offset, const char* message) {
  return error_handler->OnError(offset, message);
}

bool BinaryReaderIR::OnError(const char* message) {
  return HandleError(state->offset, message);
}

Result BinaryReaderIR::OnTypeCount(Index count) {
  module->func_types.reserve(count);
  return Result::Ok;
}

Result BinaryReaderIR::OnType(Index index,
                               Index param_count,
                               Type* param_types,
                               Index result_count,
                               Type* result_types) {
  ModuleField* field = module->AppendField();
  field->type = ModuleFieldType::FuncType;
  field->func_type = new FuncType();

  FuncType* func_type = field->func_type;
  func_type->sig.param_types.assign(param_types, param_types + param_count);
  func_type->sig.result_types.assign(result_types, result_types + result_count);
  module->func_types.push_back(func_type);
  return Result::Ok;
}

Result BinaryReaderIR::OnImportCount(Index count) {
  module->imports.reserve(count);
  return Result::Ok;
}

Result BinaryReaderIR::OnImport(Index index,
                                 StringSlice module_name,
                                 StringSlice field_name) {
  ModuleField* field = module->AppendField();
  field->type = ModuleFieldType::Import;
  field->import = new Import();

  Import* import = field->import;
  import->module_name = dup_string_slice(module_name);
  import->field_name = dup_string_slice(field_name);
  module->imports.push_back(import);
  return Result::Ok;
}

Result BinaryReaderIR::OnImportFunc(Index import_index,
                                    StringSlice module_name,
                                    StringSlice field_name,
                                    Index func_index,
                                    Index sig_index) {
  assert(import_index == module->imports.size() - 1);
  Import* import = module->imports[import_index];

  import->kind = ExternalKind::Func;
  import->func = new Func();
  import->func->decl.has_func_type = true;
  import->func->decl.type_var.type = VarType::Index;
  import->func->decl.type_var.index = sig_index;
  import->func->decl.sig = module->func_types[sig_index]->sig;

  module->funcs.push_back(import->func);
  module->num_func_imports++;
  return Result::Ok;
}

Result BinaryReaderIR::OnImportTable(Index import_index,
                                     StringSlice module_name,
                                     StringSlice field_name,
                                     Index table_index,
                                     Type elem_type,
                                     const Limits* elem_limits) {
  assert(import_index == module->imports.size() - 1);
  Import* import = module->imports[import_index];
  import->kind = ExternalKind::Table;
  import->table = new Table();
  import->table->elem_limits = *elem_limits;
  module->tables.push_back(import->table);
  module->num_table_imports++;
  return Result::Ok;
}

Result BinaryReaderIR::OnImportMemory(Index import_index,
                                      StringSlice module_name,
                                      StringSlice field_name,
                                      Index memory_index,
                                      const Limits* page_limits) {
  assert(import_index == module->imports.size() - 1);
  Import* import = module->imports[import_index];
  import->kind = ExternalKind::Memory;
  import->memory = new Memory();
  import->memory->page_limits = *page_limits;
  module->memories.push_back(import->memory);
  module->num_memory_imports++;
  return Result::Ok;
}

Result BinaryReaderIR::OnImportGlobal(Index import_index,
                                      StringSlice module_name,
                                      StringSlice field_name,
                                      Index global_index,
                                      Type type,
                                      bool mutable_) {
  assert(import_index == module->imports.size() - 1);
  Import* import = module->imports[import_index];
  import->kind = ExternalKind::Global;
  import->global = new Global();
  import->global->type = type;
  import->global->mutable_ = mutable_;
  module->globals.push_back(import->global);
  module->num_global_imports++;
  return Result::Ok;
}

Result BinaryReaderIR::OnFunctionCount(Index count) {
  module->funcs.reserve(module->num_func_imports + count);
  return Result::Ok;
}

Result BinaryReaderIR::OnFunction(Index index, Index sig_index) {
  ModuleField* field = module->AppendField();
  field->type = ModuleFieldType::Func;
  field->func = new Func();

  Func* func = field->func;
  func->decl.has_func_type = true;
  func->decl.type_var.type = VarType::Index;
  func->decl.type_var.index = sig_index;
  func->decl.sig = module->func_types[sig_index]->sig;

  module->funcs.push_back(func);
  return Result::Ok;
}

Result BinaryReaderIR::OnTableCount(Index count) {
  module->tables.reserve(module->num_table_imports + count);
  return Result::Ok;
}

Result BinaryReaderIR::OnTable(Index index,
                               Type elem_type,
                               const Limits* elem_limits) {
  ModuleField* field = module->AppendField();
  field->type = ModuleFieldType::Table;
  field->table = new Table();
  field->table->elem_limits = *elem_limits;
  module->tables.push_back(field->table);
  return Result::Ok;
}

Result BinaryReaderIR::OnMemoryCount(Index count) {
  module->memories.reserve(module->num_memory_imports + count);
  return Result::Ok;
}

Result BinaryReaderIR::OnMemory(Index index, const Limits* page_limits) {
  ModuleField* field = module->AppendField();
  field->type = ModuleFieldType::Memory;
  field->memory = new Memory();
  field->memory->page_limits = *page_limits;
  module->memories.push_back(field->memory);
  return Result::Ok;
}

Result BinaryReaderIR::OnGlobalCount(Index count) {
  module->globals.reserve(module->num_global_imports + count);
  return Result::Ok;
}

Result BinaryReaderIR::BeginGlobal(Index index, Type type, bool mutable_) {
  ModuleField* field = module->AppendField();
  field->type = ModuleFieldType::Global;
  field->global = new Global();
  field->global->type = type;
  field->global->mutable_ = mutable_;
  module->globals.push_back(field->global);
  return Result::Ok;
}

Result BinaryReaderIR::BeginGlobalInitExpr(Index index) {
  assert(index == module->globals.size() - 1);
  Global* global = module->globals[index];
  current_init_expr = &global->init_expr;
  return Result::Ok;
}

Result BinaryReaderIR::EndGlobalInitExpr(Index index) {
  current_init_expr = nullptr;
  return Result::Ok;
}

Result BinaryReaderIR::OnExportCount(Index count) {
  module->exports.reserve(count);
  return Result::Ok;
}

Result BinaryReaderIR::OnExport(Index index,
                                ExternalKind kind,
                                Index item_index,
                                StringSlice name) {
  ModuleField* field = module->AppendField();
  field->type = ModuleFieldType::Export;
  field->export_ = new Export();

  Export* export_ = field->export_;
  export_->name = dup_string_slice(name);
  switch (kind) {
    case ExternalKind::Func:
      assert(item_index < module->funcs.size());
      break;
    case ExternalKind::Table:
      assert(item_index < module->tables.size());
      break;
    case ExternalKind::Memory:
      assert(item_index < module->memories.size());
      break;
    case ExternalKind::Global:
      assert(item_index < module->globals.size());
      break;
    case ExternalKind::Except:
      WABT_FATAL("OnExport(except) not implemented\n");
      break;
  }
  export_->var.type = VarType::Index;
  export_->var.index = item_index;
  export_->kind = kind;
  module->exports.push_back(export_);
  return Result::Ok;
}

Result BinaryReaderIR::OnStartFunction(Index func_index) {
  ModuleField* field = module->AppendField();
  field->type = ModuleFieldType::Start;

  field->start.type = VarType::Index;
  assert(func_index < module->funcs.size());
  field->start.index = func_index;

  module->start = &field->start;
  return Result::Ok;
}

Result BinaryReaderIR::OnFunctionBodyCount(Index count) {
  assert(module->num_func_imports + count == module->funcs.size());
  return Result::Ok;
}

Result BinaryReaderIR::BeginFunctionBody(Index index) {
  current_func = module->funcs[index];
  PushLabel(LabelType::Func, &current_func->first_expr);
  return Result::Ok;
}

Result BinaryReaderIR::OnLocalDecl(Index decl_index, Index count, Type type) {
  TypeVector& types = current_func->local_types;
  types.reserve(types.size() + count);
  for (size_t i = 0; i < count; ++i)
    types.push_back(type);
  return Result::Ok;
}

Result BinaryReaderIR::OnBinaryExpr(Opcode opcode) {
  Expr* expr = Expr::CreateBinary(opcode);
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnBlockExpr(Index num_types, Type* sig_types) {
  Expr* expr = Expr::CreateBlock(new Block());
  expr->block->sig.assign(sig_types, sig_types + num_types);
  AppendExpr(expr);
  PushLabel(LabelType::Block, &expr->block->first);
  return Result::Ok;
}

Result BinaryReaderIR::OnBrExpr(Index depth) {
  Expr* expr = Expr::CreateBr(Var(depth));
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnBrIfExpr(Index depth) {
  Expr* expr = Expr::CreateBrIf(Var(depth));
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnBrTableExpr(Index num_targets,
                                     Index* target_depths,
                                     Index default_target_depth) {
  VarVector* targets = new VarVector();
  targets->resize(num_targets);
  for (Index i = 0; i < num_targets; ++i) {
    (*targets)[i] = Var(target_depths[i]);
  }
  Expr* expr = Expr::CreateBrTable(targets, Var(default_target_depth));
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnCallExpr(Index func_index) {
  assert(func_index < module->funcs.size());
  Expr* expr = Expr::CreateCall(Var(func_index));
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnCallIndirectExpr(Index sig_index) {
  assert(sig_index < module->func_types.size());
  Expr* expr = Expr::CreateCallIndirect(Var(sig_index));
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnCompareExpr(Opcode opcode) {
  Expr* expr = Expr::CreateCompare(opcode);
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnConvertExpr(Opcode opcode) {
  Expr* expr = Expr::CreateConvert(opcode);
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnCurrentMemoryExpr() {
  Expr* expr = Expr::CreateCurrentMemory();
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnDropExpr() {
  Expr* expr = Expr::CreateDrop();
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnElseExpr() {
  LabelNode* label;
  CHECK_RESULT(TopLabel(&label));
  if (label->label_type != LabelType::If) {
    PrintError("else expression without matching if");
    return Result::Error;
  }

  LabelNode* parent_label;
  CHECK_RESULT(GetLabelAt(&parent_label, 1));
  assert(parent_label->last->type == ExprType::If);

  label->label_type = LabelType::Else;
  label->first = &parent_label->last->if_.false_;
  label->last = nullptr;
  return Result::Ok;
}

Result BinaryReaderIR::OnEndExpr() {
  return PopLabel();
}

Result BinaryReaderIR::OnF32ConstExpr(uint32_t value_bits) {
  Expr* expr = Expr::CreateConst(Const(Const::F32(), value_bits));
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnF64ConstExpr(uint64_t value_bits) {
  Expr* expr = Expr::CreateConst(Const(Const::F64(), value_bits));
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnGetGlobalExpr(Index global_index) {
  Expr* expr = Expr::CreateGetGlobal(Var(global_index));
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnGetLocalExpr(Index local_index) {
  Expr* expr = Expr::CreateGetLocal(Var(local_index));
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnGrowMemoryExpr() {
  Expr* expr = Expr::CreateGrowMemory();
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnI32ConstExpr(uint32_t value) {
  Expr* expr = Expr::CreateConst(Const(Const::I32(), value));
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnI64ConstExpr(uint64_t value) {
  Expr* expr = Expr::CreateConst(Const(Const::I64(), value));
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnIfExpr(Index num_types, Type* sig_types) {
  Expr* expr = Expr::CreateIf(new Block());
  expr->if_.true_->sig.assign(sig_types, sig_types + num_types);
  expr->if_.false_ = nullptr;
  AppendExpr(expr);
  PushLabel(LabelType::If, &expr->if_.true_->first);
  return Result::Ok;
}

Result BinaryReaderIR::OnLoadExpr(Opcode opcode,
                                  uint32_t alignment_log2,
                                  Address offset) {
  Expr* expr = Expr::CreateLoad(opcode, 1 << alignment_log2, offset);
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnLoopExpr(Index num_types, Type* sig_types) {
  Expr* expr = Expr::CreateLoop(new Block());
  expr->loop->sig.assign(sig_types, sig_types + num_types);
  AppendExpr(expr);
  PushLabel(LabelType::Loop, &expr->loop->first);
  return Result::Ok;
}

Result BinaryReaderIR::OnNopExpr() {
  Expr* expr = Expr::CreateNop();
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnReturnExpr() {
  Expr* expr = Expr::CreateReturn();
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnSelectExpr() {
  Expr* expr = Expr::CreateSelect();
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnSetGlobalExpr(Index global_index) {
  Expr* expr = Expr::CreateSetGlobal(Var(global_index));
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnSetLocalExpr(Index local_index) {
  Expr* expr = Expr::CreateSetLocal(Var(local_index));
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnStoreExpr(Opcode opcode,
                                   uint32_t alignment_log2,
                                   Address offset) {
  Expr* expr = Expr::CreateStore(opcode, 1 << alignment_log2, offset);
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnTeeLocalExpr(Index local_index) {
  Expr* expr = Expr::CreateTeeLocal(Var(local_index));
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnUnaryExpr(Opcode opcode) {
  Expr* expr = Expr::CreateUnary(opcode);
  return AppendExpr(expr);
}

Result BinaryReaderIR::OnUnreachableExpr() {
  Expr* expr = Expr::CreateUnreachable();
  return AppendExpr(expr);
}

Result BinaryReaderIR::EndFunctionBody(Index index) {
  CHECK_RESULT(PopLabel());
  current_func = nullptr;
  return Result::Ok;
}

Result BinaryReaderIR::OnElemSegmentCount(Index count) {
  module->elem_segments.reserve(count);
  return Result::Ok;
}

Result BinaryReaderIR::BeginElemSegment(Index index, Index table_index) {
  ModuleField* field = module->AppendField();
  field->type = ModuleFieldType::ElemSegment;
  field->elem_segment = new ElemSegment();
  field->elem_segment->table_var.type = VarType::Index;
  field->elem_segment->table_var.index = table_index;
  module->elem_segments.push_back(field->elem_segment);
  return Result::Ok;
}

Result BinaryReaderIR::BeginElemSegmentInitExpr(Index index) {
  assert(index == module->elem_segments.size() - 1);
  ElemSegment* segment = module->elem_segments[index];
  current_init_expr = &segment->offset;
  return Result::Ok;
}

Result BinaryReaderIR::EndElemSegmentInitExpr(Index index) {
  current_init_expr = nullptr;
  return Result::Ok;
}

Result BinaryReaderIR::OnElemSegmentFunctionIndexCount(Index index,
                                                       Index count) {
  assert(index == module->elem_segments.size() - 1);
  ElemSegment* segment = module->elem_segments[index];
  segment->vars.reserve(count);
  return Result::Ok;
}

Result BinaryReaderIR::OnElemSegmentFunctionIndex(Index segment_index,
                                                  Index func_index) {
  assert(segment_index == module->elem_segments.size() - 1);
  ElemSegment* segment = module->elem_segments[segment_index];
  segment->vars.emplace_back();
  Var* var = &segment->vars.back();
  var->type = VarType::Index;
  var->index = func_index;
  return Result::Ok;
}

Result BinaryReaderIR::OnDataSegmentCount(Index count) {
  module->data_segments.reserve(count);
  return Result::Ok;
}

Result BinaryReaderIR::BeginDataSegment(Index index, Index memory_index) {
  ModuleField* field = module->AppendField();
  field->type = ModuleFieldType::DataSegment;
  field->data_segment = new DataSegment();
  field->data_segment->memory_var.type = VarType::Index;
  field->data_segment->memory_var.index = memory_index;
  module->data_segments.push_back(field->data_segment);
  return Result::Ok;
}

Result BinaryReaderIR::BeginDataSegmentInitExpr(Index index) {
  assert(index == module->data_segments.size() - 1);
  DataSegment* segment = module->data_segments[index];
  current_init_expr = &segment->offset;
  return Result::Ok;
}

Result BinaryReaderIR::EndDataSegmentInitExpr(Index index) {
  current_init_expr = nullptr;
  return Result::Ok;
}

Result BinaryReaderIR::OnDataSegmentData(Index index,
                                         const void* data,
                                         Address size) {
  assert(index == module->data_segments.size() - 1);
  DataSegment* segment = module->data_segments[index];
  segment->data = new char[size];
  segment->size = size;
  memcpy(segment->data, data, size);
  return Result::Ok;
}

Result BinaryReaderIR::OnFunctionNamesCount(Index count) {
  if (count > module->funcs.size()) {
    PrintError("expected function name count (%" PRIindex
               ") <= function count (%" PRIzd ")",
               count, module->funcs.size());
    return Result::Error;
  }
  return Result::Ok;
}

Result BinaryReaderIR::OnFunctionName(Index index, StringSlice name) {
  if (string_slice_is_empty(&name))
    return Result::Ok;

  Func* func = module->funcs[index];
  std::string dollar_name = std::string("$") + string_slice_to_string(name);
  func->name = dup_string_slice(string_to_string_slice(dollar_name));
  module->func_bindings.emplace(dollar_name, Binding(index));
  return Result::Ok;
}

Result BinaryReaderIR::OnLocalNameLocalCount(Index index, Index count) {
  assert(index < module->funcs.size());
  Func* func = module->funcs[index];
  Index num_params_and_locals = func->GetNumParamsAndLocals();
  if (count > num_params_and_locals) {
    PrintError("expected local name count (%" PRIindex
               ") <= local count (%" PRIindex ")",
               count, num_params_and_locals);
    return Result::Error;
  }
  return Result::Ok;
}

Result BinaryReaderIR::OnInitExprF32ConstExpr(Index index, uint32_t value) {
  *current_init_expr = Expr::CreateConst(Const(Const::F32(), value));
  return Result::Ok;
}

Result BinaryReaderIR::OnInitExprF64ConstExpr(Index index, uint64_t value) {
  *current_init_expr = Expr::CreateConst(Const(Const::F64(), value));
  return Result::Ok;
}

Result BinaryReaderIR::OnInitExprGetGlobalExpr(Index index,
                                               Index global_index) {
  *current_init_expr = Expr::CreateGetGlobal(Var(global_index));
  return Result::Ok;
}

Result BinaryReaderIR::OnInitExprI32ConstExpr(Index index, uint32_t value) {
  *current_init_expr = Expr::CreateConst(Const(Const::I32(), value));
  return Result::Ok;
}

Result BinaryReaderIR::OnInitExprI64ConstExpr(Index index, uint64_t value) {
  *current_init_expr = Expr::CreateConst(Const(Const::I64(), value));
  return Result::Ok;
}

Result BinaryReaderIR::OnLocalName(Index func_index,
                                   Index local_index,
                                   StringSlice name) {
  if (string_slice_is_empty(&name))
    return Result::Ok;

  Func* func = module->funcs[func_index];
  Index num_params = func->GetNumParams();
  BindingHash* bindings;
  Index index;
  if (local_index < num_params) {
    /* param name */
    bindings = &func->param_bindings;
    index = local_index;
  } else {
    /* local name */
    bindings = &func->local_bindings;
    index = local_index - num_params;
  }
  bindings->emplace(std::string("$") + string_slice_to_string(name),
                    Binding(index));
  return Result::Ok;
}

}  // namespace

Result read_binary_ir(const void* data,
                      size_t size,
                      const ReadBinaryOptions* options,
                      BinaryErrorHandler* error_handler,
                      struct Module* out_module) {
  BinaryReaderIR reader(out_module, error_handler);
  Result result = read_binary(data, size, &reader, options);
  return result;
}

}  // namespace wabt

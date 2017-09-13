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

#include "src/type-checker.h"

namespace wabt {

TypeChecker::Label::Label(LabelType label_type,
                          const TypeVector& sig,
                          size_t limit)
    : label_type(label_type),
      sig(sig),
      type_stack_limit(limit),
      unreachable(false) {}

TypeChecker::TypeChecker(const ErrorCallback& error_callback)
    : error_callback_(error_callback) {}

void TypeChecker::PrintError(const char* fmt, ...) {
  if (error_callback_) {
    WABT_SNPRINTF_ALLOCA(buffer, length, fmt);
    error_callback_(buffer);
  }
}

Result TypeChecker::GetLabel(Index depth, Label** out_label) {
  if (depth >= label_stack_.size()) {
    assert(label_stack_.size() > 0);
    PrintError("invalid depth: %" PRIindex " (max %" PRIzd ")", depth,
               label_stack_.size() - 1);
    *out_label = nullptr;
    return Result::Error;
  }
  *out_label = &label_stack_[label_stack_.size() - depth - 1];
  return Result::Ok;
}

Result TypeChecker::TopLabel(Label** out_label) {
  return GetLabel(0, out_label);
}

bool TypeChecker::IsUnreachable() {
  Label* label;
  if (Failed(TopLabel(&label)))
    return true;
  return label->unreachable;
}

void TypeChecker::ResetTypeStackToLabel(Label* label) {
  type_stack_.resize(label->type_stack_limit);
}

Result TypeChecker::SetUnreachable() {
  Label* label;
  CHECK_RESULT(TopLabel(&label));
  label->unreachable = true;
  ResetTypeStackToLabel(label);
  return Result::Ok;
}

void TypeChecker::PushLabel(LabelType label_type, const TypeVector& sig) {
  label_stack_.emplace_back(label_type, sig, type_stack_.size());
}

Result TypeChecker::PopLabel() {
  label_stack_.pop_back();
  return Result::Ok;
}

Result TypeChecker::CheckLabelType(Label* label, LabelType label_type) {
  return label->label_type == label_type ? Result::Ok : Result::Error;
}

Result TypeChecker::PeekType(Index depth, Type* out_type) {
  Label* label;
  CHECK_RESULT(TopLabel(&label));

  if (label->type_stack_limit + depth >= type_stack_.size()) {
    *out_type = Type::Any;
    return label->unreachable ? Result::Ok : Result::Error;
  }
  *out_type = type_stack_[type_stack_.size() - depth - 1];
  return Result::Ok;
}

Result TypeChecker::TopType(Type* out_type) {
  return PeekType(0, out_type);
}

Result TypeChecker::PopType(Type* out_type) {
  Label* label;
  CHECK_RESULT(TopLabel(&label));
  Result result = TopType(out_type);
  if (type_stack_.size() > label->type_stack_limit)
    type_stack_.pop_back();
  return result;
}

Result TypeChecker::DropTypes(size_t drop_count) {
  Label* label;
  CHECK_RESULT(TopLabel(&label));
  if (label->type_stack_limit + drop_count > type_stack_.size()) {
    if (label->unreachable) {
      ResetTypeStackToLabel(label);
      return Result::Ok;
    }
    return Result::Error;
  }
  type_stack_.erase(type_stack_.end() - drop_count, type_stack_.end());
  return Result::Ok;
}

void TypeChecker::PushType(Type type) {
  if (type != Type::Void)
    type_stack_.push_back(type);
}

void TypeChecker::PushTypes(const TypeVector& types) {
  for (Type type : types)
    PushType(type);
}

Result TypeChecker::CheckTypeStackLimit(size_t expected, const char* desc) {
  Label* label;
  CHECK_RESULT(TopLabel(&label));
  size_t avail = type_stack_.size() - label->type_stack_limit;
  if (!label->unreachable && expected > avail) {
    PrintError("type stack size too small at %s. got %" PRIzd
               ", expected at least %" PRIzd,
               desc, avail, expected);
    return Result::Error;
  }
  return Result::Ok;
}

Result TypeChecker::CheckTypeStackEnd(const char* desc) {
  Label* label;
  CHECK_RESULT(TopLabel(&label));
  if (type_stack_.size() != label->type_stack_limit) {
    PrintError("type stack at end of %s is %" PRIzd ", expected %" PRIzd, desc,
               type_stack_.size(), label->type_stack_limit);
    return Result::Error;
  }
  return Result::Ok;
}

Result TypeChecker::CheckType(Type actual, Type expected, const char* desc) {
  if (expected != actual && expected != Type::Any && actual != Type::Any) {
    PrintError("type mismatch in %s, expected %s but got %s.", desc,
               GetTypeName(expected), GetTypeName(actual));
    return Result::Error;
  }
  return Result::Ok;
}

Result TypeChecker::CheckSignature(const TypeVector& sig, const char* desc) {
  Result result = CheckTypeStackLimit(sig.size(), desc);
  for (size_t i = 0; i < sig.size(); ++i) {
    Type actual = Type::Any;
    result |= PeekType(sig.size() - i - 1, &actual);
    result |= CheckType(actual, sig[i], desc);
  }
  return result;
}

Result TypeChecker::PopAndCheckSignature(const TypeVector& sig,
                                         const char* desc) {
  Result result = CheckSignature(sig, desc);
  result |= DropTypes(sig.size());
  return result;
}

Result TypeChecker::PopAndCheckCall(const TypeVector& param_types,
                                    const TypeVector& result_types,
                                    const char* desc) {
  Result result = CheckTypeStackLimit(param_types.size(), desc);
  for (size_t i = 0; i < param_types.size(); ++i) {
    Type actual = Type::Any;
    result |= PeekType(param_types.size() - i - 1, &actual);
    result |= CheckType(actual, param_types[i], desc);
  }
  result |= DropTypes(param_types.size());
  PushTypes(result_types);
  return result;
}

Result TypeChecker::PopAndCheck1Type(Type expected, const char* desc) {
  Result result = Result::Ok;
  Type actual = Type::Any;
  result |= CheckTypeStackLimit(1, desc);
  result |= PopType(&actual);
  result |= CheckType(actual, expected, desc);
  return result;
}

Result TypeChecker::PopAndCheck2Types(Type expected1,
                                      Type expected2,
                                      const char* desc) {
  Result result = Result::Ok;
  Type actual1 = Type::Any;
  Type actual2 = Type::Any;
  result |= CheckTypeStackLimit(2, desc);
  result |= PopType(&actual2);
  result |= PopType(&actual1);
  result |= CheckType(actual1, expected1, desc);
  result |= CheckType(actual2, expected2, desc);
  return result;
}

Result TypeChecker::PopAndCheck2TypesAreEqual(Type* out_type,
                                              const char* desc) {
  Result result = Result::Ok;
  Type right = Type::Any;
  Type left = Type::Any;
  result |= CheckTypeStackLimit(2, desc);
  result |= PopType(&right);
  result |= PopType(&left);
  result |= CheckType(left, right, desc);
  *out_type = right;
  return result;
}

Result TypeChecker::CheckOpcode1(Opcode opcode) {
  Result result = PopAndCheck1Type(opcode.GetParamType1(), opcode.GetName());
  PushType(opcode.GetResultType());
  return result;
}

Result TypeChecker::CheckOpcode2(Opcode opcode) {
  Result result = PopAndCheck2Types(opcode.GetParamType1(),
                                    opcode.GetParamType2(), opcode.GetName());
  PushType(opcode.GetResultType());
  return result;
}

Result TypeChecker::BeginFunction(const TypeVector* sig) {
  type_stack_.clear();
  label_stack_.clear();
  PushLabel(LabelType::Func, *sig);
  return Result::Ok;
}

Result TypeChecker::OnBinary(Opcode opcode) {
  return CheckOpcode2(opcode);
}

Result TypeChecker::OnBlock(const TypeVector* sig) {
  PushLabel(LabelType::Block, *sig);
  return Result::Ok;
}

Result TypeChecker::OnBr(Index depth) {
  Result result = Result::Ok;
  Label* label;
  CHECK_RESULT(GetLabel(depth, &label));
  if (label->label_type != LabelType::Loop)
    result |= CheckSignature(label->sig, "br");
  CHECK_RESULT(SetUnreachable());
  return result;
}

Result TypeChecker::OnBrIf(Index depth) {
  Result result = PopAndCheck1Type(Type::I32, "br_if");
  Label* label;
  CHECK_RESULT(GetLabel(depth, &label));
  if (label->label_type != LabelType::Loop) {
    result |= PopAndCheckSignature(label->sig, "br_if");
    PushTypes(label->sig);
  }
  return result;
}

Result TypeChecker::BeginBrTable() {
  br_table_sig_ = Type::Any;
  return PopAndCheck1Type(Type::I32, "br_table");
}

Result TypeChecker::OnBrTableTarget(Index depth) {
  Result result = Result::Ok;
  Label* label;
  CHECK_RESULT(GetLabel(depth, &label));
  Type label_sig;
  if (label->label_type == LabelType::Loop) {
    label_sig = Type::Void;
  } else {
    assert(label->sig.size() <= 1);
    label_sig = label->sig.size() == 0 ? Type::Void : label->sig[0];
  }

  result |= CheckType(br_table_sig_, label_sig, "br_table");
  br_table_sig_ = label_sig;

  if (label->label_type != LabelType::Loop)
    result |= CheckSignature(label->sig, "br_table");
  return result;
}

Result TypeChecker::EndBrTable() {
  return SetUnreachable();
}

Result TypeChecker::OnCall(const TypeVector* param_types,
                           const TypeVector* result_types) {
  return PopAndCheckCall(*param_types, *result_types, "call");
}

Result TypeChecker::OnCallIndirect(const TypeVector* param_types,
                                   const TypeVector* result_types) {
  Result result = PopAndCheck1Type(Type::I32, "call_indirect");
  result |= PopAndCheckCall(*param_types, *result_types, "call_indirect");
  return result;
}

Result TypeChecker::OnCompare(Opcode opcode) {
  return CheckOpcode2(opcode);
}

Result TypeChecker::OnCatch(const TypeVector* sig) {
  PushTypes(*sig);
  return Result::Ok;
}

Result TypeChecker::OnCatchBlock(const TypeVector* sig) {
  Result result = Result::Ok;
  Label* label;
  CHECK_RESULT(TopLabel(&label));
  result |= CheckLabelType(label, LabelType::Try);
  result |= PopAndCheckSignature(label->sig, "try block");
  result |= CheckTypeStackEnd("try block");
  ResetTypeStackToLabel(label);
  label->label_type = LabelType::Catch;
  label->unreachable = false;
  return result;
}

Result TypeChecker::OnConst(Type type) {
  PushType(type);
  return Result::Ok;
}

Result TypeChecker::OnConvert(Opcode opcode) {
  return CheckOpcode1(opcode);
}

Result TypeChecker::OnCurrentMemory() {
  PushType(Type::I32);
  return Result::Ok;
}

Result TypeChecker::OnDrop() {
  Result result = Result::Ok;
  Type type = Type::Any;
  result |= CheckTypeStackLimit(1, "drop");
  result |= PopType(&type);
  return result;
}

Result TypeChecker::OnElse() {
  Result result = Result::Ok;
  Label* label;
  CHECK_RESULT(TopLabel(&label));
  result |= CheckLabelType(label, LabelType::If);
  result |= PopAndCheckSignature(label->sig, "if true branch");
  result |= CheckTypeStackEnd("if true branch");
  ResetTypeStackToLabel(label);
  label->label_type = LabelType::Else;
  label->unreachable = false;
  return result;
}

Result TypeChecker::OnEnd(Label* label,
                          const char* sig_desc,
                          const char* end_desc) {
  Result result = Result::Ok;
  result |= PopAndCheckSignature(label->sig, sig_desc);
  result |= CheckTypeStackEnd(end_desc);
  ResetTypeStackToLabel(label);
  PushTypes(label->sig);
  PopLabel();
  return result;
}

Result TypeChecker::OnEnd() {
  Result result = Result::Ok;
  static const char* s_label_type_name[] = {"function", "block", "loop", "if",
                                            "if false branch", "try",
                                            "try catch"};
  WABT_STATIC_ASSERT(WABT_ARRAY_SIZE(s_label_type_name) == kLabelTypeCount);
  Label* label;
  CHECK_RESULT(TopLabel(&label));
  assert(static_cast<int>(label->label_type) < kLabelTypeCount);
  if (label->label_type == LabelType::If) {
    if (label->sig.size() != 0) {
      PrintError("if without else cannot have type signature.");
      result = Result::Error;
    }
  }
  const char* desc = s_label_type_name[static_cast<int>(label->label_type)];
  result |= OnEnd(label, desc, desc);
  return result;
}

Result TypeChecker::OnGrowMemory() {
  return CheckOpcode1(Opcode::GrowMemory);
}

Result TypeChecker::OnIf(const TypeVector* sig) {
  Result result = PopAndCheck1Type(Type::I32, "if");
  PushLabel(LabelType::If, *sig);
  return result;
}

Result TypeChecker::OnGetGlobal(Type type) {
  PushType(type);
  return Result::Ok;
}

Result TypeChecker::OnGetLocal(Type type) {
  PushType(type);
  return Result::Ok;
}

Result TypeChecker::OnLoad(Opcode opcode) {
  return CheckOpcode1(opcode);
}

Result TypeChecker::OnLoop(const TypeVector* sig) {
  PushLabel(LabelType::Loop, *sig);
  return Result::Ok;
}

Result TypeChecker::OnRethrow(Index depth) {
  Result result = Result::Ok;
  Label* label;
  CHECK_RESULT(GetLabel(depth, &label));
  if (label->label_type != LabelType::Catch) {
    std::string candidates;
    size_t last = label_stack_.size() - 1;
    for (size_t i = 0; i < label_stack_.size(); ++i) {
      if (label_stack_[last - i].label_type == LabelType::Catch) {
        if (!candidates.empty())
          candidates.append(", ");
        candidates.append(std::to_string(i));
      }
    }
    if (candidates.empty()) {
      PrintError("Rethrow not in try catch block");
    } else {
      PrintError("invalid rethrow depth: %" PRIindex " (catches: %s)", depth,
                 candidates.c_str());
    }
    result = Result::Error;
  }
  CHECK_RESULT(SetUnreachable());
  return result;
}

Result TypeChecker::OnThrow(const TypeVector* sig) {
  Result result = Result::Ok;
  result |= PopAndCheckSignature(*sig, "throw");
  CHECK_RESULT(SetUnreachable());
  return result;
}

Result TypeChecker::OnReturn() {
  Result result = Result::Ok;
  Label* func_label;
  CHECK_RESULT(GetLabel(label_stack_.size() - 1, &func_label));
  result |= PopAndCheckSignature(func_label->sig, "return");
  CHECK_RESULT(SetUnreachable());
  return result;
}

Result TypeChecker::OnSelect() {
  Result result = Result::Ok;
  result |= PopAndCheck1Type(Type::I32, "select");
  Type type = Type::Any;
  result |= PopAndCheck2TypesAreEqual(&type, "select");
  PushType(type);
  return result;
}

Result TypeChecker::OnSetGlobal(Type type) {
  return PopAndCheck1Type(type, "set_global");
}

Result TypeChecker::OnSetLocal(Type type) {
  return PopAndCheck1Type(type, "set_local");
}

Result TypeChecker::OnStore(Opcode opcode) {
  return CheckOpcode2(opcode);
}

Result TypeChecker::OnTryBlock(const TypeVector* sig) {
  PushLabel(LabelType::Try, *sig);
  return Result::Ok;
}

Result TypeChecker::OnTeeLocal(Type type) {
  Result result = Result::Ok;
  result |= PopAndCheck1Type(type, "tee_local");
  PushType(type);
  return result;
}

Result TypeChecker::OnUnary(Opcode opcode) {
  return CheckOpcode1(opcode);
}

Result TypeChecker::OnUnreachable() {
  return SetUnreachable();
}

Result TypeChecker::EndFunction() {
  Result result = Result::Ok;
  Label* label;
  CHECK_RESULT(TopLabel(&label));
  result |= CheckLabelType(label, LabelType::Func);
  result |= OnEnd(label, "implicit return", "function");
  return result;
}

}  // namespace wabt

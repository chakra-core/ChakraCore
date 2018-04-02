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

#ifndef WABT_TYPE_CHECKER_H_
#define WABT_TYPE_CHECKER_H_

#include <functional>
#include <vector>

#include "src/common.h"
#include "src/opcode.h"

namespace wabt {

class TypeChecker {
 public:
  typedef std::function<void(const char* msg)> ErrorCallback;

  struct Label {
    Label(LabelType, const TypeVector& sig, size_t limit);

    LabelType label_type;
    TypeVector sig;
    size_t type_stack_limit;
    bool unreachable;
  };

  TypeChecker() = default;
  explicit TypeChecker(const ErrorCallback&);

  void set_error_callback(const ErrorCallback& error_callback) {
    error_callback_ = error_callback;
  }

  size_t type_stack_size() const { return type_stack_.size(); }

  bool IsUnreachable();
  Result GetLabel(Index depth, Label** out_label);

  Result BeginFunction(const TypeVector* sig);
  Result OnAtomicLoad(Opcode);
  Result OnAtomicStore(Opcode);
  Result OnAtomicRmw(Opcode);
  Result OnAtomicRmwCmpxchg(Opcode);
  Result OnAtomicWait(Opcode);
  Result OnAtomicWake(Opcode);
  Result OnBinary(Opcode);
  Result OnBlock(const TypeVector* sig);
  Result OnBr(Index depth);
  Result OnBrIf(Index depth);
  Result BeginBrTable();
  Result OnBrTableTarget(Index depth);
  Result EndBrTable();
  Result OnCall(const TypeVector* param_types, const TypeVector* result_types);
  Result OnCallIndirect(const TypeVector* param_types,
                        const TypeVector* result_types);
  Result OnCatch();
  Result OnCompare(Opcode);
  Result OnConst(Type);
  Result OnConvert(Opcode);
  Result OnCurrentMemory();
  Result OnDrop();
  Result OnElse();
  Result OnEnd();
  Result OnGetGlobal(Type);
  Result OnGetLocal(Type);
  Result OnGrowMemory();
  Result OnIf(const TypeVector* sig);
  Result OnIfExcept(const TypeVector* sig, const TypeVector* except_sig);
  Result OnLoad(Opcode);
  Result OnLoop(const TypeVector* sig);
  Result OnRethrow();
  Result OnReturn();
  Result OnSelect();
  Result OnSetGlobal(Type);
  Result OnSetLocal(Type);
  Result OnSimdLaneOp(Opcode, uint64_t);
  Result OnSimdShuffleOp(Opcode, v128);
  Result OnStore(Opcode);
  Result OnTeeLocal(Type);
  Result OnTernary(Opcode);
  Result OnThrow(const TypeVector* sig);
  Result OnTry(const TypeVector* sig);
  Result OnUnary(Opcode);
  Result OnUnreachable();
  Result EndFunction();

 private:
  void WABT_PRINTF_FORMAT(2, 3) PrintError(const char* fmt, ...);
  Result TopLabel(Label** out_label);
  void ResetTypeStackToLabel(Label* label);
  Result SetUnreachable();
  void PushLabel(LabelType label_type, const TypeVector& sig);
  Result PopLabel();
  Result CheckLabelType(Label* label, LabelType label_type);
  Result PeekType(Index depth, Type* out_type);
  Result PeekAndCheckType(Index depth, Type expected);
  Result DropTypes(size_t drop_count);
  void PushType(Type type);
  void PushTypes(const TypeVector& types);
  Result CheckTypeStackEnd(const char* desc);
  Result CheckType(Type actual, Type expected);
  Result CheckSignature(const TypeVector& sig);
  Result PopAndCheckSignature(const TypeVector& sig, const char* desc);
  Result PopAndCheckCall(const TypeVector& param_types,
                         const TypeVector& result_types,
                         const char* desc);
  Result PopAndCheck1Type(Type expected, const char* desc);
  Result PopAndCheck2Types(Type expected1, Type expected2, const char* desc);
  Result PopAndCheck3Types(Type expected1,
                           Type expected2,
                           Type expected3,
                           const char* desc);
  Result CheckOpcode1(Opcode opcode);
  Result CheckOpcode2(Opcode opcode);
  Result CheckOpcode3(Opcode opcode);
  Result OnEnd(Label* label, const char* sig_desc, const char* end_desc);

  template <typename... Args>
  void PrintStackIfFailed(Result result, const char* desc, Args... args) {
    // Minor optimzation, check result before constructing the vector to pass
    // to the other overload of PrintStackIfFailed.
    if (Failed(result)) {
      PrintStackIfFailed(result, desc, {args...});
    }
  }

  void PrintStackIfFailed(Result, const char* desc, const TypeVector&);

  ErrorCallback error_callback_;
  TypeVector type_stack_;
  std::vector<Label> label_stack_;
  // TODO(binji): This will need to be complete signature when signatures with
  // multiple types are allowed.
  Type br_table_sig_ = Type::Void;
};

}  // namespace wabt

#endif /* WABT_TYPE_CHECKER_H_ */

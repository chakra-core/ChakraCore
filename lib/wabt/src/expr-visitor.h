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

#ifndef WABT_EXPR_VISITOR_H_
#define WABT_EXPR_VISITOR_H_

#include "src/common.h"
#include "src/ir.h"

namespace wabt {

class ExprVisitor {
 public:
  class Delegate;
  class DelegateNop;

  explicit ExprVisitor(Delegate* delegate);

  Result VisitExpr(Expr*);
  Result VisitExprList(ExprList&);
  Result VisitFunc(Func*);

 private:
  Delegate* delegate_;
};

class ExprVisitor::Delegate {
 public:
  virtual ~Delegate() {}

  virtual Result OnBinaryExpr(BinaryExpr*) = 0;
  virtual Result BeginBlockExpr(BlockExpr*) = 0;
  virtual Result EndBlockExpr(BlockExpr*) = 0;
  virtual Result OnBrExpr(BrExpr*) = 0;
  virtual Result OnBrIfExpr(BrIfExpr*) = 0;
  virtual Result OnBrTableExpr(BrTableExpr*) = 0;
  virtual Result OnCallExpr(CallExpr*) = 0;
  virtual Result OnCallIndirectExpr(CallIndirectExpr*) = 0;
  virtual Result OnCompareExpr(CompareExpr*) = 0;
  virtual Result OnConstExpr(ConstExpr*) = 0;
  virtual Result OnConvertExpr(ConvertExpr*) = 0;
  virtual Result OnCurrentMemoryExpr(CurrentMemoryExpr*) = 0;
  virtual Result OnDropExpr(DropExpr*) = 0;
  virtual Result OnGetGlobalExpr(GetGlobalExpr*) = 0;
  virtual Result OnGetLocalExpr(GetLocalExpr*) = 0;
  virtual Result OnGrowMemoryExpr(GrowMemoryExpr*) = 0;
  virtual Result BeginIfExpr(IfExpr*) = 0;
  virtual Result AfterIfTrueExpr(IfExpr*) = 0;
  virtual Result EndIfExpr(IfExpr*) = 0;
  virtual Result OnLoadExpr(LoadExpr*) = 0;
  virtual Result BeginLoopExpr(LoopExpr*) = 0;
  virtual Result EndLoopExpr(LoopExpr*) = 0;
  virtual Result OnNopExpr(NopExpr*) = 0;
  virtual Result OnReturnExpr(ReturnExpr*) = 0;
  virtual Result OnSelectExpr(SelectExpr*) = 0;
  virtual Result OnSetGlobalExpr(SetGlobalExpr*) = 0;
  virtual Result OnSetLocalExpr(SetLocalExpr*) = 0;
  virtual Result OnStoreExpr(StoreExpr*) = 0;
  virtual Result OnTeeLocalExpr(TeeLocalExpr*) = 0;
  virtual Result OnUnaryExpr(UnaryExpr*) = 0;
  virtual Result OnUnreachableExpr(UnreachableExpr*) = 0;
  virtual Result BeginTryExpr(TryExpr*) = 0;
  virtual Result EndTryExpr(TryExpr*) = 0;
  virtual Result OnCatchExpr(TryExpr*, Catch*) = 0;
  virtual Result OnThrowExpr(ThrowExpr*) = 0;
  virtual Result OnRethrowExpr(RethrowExpr*) = 0;
  virtual Result OnAtomicLoadExpr(AtomicLoadExpr*) = 0;
  virtual Result OnAtomicStoreExpr(AtomicStoreExpr*) = 0;
  virtual Result OnAtomicRmwExpr(AtomicRmwExpr*) = 0;
  virtual Result OnAtomicRmwCmpxchgExpr(AtomicRmwCmpxchgExpr*) = 0;
};

class ExprVisitor::DelegateNop : public ExprVisitor::Delegate {
 public:
  Result OnBinaryExpr(BinaryExpr*) override { return Result::Ok; }
  Result BeginBlockExpr(BlockExpr*) override { return Result::Ok; }
  Result EndBlockExpr(BlockExpr*) override { return Result::Ok; }
  Result OnBrExpr(BrExpr*) override { return Result::Ok; }
  Result OnBrIfExpr(BrIfExpr*) override { return Result::Ok; }
  Result OnBrTableExpr(BrTableExpr*) override { return Result::Ok; }
  Result OnCallExpr(CallExpr*) override { return Result::Ok; }
  Result OnCallIndirectExpr(CallIndirectExpr*) override { return Result::Ok; }
  Result OnCompareExpr(CompareExpr*) override { return Result::Ok; }
  Result OnConstExpr(ConstExpr*) override { return Result::Ok; }
  Result OnConvertExpr(ConvertExpr*) override { return Result::Ok; }
  Result OnCurrentMemoryExpr(CurrentMemoryExpr*) override { return Result::Ok; }
  Result OnDropExpr(DropExpr*) override { return Result::Ok; }
  Result OnGetGlobalExpr(GetGlobalExpr*) override { return Result::Ok; }
  Result OnGetLocalExpr(GetLocalExpr*) override { return Result::Ok; }
  Result OnGrowMemoryExpr(GrowMemoryExpr*) override { return Result::Ok; }
  Result BeginIfExpr(IfExpr*) override { return Result::Ok; }
  Result AfterIfTrueExpr(IfExpr*) override { return Result::Ok; }
  Result EndIfExpr(IfExpr*) override { return Result::Ok; }
  Result OnLoadExpr(LoadExpr*) override { return Result::Ok; }
  Result BeginLoopExpr(LoopExpr*) override { return Result::Ok; }
  Result EndLoopExpr(LoopExpr*) override { return Result::Ok; }
  Result OnNopExpr(NopExpr*) override { return Result::Ok; }
  Result OnReturnExpr(ReturnExpr*) override { return Result::Ok; }
  Result OnSelectExpr(SelectExpr*) override { return Result::Ok; }
  Result OnSetGlobalExpr(SetGlobalExpr*) override { return Result::Ok; }
  Result OnSetLocalExpr(SetLocalExpr*) override { return Result::Ok; }
  Result OnStoreExpr(StoreExpr*) override { return Result::Ok; }
  Result OnTeeLocalExpr(TeeLocalExpr*) override { return Result::Ok; }
  Result OnUnaryExpr(UnaryExpr*) override { return Result::Ok; }
  Result OnUnreachableExpr(UnreachableExpr*) override { return Result::Ok; }
  Result BeginTryExpr(TryExpr*) override { return Result::Ok; }
  Result EndTryExpr(TryExpr*) override { return Result::Ok; }
  Result OnCatchExpr(TryExpr*, Catch*) override { return Result::Ok; }
  Result OnThrowExpr(ThrowExpr*) override { return Result::Ok; }
  Result OnRethrowExpr(RethrowExpr*) override { return Result::Ok; }
  Result OnAtomicLoadExpr(AtomicLoadExpr*) override { return Result::Ok; }
  Result OnAtomicStoreExpr(AtomicStoreExpr*) override { return Result::Ok; }
  Result OnAtomicRmwExpr(AtomicRmwExpr*) override { return Result::Ok; }
  Result OnAtomicRmwCmpxchgExpr(AtomicRmwCmpxchgExpr*) override {
    return Result::Ok;
  }
};

}  // namespace wabt

#endif  // WABT_EXPR_VISITOR_H_

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

#include "common.h"

namespace wabt {

struct Expr;
struct Func;

class ExprVisitor {
 public:
  class Delegate;
  class DelegateNop;

  explicit ExprVisitor(Delegate* delegate);

  Result VisitExpr(Expr*);
  Result VisitExprList(Expr*);
  Result VisitFunc(Func*);

 private:
  Delegate* delegate_;
};

class ExprVisitor::Delegate {
 public:
  virtual ~Delegate() {}

  virtual Result OnBinaryExpr(Expr*) = 0;
  virtual Result BeginBlockExpr(Expr*) = 0;
  virtual Result EndBlockExpr(Expr*) = 0;
  virtual Result OnBrExpr(Expr*) = 0;
  virtual Result OnBrIfExpr(Expr*) = 0;
  virtual Result OnBrTableExpr(Expr*) = 0;
  virtual Result OnCallExpr(Expr*) = 0;
  virtual Result OnCallIndirectExpr(Expr*) = 0;
  virtual Result OnCompareExpr(Expr*) = 0;
  virtual Result OnConstExpr(Expr*) = 0;
  virtual Result OnConvertExpr(Expr*) = 0;
  virtual Result OnCurrentMemoryExpr(Expr*) = 0;
  virtual Result OnDropExpr(Expr*) = 0;
  virtual Result OnGetGlobalExpr(Expr*) = 0;
  virtual Result OnGetLocalExpr(Expr*) = 0;
  virtual Result OnGrowMemoryExpr(Expr*) = 0;
  virtual Result BeginIfExpr(Expr*) = 0;
  virtual Result AfterIfTrueExpr(Expr*) = 0;
  virtual Result EndIfExpr(Expr*) = 0;
  virtual Result OnLoadExpr(Expr*) = 0;
  virtual Result BeginLoopExpr(Expr*) = 0;
  virtual Result EndLoopExpr(Expr*) = 0;
  virtual Result OnNopExpr(Expr*) = 0;
  virtual Result OnReturnExpr(Expr*) = 0;
  virtual Result OnSelectExpr(Expr*) = 0;
  virtual Result OnSetGlobalExpr(Expr*) = 0;
  virtual Result OnSetLocalExpr(Expr*) = 0;
  virtual Result OnStoreExpr(Expr*) = 0;
  virtual Result OnTeeLocalExpr(Expr*) = 0;
  virtual Result OnUnaryExpr(Expr*) = 0;
  virtual Result OnUnreachableExpr(Expr*) = 0;
};

class ExprVisitor::DelegateNop : public ExprVisitor::Delegate {
 public:
  Result OnBinaryExpr(Expr*) override { return Result::Ok; }
  Result BeginBlockExpr(Expr*) override { return Result::Ok; }
  Result EndBlockExpr(Expr*) override { return Result::Ok; }
  Result OnBrExpr(Expr*) override { return Result::Ok; }
  Result OnBrIfExpr(Expr*) override { return Result::Ok; }
  Result OnBrTableExpr(Expr*) override { return Result::Ok; }
  Result OnCallExpr(Expr*) override { return Result::Ok; }
  Result OnCallIndirectExpr(Expr*) override { return Result::Ok; }
  Result OnCompareExpr(Expr*) override { return Result::Ok; }
  Result OnConstExpr(Expr*) override { return Result::Ok; }
  Result OnConvertExpr(Expr*) override { return Result::Ok; }
  Result OnCurrentMemoryExpr(Expr*) override { return Result::Ok; }
  Result OnDropExpr(Expr*) override { return Result::Ok; }
  Result OnGetGlobalExpr(Expr*) override { return Result::Ok; }
  Result OnGetLocalExpr(Expr*) override { return Result::Ok; }
  Result OnGrowMemoryExpr(Expr*) override { return Result::Ok; }
  Result BeginIfExpr(Expr*) override { return Result::Ok; }
  Result AfterIfTrueExpr(Expr*) override { return Result::Ok; }
  Result EndIfExpr(Expr*) override { return Result::Ok; }
  Result OnLoadExpr(Expr*) override { return Result::Ok; }
  Result BeginLoopExpr(Expr*) override { return Result::Ok; }
  Result EndLoopExpr(Expr*) override { return Result::Ok; }
  Result OnNopExpr(Expr*) override { return Result::Ok; }
  Result OnReturnExpr(Expr*) override { return Result::Ok; }
  Result OnSelectExpr(Expr*) override { return Result::Ok; }
  Result OnSetGlobalExpr(Expr*) override { return Result::Ok; }
  Result OnSetLocalExpr(Expr*) override { return Result::Ok; }
  Result OnStoreExpr(Expr*) override { return Result::Ok; }
  Result OnTeeLocalExpr(Expr*) override { return Result::Ok; }
  Result OnUnaryExpr(Expr*) override { return Result::Ok; }
  Result OnUnreachableExpr(Expr*) override { return Result::Ok; }
};

}  // namespace wabt

#endif  // WABT_EXPR_VISITOR_H_

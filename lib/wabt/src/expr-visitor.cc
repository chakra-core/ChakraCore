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

#include "expr-visitor.h"

#include "ir.h"

#define CHECK_RESULT(expr)   \
  do {                       \
    if (WABT_FAILED((expr))) \
      return Result::Error;  \
  } while (0)

namespace wabt {

ExprVisitor::ExprVisitor(Delegate* delegate) : delegate_(delegate) {}

Result ExprVisitor::VisitExpr(Expr* expr) {
  switch (expr->type) {
    case ExprType::Binary:
      CHECK_RESULT(delegate_->OnBinaryExpr(expr));
      break;

    case ExprType::Block:
      CHECK_RESULT(delegate_->BeginBlockExpr(expr));
      CHECK_RESULT(VisitExprList(expr->block->first));
      CHECK_RESULT(delegate_->EndBlockExpr(expr));
      break;

    case ExprType::Br:
      CHECK_RESULT(delegate_->OnBrExpr(expr));
      break;

    case ExprType::BrIf:
      CHECK_RESULT(delegate_->OnBrIfExpr(expr));
      break;

    case ExprType::BrTable:
      CHECK_RESULT(delegate_->OnBrTableExpr(expr));
      break;

    case ExprType::Call:
      CHECK_RESULT(delegate_->OnCallExpr(expr));
      break;

    case ExprType::CallIndirect:
      CHECK_RESULT(delegate_->OnCallIndirectExpr(expr));
      break;

    case ExprType::Catch:
      // TODO(karlschimpf): Define
      WABT_FATAL("Catch: don't know how to visit\n");
      return Result::Error;

    case ExprType::CatchAll:
      // TODO(karlschimpf): Define
      WABT_FATAL("CatchAll: don't know how to visit\n");
      return Result::Error;

    case ExprType::Compare:
      CHECK_RESULT(delegate_->OnCompareExpr(expr));
      break;

    case ExprType::Const:
      CHECK_RESULT(delegate_->OnConstExpr(expr));
      break;

    case ExprType::Convert:
      CHECK_RESULT(delegate_->OnConvertExpr(expr));
      break;

    case ExprType::CurrentMemory:
      CHECK_RESULT(delegate_->OnCurrentMemoryExpr(expr));
      break;

    case ExprType::Drop:
      CHECK_RESULT(delegate_->OnDropExpr(expr));
      break;

    case ExprType::GetGlobal:
      CHECK_RESULT(delegate_->OnGetGlobalExpr(expr));
      break;

    case ExprType::GetLocal:
      CHECK_RESULT(delegate_->OnGetLocalExpr(expr));
      break;

    case ExprType::GrowMemory:
      CHECK_RESULT(delegate_->OnGrowMemoryExpr(expr));
      break;

    case ExprType::If:
      CHECK_RESULT(delegate_->BeginIfExpr(expr));
      CHECK_RESULT(VisitExprList(expr->if_.true_->first));
      CHECK_RESULT(delegate_->AfterIfTrueExpr(expr));
      CHECK_RESULT(VisitExprList(expr->if_.false_));
      CHECK_RESULT(delegate_->EndIfExpr(expr));
      break;

    case ExprType::Load:
      CHECK_RESULT(delegate_->OnLoadExpr(expr));
      break;

    case ExprType::Loop:
      CHECK_RESULT(delegate_->BeginLoopExpr(expr));
      CHECK_RESULT(VisitExprList(expr->loop->first));
      CHECK_RESULT(delegate_->EndLoopExpr(expr));
      break;

    case ExprType::Nop:
      CHECK_RESULT(delegate_->OnNopExpr(expr));
      break;

    case ExprType::Rethrow:
      // TODO(karlschimpf): Define
      WABT_FATAL("Rethrow: don't know how to visit\n");
      return Result::Error;

    case ExprType::Return:
      CHECK_RESULT(delegate_->OnReturnExpr(expr));
      break;

    case ExprType::Select:
      CHECK_RESULT(delegate_->OnSelectExpr(expr));
      break;

    case ExprType::SetGlobal:
      CHECK_RESULT(delegate_->OnSetGlobalExpr(expr));
      break;

    case ExprType::SetLocal:
      CHECK_RESULT(delegate_->OnSetLocalExpr(expr));
      break;

    case ExprType::Store:
      CHECK_RESULT(delegate_->OnStoreExpr(expr));
      break;

    case ExprType::TeeLocal:
      CHECK_RESULT(delegate_->OnTeeLocalExpr(expr));
      break;

    case ExprType::Throw:
      // TODO(karlschimpf): Define
      WABT_FATAL("Throw: don't know how to visit\n");
      return Result::Error;

    case ExprType::TryBlock:
      // TODO(karlschimpf): Define
      WABT_FATAL("TryBlock: don't know how to visit\n");
      return Result::Error;
      break;

    case ExprType::Unary:
      CHECK_RESULT(delegate_->OnUnaryExpr(expr));
      break;

    case ExprType::Unreachable:
      CHECK_RESULT(delegate_->OnUnreachableExpr(expr));
      break;
  }

  return Result::Ok;
}

Result ExprVisitor::VisitExprList(Expr* first) {
  for (Expr* expr = first; expr; expr = expr->next)
    CHECK_RESULT(VisitExpr(expr));
  return Result::Ok;
}

Result ExprVisitor::VisitFunc(Func* func) {
  return VisitExprList(func->first_expr);
}

}  // namespace wabt

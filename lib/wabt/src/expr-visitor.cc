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

#include "cast.h"
#include "ir.h"

#define CHECK_RESULT(expr)   \
  do {                       \
    if (Failed((expr)))      \
      return Result::Error;  \
  } while (0)

namespace wabt {

ExprVisitor::ExprVisitor(Delegate* delegate) : delegate_(delegate) {}

Result ExprVisitor::VisitExpr(Expr* expr) {
  switch (expr->type) {
    case ExprType::Binary:
      CHECK_RESULT(delegate_->OnBinaryExpr(cast<BinaryExpr>(expr)));
      break;

    case ExprType::Block: {
      auto block_expr = cast<BlockExpr>(expr);
      CHECK_RESULT(delegate_->BeginBlockExpr(block_expr));
      CHECK_RESULT(VisitExprList(block_expr->block->exprs));
      CHECK_RESULT(delegate_->EndBlockExpr(block_expr));
      break;
    }

    case ExprType::Br:
      CHECK_RESULT(delegate_->OnBrExpr(cast<BrExpr>(expr)));
      break;

    case ExprType::BrIf:
      CHECK_RESULT(delegate_->OnBrIfExpr(cast<BrIfExpr>(expr)));
      break;

    case ExprType::BrTable:
      CHECK_RESULT(delegate_->OnBrTableExpr(cast<BrTableExpr>(expr)));
      break;

    case ExprType::Call:
      CHECK_RESULT(delegate_->OnCallExpr(cast<CallExpr>(expr)));
      break;

    case ExprType::CallIndirect:
      CHECK_RESULT(delegate_->OnCallIndirectExpr(cast<CallIndirectExpr>(expr)));
      break;

    case ExprType::Compare:
      CHECK_RESULT(delegate_->OnCompareExpr(cast<CompareExpr>(expr)));
      break;

    case ExprType::Const:
      CHECK_RESULT(delegate_->OnConstExpr(cast<ConstExpr>(expr)));
      break;

    case ExprType::Convert:
      CHECK_RESULT(delegate_->OnConvertExpr(cast<ConvertExpr>(expr)));
      break;

    case ExprType::CurrentMemory:
      CHECK_RESULT(
          delegate_->OnCurrentMemoryExpr(cast<CurrentMemoryExpr>(expr)));
      break;

    case ExprType::Drop:
      CHECK_RESULT(delegate_->OnDropExpr(cast<DropExpr>(expr)));
      break;

    case ExprType::GetGlobal:
      CHECK_RESULT(delegate_->OnGetGlobalExpr(cast<GetGlobalExpr>(expr)));
      break;

    case ExprType::GetLocal:
      CHECK_RESULT(delegate_->OnGetLocalExpr(cast<GetLocalExpr>(expr)));
      break;

    case ExprType::GrowMemory:
      CHECK_RESULT(delegate_->OnGrowMemoryExpr(cast<GrowMemoryExpr>(expr)));
      break;

    case ExprType::If: {
      auto if_expr = cast<IfExpr>(expr);
      CHECK_RESULT(delegate_->BeginIfExpr(if_expr));
      CHECK_RESULT(VisitExprList(if_expr->true_->exprs));
      CHECK_RESULT(delegate_->AfterIfTrueExpr(if_expr));
      CHECK_RESULT(VisitExprList(if_expr->false_));
      CHECK_RESULT(delegate_->EndIfExpr(if_expr));
      break;
    }

    case ExprType::Load:
      CHECK_RESULT(delegate_->OnLoadExpr(cast<LoadExpr>(expr)));
      break;

    case ExprType::Loop: {
      auto loop_expr = cast<LoopExpr>(expr);
      CHECK_RESULT(delegate_->BeginLoopExpr(loop_expr));
      CHECK_RESULT(VisitExprList(loop_expr->block->exprs));
      CHECK_RESULT(delegate_->EndLoopExpr(loop_expr));
      break;
    }

    case ExprType::Nop:
      CHECK_RESULT(delegate_->OnNopExpr(cast<NopExpr>(expr)));
      break;

    case ExprType::Rethrow:
      CHECK_RESULT(delegate_->OnRethrowExpr(cast<RethrowExpr>(expr)));
      break;

    case ExprType::Return:
      CHECK_RESULT(delegate_->OnReturnExpr(cast<ReturnExpr>(expr)));
      break;

    case ExprType::Select:
      CHECK_RESULT(delegate_->OnSelectExpr(cast<SelectExpr>(expr)));
      break;

    case ExprType::SetGlobal:
      CHECK_RESULT(delegate_->OnSetGlobalExpr(cast<SetGlobalExpr>(expr)));
      break;

    case ExprType::SetLocal:
      CHECK_RESULT(delegate_->OnSetLocalExpr(cast<SetLocalExpr>(expr)));
      break;

    case ExprType::Store:
      CHECK_RESULT(delegate_->OnStoreExpr(cast<StoreExpr>(expr)));
      break;

    case ExprType::TeeLocal:
      CHECK_RESULT(delegate_->OnTeeLocalExpr(cast<TeeLocalExpr>(expr)));
      break;

    case ExprType::Throw:
      CHECK_RESULT(delegate_->OnThrowExpr(cast<ThrowExpr>(expr)));
      break;

    case ExprType::TryBlock: {
      auto try_expr = cast<TryExpr>(expr);
      CHECK_RESULT(delegate_->BeginTryExpr(try_expr));
      CHECK_RESULT(VisitExprList(try_expr->block->exprs));
      for (Catch* catch_ : try_expr->catches) {
        CHECK_RESULT(delegate_->OnCatchExpr(try_expr, catch_));
        CHECK_RESULT(VisitExprList(catch_->exprs));
      }
      CHECK_RESULT(delegate_->EndTryExpr(try_expr));
      break;
    }

    case ExprType::Unary:
      CHECK_RESULT(delegate_->OnUnaryExpr(cast<UnaryExpr>(expr)));
      break;

    case ExprType::Unreachable:
      CHECK_RESULT(delegate_->OnUnreachableExpr(cast<UnreachableExpr>(expr)));
      break;
  }

  return Result::Ok;
}

Result ExprVisitor::VisitExprList(ExprList& exprs) {
  for (Expr& expr : exprs)
    CHECK_RESULT(VisitExpr(&expr));
  return Result::Ok;
}

Result ExprVisitor::VisitFunc(Func* func) {
  return VisitExprList(func->exprs);
}

}  // namespace wabt

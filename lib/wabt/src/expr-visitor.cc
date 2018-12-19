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

#include "src/expr-visitor.h"

#include "src/cast.h"
#include "src/ir.h"

namespace wabt {

ExprVisitor::ExprVisitor(Delegate* delegate) : delegate_(delegate) {}

Result ExprVisitor::VisitExpr(Expr* root_expr) {
  state_stack_.clear();
  expr_stack_.clear();
  expr_iter_stack_.clear();

  PushDefault(root_expr);

  while (!state_stack_.empty()) {
    State state = state_stack_.back();
    auto* expr = expr_stack_.back();

    switch (state) {
      case State::Default:
        PopDefault();
        CHECK_RESULT(HandleDefaultState(expr));
        break;

      case State::Block: {
        auto block_expr = cast<BlockExpr>(expr);
        auto& iter = expr_iter_stack_.back();
        if (iter != block_expr->block.exprs.end()) {
          PushDefault(&*iter++);
        } else {
          CHECK_RESULT(delegate_->EndBlockExpr(block_expr));
          PopExprlist();
        }
        break;
      }

      case State::IfTrue: {
        auto if_expr = cast<IfExpr>(expr);
        auto& iter = expr_iter_stack_.back();
        if (iter != if_expr->true_.exprs.end()) {
          PushDefault(&*iter++);
        } else {
          CHECK_RESULT(delegate_->AfterIfTrueExpr(if_expr));
          PopExprlist();
          PushExprlist(State::IfFalse, expr, if_expr->false_);
        }
        break;
      }

      case State::IfFalse: {
        auto if_expr = cast<IfExpr>(expr);
        auto& iter = expr_iter_stack_.back();
        if (iter != if_expr->false_.end()) {
          PushDefault(&*iter++);
        } else {
          CHECK_RESULT(delegate_->EndIfExpr(if_expr));
          PopExprlist();
        }
        break;
      }

      case State::IfExceptTrue: {
        auto if_except_expr = cast<IfExceptExpr>(expr);
        auto& iter = expr_iter_stack_.back();
        if (iter != if_except_expr->true_.exprs.end()) {
          PushDefault(&*iter++);
        } else {
          CHECK_RESULT(delegate_->AfterIfExceptTrueExpr(if_except_expr));
          PopExprlist();
          PushExprlist(State::IfExceptFalse, expr, if_except_expr->false_);
        }
        break;
      }

      case State::IfExceptFalse: {
        auto if_except_expr = cast<IfExceptExpr>(expr);
        auto& iter = expr_iter_stack_.back();
        if (iter != if_except_expr->false_.end()) {
          PushDefault(&*iter++);
        } else {
          CHECK_RESULT(delegate_->EndIfExceptExpr(if_except_expr));
          PopExprlist();
        }
        break;
      }

      case State::Loop: {
        auto loop_expr = cast<LoopExpr>(expr);
        auto& iter = expr_iter_stack_.back();
        if (iter != loop_expr->block.exprs.end()) {
          PushDefault(&*iter++);
        } else {
          CHECK_RESULT(delegate_->EndLoopExpr(loop_expr));
          PopExprlist();
        }
        break;
      }

      case State::Try: {
        auto try_expr = cast<TryExpr>(expr);
        auto& iter = expr_iter_stack_.back();
        if (iter != try_expr->block.exprs.end()) {
          PushDefault(&*iter++);
        } else {
          if (try_expr->catch_.empty()) {
            CHECK_RESULT(delegate_->EndTryExpr(try_expr));
            PopExprlist();
          } else {
            CHECK_RESULT(delegate_->OnCatchExpr(try_expr));
            PopExprlist();
            PushExprlist(State::Catch, expr, try_expr->catch_);
          }
        }
        break;
      }

      case State::Catch: {
        auto try_expr = cast<TryExpr>(expr);
        auto& iter = expr_iter_stack_.back();
        if (iter != try_expr->catch_.end()) {
          PushDefault(&*iter++);
        } else {
          CHECK_RESULT(delegate_->EndTryExpr(try_expr));
          PopExprlist();
        }
        break;
      }
    }
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

Result ExprVisitor::HandleDefaultState(Expr* expr) {
  switch (expr->type()) {
    case ExprType::AtomicLoad:
      CHECK_RESULT(delegate_->OnAtomicLoadExpr(cast<AtomicLoadExpr>(expr)));
      break;

    case ExprType::AtomicStore:
      CHECK_RESULT(delegate_->OnAtomicStoreExpr(cast<AtomicStoreExpr>(expr)));
      break;

    case ExprType::AtomicRmw:
      CHECK_RESULT(delegate_->OnAtomicRmwExpr(cast<AtomicRmwExpr>(expr)));
      break;

    case ExprType::AtomicRmwCmpxchg:
      CHECK_RESULT(
          delegate_->OnAtomicRmwCmpxchgExpr(cast<AtomicRmwCmpxchgExpr>(expr)));
      break;

    case ExprType::AtomicWait:
      CHECK_RESULT(delegate_->OnAtomicWaitExpr(cast<AtomicWaitExpr>(expr)));
      break;

    case ExprType::AtomicWake:
      CHECK_RESULT(delegate_->OnAtomicWakeExpr(cast<AtomicWakeExpr>(expr)));
      break;

    case ExprType::Binary:
      CHECK_RESULT(delegate_->OnBinaryExpr(cast<BinaryExpr>(expr)));
      break;

    case ExprType::Block: {
      auto block_expr = cast<BlockExpr>(expr);
      CHECK_RESULT(delegate_->BeginBlockExpr(block_expr));
      PushExprlist(State::Block, expr, block_expr->block.exprs);
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

    case ExprType::Drop:
      CHECK_RESULT(delegate_->OnDropExpr(cast<DropExpr>(expr)));
      break;

    case ExprType::GetGlobal:
      CHECK_RESULT(delegate_->OnGetGlobalExpr(cast<GetGlobalExpr>(expr)));
      break;

    case ExprType::GetLocal:
      CHECK_RESULT(delegate_->OnGetLocalExpr(cast<GetLocalExpr>(expr)));
      break;

    case ExprType::If: {
      auto if_expr = cast<IfExpr>(expr);
      CHECK_RESULT(delegate_->BeginIfExpr(if_expr));
      PushExprlist(State::IfTrue, expr, if_expr->true_.exprs);
      break;
    }

    case ExprType::IfExcept: {
      auto if_except_expr = cast<IfExceptExpr>(expr);
      CHECK_RESULT(delegate_->BeginIfExceptExpr(if_except_expr));
      PushExprlist(State::IfExceptTrue, expr, if_except_expr->true_.exprs);
      break;
    }

    case ExprType::Load:
      CHECK_RESULT(delegate_->OnLoadExpr(cast<LoadExpr>(expr)));
      break;

    case ExprType::Loop: {
      auto loop_expr = cast<LoopExpr>(expr);
      CHECK_RESULT(delegate_->BeginLoopExpr(loop_expr));
      PushExprlist(State::Loop, expr, loop_expr->block.exprs);
      break;
    }

    case ExprType::MemoryCopy:
      CHECK_RESULT(delegate_->OnMemoryCopyExpr(cast<MemoryCopyExpr>(expr)));
      break;

    case ExprType::MemoryDrop:
      CHECK_RESULT(delegate_->OnMemoryDropExpr(cast<MemoryDropExpr>(expr)));
      break;

    case ExprType::MemoryFill:
      CHECK_RESULT(delegate_->OnMemoryFillExpr(cast<MemoryFillExpr>(expr)));
      break;

    case ExprType::MemoryGrow:
      CHECK_RESULT(delegate_->OnMemoryGrowExpr(cast<MemoryGrowExpr>(expr)));
      break;

    case ExprType::MemoryInit:
      CHECK_RESULT(delegate_->OnMemoryInitExpr(cast<MemoryInitExpr>(expr)));
      break;

    case ExprType::MemorySize:
      CHECK_RESULT(delegate_->OnMemorySizeExpr(cast<MemorySizeExpr>(expr)));
      break;

    case ExprType::TableCopy:
      CHECK_RESULT(delegate_->OnTableCopyExpr(cast<TableCopyExpr>(expr)));
      break;

    case ExprType::TableDrop:
      CHECK_RESULT(delegate_->OnTableDropExpr(cast<TableDropExpr>(expr)));
      break;

    case ExprType::TableInit:
      CHECK_RESULT(delegate_->OnTableInitExpr(cast<TableInitExpr>(expr)));
      break;

    case ExprType::Nop:
      CHECK_RESULT(delegate_->OnNopExpr(cast<NopExpr>(expr)));
      break;

    case ExprType::Rethrow:
      CHECK_RESULT(delegate_->OnRethrowExpr(cast<RethrowExpr>(expr)));
      break;

    case ExprType::Return:
      CHECK_RESULT(delegate_->OnReturnExpr(cast<ReturnExpr>(expr)));
      break;

    case ExprType::ReturnCall:
      CHECK_RESULT(delegate_->OnReturnCallExpr(cast<ReturnCallExpr>(expr)));
      break;

    case ExprType::ReturnCallIndirect:
      CHECK_RESULT(delegate_->OnReturnCallIndirectExpr(
          cast<ReturnCallIndirectExpr>(expr)));
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

    case ExprType::Try: {
      auto try_expr = cast<TryExpr>(expr);
      CHECK_RESULT(delegate_->BeginTryExpr(try_expr));
      PushExprlist(State::Try, expr, try_expr->block.exprs);
      break;
    }

    case ExprType::Unary:
      CHECK_RESULT(delegate_->OnUnaryExpr(cast<UnaryExpr>(expr)));
      break;

    case ExprType::Ternary:
      CHECK_RESULT(delegate_->OnTernaryExpr(cast<TernaryExpr>(expr)));
      break;

    case ExprType::SimdLaneOp: {
      CHECK_RESULT(delegate_->OnSimdLaneOpExpr(cast<SimdLaneOpExpr>(expr)));
      break;
    }

    case ExprType::SimdShuffleOp: {
      CHECK_RESULT(
          delegate_->OnSimdShuffleOpExpr(cast<SimdShuffleOpExpr>(expr)));
      break;
    }

    case ExprType::Unreachable:
      CHECK_RESULT(delegate_->OnUnreachableExpr(cast<UnreachableExpr>(expr)));
      break;
  }

  return Result::Ok;
}

void ExprVisitor::PushDefault(Expr* expr) {
  state_stack_.emplace_back(State::Default);
  expr_stack_.emplace_back(expr);
}

void ExprVisitor::PopDefault() {
  state_stack_.pop_back();
  expr_stack_.pop_back();
}

void ExprVisitor::PushExprlist(State state, Expr* expr, ExprList& expr_list) {
  state_stack_.emplace_back(state);
  expr_stack_.emplace_back(expr);
  expr_iter_stack_.emplace_back(expr_list.begin());
}

void ExprVisitor::PopExprlist() {
  state_stack_.pop_back();
  expr_stack_.pop_back();
  expr_iter_stack_.pop_back();
}

}  // namespace wabt

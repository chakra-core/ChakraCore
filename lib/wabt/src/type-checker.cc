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

#include "type-checker.h"

#define CHECK_RESULT(expr) \
  do {                     \
    if (WABT_FAILED(expr)) \
      return Result::Error;   \
  } while (0)

#define COMBINE_RESULT(result_var, result)                            \
  do {                                                                \
    (result_var) = static_cast<Result>(static_cast<int>(result_var) | \
                                       static_cast<int>(result));     \
  } while (0)

namespace wabt {

TypeCheckerLabel::TypeCheckerLabel(LabelType label_type,
                                   const TypeVector& sig,
                                   size_t limit)
    : label_type(label_type),
      sig(sig),
      type_stack_limit(limit),
      unreachable(false) {}

static void WABT_PRINTF_FORMAT(2, 3)
    print_error(TypeChecker* tc, const char* fmt, ...) {
  if (tc->error_handler->on_error) {
    WABT_SNPRINTF_ALLOCA(buffer, length, fmt);
    tc->error_handler->on_error(buffer, tc->error_handler->user_data);
  }
}

Result typechecker_get_label(TypeChecker* tc,
                             size_t depth,
                             TypeCheckerLabel** out_label) {
  if (depth >= tc->label_stack.size()) {
    assert(tc->label_stack.size() > 0);
    print_error(tc, "invalid depth: %" PRIzd " (max %" PRIzd ")", depth,
                tc->label_stack.size() - 1);
    *out_label = nullptr;
    return Result::Error;
  }
  *out_label = &tc->label_stack[tc->label_stack.size() - depth - 1];
  return Result::Ok;
}

static Result top_label(TypeChecker* tc, TypeCheckerLabel** out_label) {
  return typechecker_get_label(tc, 0, out_label);
}

bool typechecker_is_unreachable(TypeChecker* tc) {
  TypeCheckerLabel* label;
  if (WABT_FAILED(top_label(tc, &label)))
    return true;
  return label->unreachable;
}

static void reset_type_stack_to_label(TypeChecker* tc,
                                      TypeCheckerLabel* label) {
  tc->type_stack.resize(label->type_stack_limit);
}

static Result set_unreachable(TypeChecker* tc) {
  TypeCheckerLabel* label;
  CHECK_RESULT(top_label(tc, &label));
  label->unreachable = true;
  reset_type_stack_to_label(tc, label);
  return Result::Ok;
}

static void push_label(TypeChecker* tc,
                       LabelType label_type,
                       const TypeVector& sig) {
  tc->label_stack.emplace_back(label_type, sig, tc->type_stack.size());
}

static Result pop_label(TypeChecker* tc) {
  tc->label_stack.pop_back();
  return Result::Ok;
}

static Result check_label_type(TypeCheckerLabel* label, LabelType label_type) {
  return label->label_type == label_type ? Result::Ok : Result::Error;
}

static Result peek_type(TypeChecker* tc, uint32_t depth, Type* out_type) {
  TypeCheckerLabel* label;
  CHECK_RESULT(top_label(tc, &label));

  if (label->type_stack_limit + depth >= tc->type_stack.size()) {
    *out_type = Type::Any;
    return label->unreachable ? Result::Ok : Result::Error;
  }
  *out_type = tc->type_stack[tc->type_stack.size() - depth - 1];
  return Result::Ok;
}

static Result top_type(TypeChecker* tc, Type* out_type) {
  return peek_type(tc, 0, out_type);
}

static Result pop_type(TypeChecker* tc, Type* out_type) {
  TypeCheckerLabel* label;
  CHECK_RESULT(top_label(tc, &label));
  Result result = top_type(tc, out_type);
  if (tc->type_stack.size() > label->type_stack_limit)
    tc->type_stack.pop_back();
  return result;
}

static Result drop_types(TypeChecker* tc, size_t drop_count) {
  TypeCheckerLabel* label;
  CHECK_RESULT(top_label(tc, &label));
  if (label->type_stack_limit + drop_count > tc->type_stack.size()) {
    if (label->unreachable) {
      reset_type_stack_to_label(tc, label);
      return Result::Ok;
    }
    return Result::Error;
  }
  tc->type_stack.erase(tc->type_stack.end() - drop_count, tc->type_stack.end());
  return Result::Ok;
}

static void push_type(TypeChecker* tc, Type type) {
  if (type != Type::Void)
    tc->type_stack.push_back(type);
}

static void push_types(TypeChecker* tc, const TypeVector& types) {
  for (Type type: types)
    push_type(tc, type);
}

static Result check_type_stack_limit(TypeChecker* tc,
                                     size_t expected,
                                     const char* desc) {
  TypeCheckerLabel* label;
  CHECK_RESULT(top_label(tc, &label));
  size_t avail = tc->type_stack.size() - label->type_stack_limit;
  if (!label->unreachable && expected > avail) {
    print_error(tc, "type stack size too small at %s. got %" PRIzd
                    ", expected at least %" PRIzd,
                desc, avail, expected);
    return Result::Error;
  }
  return Result::Ok;
}

static Result check_type_stack_end(TypeChecker* tc, const char* desc) {
  TypeCheckerLabel* label;
  CHECK_RESULT(top_label(tc, &label));
  if (tc->type_stack.size() != label->type_stack_limit) {
    print_error(tc, "type stack at end of %s is %" PRIzd ", expected %" PRIzd,
                desc, tc->type_stack.size(), label->type_stack_limit);
    return Result::Error;
  }
  return Result::Ok;
}

static Result check_type(TypeChecker* tc,
                         Type actual,
                         Type expected,
                         const char* desc) {
  if (expected != actual && expected != Type::Any && actual != Type::Any) {
    print_error(tc, "type mismatch in %s, expected %s but got %s.", desc,
                get_type_name(expected), get_type_name(actual));
    return Result::Error;
  }
  return Result::Ok;
}

static Result check_signature(TypeChecker* tc,
                              const TypeVector& sig,
                              const char* desc) {
  Result result = Result::Ok;
  COMBINE_RESULT(result, check_type_stack_limit(tc, sig.size(), desc));
  for (size_t i = 0; i < sig.size(); ++i) {
    Type actual = Type::Any;
    COMBINE_RESULT(result, peek_type(tc, sig.size() - i - 1, &actual));
    COMBINE_RESULT(result, check_type(tc, actual, sig[i], desc));
  }
  return result;
}

static Result pop_and_check_signature(TypeChecker* tc,
                                      const TypeVector& sig,
                                      const char* desc) {
  Result result = Result::Ok;
  COMBINE_RESULT(result, check_signature(tc, sig, desc));
  COMBINE_RESULT(result, drop_types(tc, sig.size()));
  return result;
}

static Result pop_and_check_call(TypeChecker* tc,
                                 const TypeVector& param_types,
                                 const TypeVector& result_types,
                                 const char* desc) {
  Result result = Result::Ok;
  COMBINE_RESULT(result, check_type_stack_limit(tc, param_types.size(), desc));
  for (size_t i = 0; i < param_types.size(); ++i) {
    Type actual = Type::Any;
    COMBINE_RESULT(result, peek_type(tc, param_types.size() - i - 1, &actual));
    COMBINE_RESULT(result, check_type(tc, actual, param_types[i], desc));
  }
  COMBINE_RESULT(result, drop_types(tc, param_types.size()));
  push_types(tc, result_types);
  return result;
}

static Result pop_and_check_1_type(TypeChecker* tc,
                                   Type expected,
                                   const char* desc) {
  Result result = Result::Ok;
  Type actual = Type::Any;
  COMBINE_RESULT(result, check_type_stack_limit(tc, 1, desc));
  COMBINE_RESULT(result, pop_type(tc, &actual));
  COMBINE_RESULT(result, check_type(tc, actual, expected, desc));
  return result;
}

static Result pop_and_check_2_types(TypeChecker* tc,
                                    Type expected1,
                                    Type expected2,
                                    const char* desc) {
  Result result = Result::Ok;
  Type actual1 = Type::Any;
  Type actual2 = Type::Any;
  COMBINE_RESULT(result, check_type_stack_limit(tc, 2, desc));
  COMBINE_RESULT(result, pop_type(tc, &actual2));
  COMBINE_RESULT(result, pop_type(tc, &actual1));
  COMBINE_RESULT(result, check_type(tc, actual1, expected1, desc));
  COMBINE_RESULT(result, check_type(tc, actual2, expected2, desc));
  return result;
}

static Result pop_and_check_2_types_are_equal(TypeChecker* tc,
                                              Type* out_type,
                                              const char* desc) {
  Result result = Result::Ok;
  Type right = Type::Any;
  Type left = Type::Any;
  COMBINE_RESULT(result, check_type_stack_limit(tc, 2, desc));
  COMBINE_RESULT(result, pop_type(tc, &right));
  COMBINE_RESULT(result, pop_type(tc, &left));
  COMBINE_RESULT(result, check_type(tc, left, right, desc));
  *out_type = right;
  return result;
}

static Result check_opcode1(TypeChecker* tc, Opcode opcode) {
  Result result = pop_and_check_1_type(tc, get_opcode_param_type_1(opcode),
                                       get_opcode_name(opcode));
  push_type(tc, get_opcode_result_type(opcode));
  return result;
}

static Result check_opcode2(TypeChecker* tc, Opcode opcode) {
  Result result = pop_and_check_2_types(tc, get_opcode_param_type_1(opcode),
                                        get_opcode_param_type_2(opcode),
                                        get_opcode_name(opcode));
  push_type(tc, get_opcode_result_type(opcode));
  return result;
}

Result typechecker_begin_function(TypeChecker* tc, const TypeVector* sig) {
  tc->type_stack.clear();
  tc->label_stack.clear();
  push_label(tc, LabelType::Func, *sig);
  return Result::Ok;
}

Result typechecker_on_binary(TypeChecker* tc, Opcode opcode) {
  return check_opcode2(tc, opcode);
}

Result typechecker_on_block(TypeChecker* tc, const TypeVector* sig) {
  push_label(tc, LabelType::Block, *sig);
  return Result::Ok;
}

Result typechecker_on_br(TypeChecker* tc, size_t depth) {
  Result result = Result::Ok;
  TypeCheckerLabel* label;
  CHECK_RESULT(typechecker_get_label(tc, depth, &label));
  if (label->label_type != LabelType::Loop)
    COMBINE_RESULT(result, check_signature(tc, label->sig, "br"));
  CHECK_RESULT(set_unreachable(tc));
  return result;
}

Result typechecker_on_br_if(TypeChecker* tc, size_t depth) {
  Result result = Result::Ok;
  COMBINE_RESULT(result, pop_and_check_1_type(tc, Type::I32, "br_if"));
  TypeCheckerLabel* label;
  CHECK_RESULT(typechecker_get_label(tc, depth, &label));
  if (label->label_type != LabelType::Loop)
    COMBINE_RESULT(result, check_signature(tc, label->sig, "br_if"));
  return result;
}

Result typechecker_begin_br_table(TypeChecker* tc) {
  tc->br_table_sig = Type::Any;
  return pop_and_check_1_type(tc, Type::I32, "br_table");
}

Result typechecker_on_br_table_target(TypeChecker* tc, size_t depth) {
  Result result = Result::Ok;
  TypeCheckerLabel* label;
  CHECK_RESULT(typechecker_get_label(tc, depth, &label));
  assert(label->sig.size() <= 1);
  Type label_sig = label->sig.size() == 0 ? Type::Void : label->sig[0];
  COMBINE_RESULT(result,
                 check_type(tc, tc->br_table_sig, label_sig, "br_table"));
  tc->br_table_sig = label_sig;

  if (label->label_type != LabelType::Loop)
    COMBINE_RESULT(result, check_signature(tc, label->sig, "br_table"));
  return result;
}

Result typechecker_end_br_table(TypeChecker* tc) {
  return set_unreachable(tc);
}

Result typechecker_on_call(TypeChecker* tc,
                           const TypeVector* param_types,
                           const TypeVector* result_types) {
  return pop_and_check_call(tc, *param_types, *result_types, "call");
}

Result typechecker_on_call_indirect(TypeChecker* tc,
                                    const TypeVector* param_types,
                                    const TypeVector* result_types) {
  Result result = Result::Ok;
  COMBINE_RESULT(result, pop_and_check_1_type(tc, Type::I32, "call_indirect"));
  COMBINE_RESULT(result, pop_and_check_call(tc, *param_types, *result_types,
                                            "call_indirect"));
  return result;
}

Result typechecker_on_compare(TypeChecker* tc, Opcode opcode) {
  return check_opcode2(tc, opcode);
}

Result typechecker_on_const(TypeChecker* tc, Type type) {
  push_type(tc, type);
  return Result::Ok;
}

Result typechecker_on_convert(TypeChecker* tc, Opcode opcode) {
  return check_opcode1(tc, opcode);
}

Result typechecker_on_current_memory(TypeChecker* tc) {
  push_type(tc, Type::I32);
  return Result::Ok;
}

Result typechecker_on_drop(TypeChecker* tc) {
  Result result = Result::Ok;
  Type type = Type::Any;
  COMBINE_RESULT(result, check_type_stack_limit(tc, 1, "drop"));
  COMBINE_RESULT(result, pop_type(tc, &type));
  return result;
}

Result typechecker_on_else(TypeChecker* tc) {
  Result result = Result::Ok;
  TypeCheckerLabel* label;
  CHECK_RESULT(top_label(tc, &label));
  COMBINE_RESULT(result, check_label_type(label, LabelType::If));
  COMBINE_RESULT(result,
                 pop_and_check_signature(tc, label->sig, "if true branch"));
  COMBINE_RESULT(result, check_type_stack_end(tc, "if true branch"));
  reset_type_stack_to_label(tc, label);
  label->label_type = LabelType::Else;
  label->unreachable = false;
  return result;
}

static Result on_end(TypeChecker* tc,
                     TypeCheckerLabel* label,
                     const char* sig_desc,
                     const char* end_desc) {
  Result result = Result::Ok;
  COMBINE_RESULT(result, pop_and_check_signature(tc, label->sig, sig_desc));
  COMBINE_RESULT(result, check_type_stack_end(tc, end_desc));
  reset_type_stack_to_label(tc, label);
  push_types(tc, label->sig);
  pop_label(tc);
  return result;
}

Result typechecker_on_end(TypeChecker* tc) {
  Result result = Result::Ok;
  static const char* s_label_type_name[] = {"function", "block", "loop", "if",
                                            "if false branch"};
  WABT_STATIC_ASSERT(WABT_ARRAY_SIZE(s_label_type_name) == kLabelTypeCount);
  TypeCheckerLabel* label;
  CHECK_RESULT(top_label(tc, &label));
  assert(static_cast<int>(label->label_type) < kLabelTypeCount);
  if (label->label_type == LabelType::If) {
    if (label->sig.size() != 0) {
      print_error(tc, "if without else cannot have type signature.");
      result = Result::Error;
    }
  }
  const char* desc = s_label_type_name[static_cast<int>(label->label_type)];
  COMBINE_RESULT(result, on_end(tc, label, desc, desc));
  return result;
}

Result typechecker_on_grow_memory(TypeChecker* tc) {
  return check_opcode1(tc, Opcode::GrowMemory);
}

Result typechecker_on_if(TypeChecker* tc, const TypeVector* sig) {
  Result result = pop_and_check_1_type(tc, Type::I32, "if");
  push_label(tc, LabelType::If, *sig);
  return result;
}

Result typechecker_on_get_global(TypeChecker* tc, Type type) {
  push_type(tc, type);
  return Result::Ok;
}

Result typechecker_on_get_local(TypeChecker* tc, Type type) {
  push_type(tc, type);
  return Result::Ok;
}

Result typechecker_on_load(TypeChecker* tc, Opcode opcode) {
  return check_opcode1(tc, opcode);
}

Result typechecker_on_loop(TypeChecker* tc, const TypeVector* sig) {
  push_label(tc, LabelType::Loop, *sig);
  return Result::Ok;
}

Result typechecker_on_return(TypeChecker* tc) {
  Result result = Result::Ok;
  TypeCheckerLabel* func_label;
  CHECK_RESULT(
      typechecker_get_label(tc, tc->label_stack.size() - 1, &func_label));
  COMBINE_RESULT(result,
                 pop_and_check_signature(tc, func_label->sig, "return"));
  CHECK_RESULT(set_unreachable(tc));
  return result;
}

Result typechecker_on_select(TypeChecker* tc) {
  Result result = Result::Ok;
  COMBINE_RESULT(result, pop_and_check_1_type(tc, Type::I32, "select"));
  Type type = Type::Any;
  COMBINE_RESULT(result, pop_and_check_2_types_are_equal(tc, &type, "select"));
  push_type(tc, type);
  return result;
}

Result typechecker_on_set_global(TypeChecker* tc, Type type) {
  return pop_and_check_1_type(tc, type, "set_global");
}

Result typechecker_on_set_local(TypeChecker* tc, Type type) {
  return pop_and_check_1_type(tc, type, "set_local");
}

Result typechecker_on_store(TypeChecker* tc, Opcode opcode) {
  return check_opcode2(tc, opcode);
}

Result typechecker_on_tee_local(TypeChecker* tc, Type type) {
  Result result = Result::Ok;
  Type value = Type::Any;
  COMBINE_RESULT(result, check_type_stack_limit(tc, 1, "tee_local"));
  COMBINE_RESULT(result, top_type(tc, &value));
  COMBINE_RESULT(result, check_type(tc, value, type, "tee_local"));
  return result;
}

Result typechecker_on_unary(TypeChecker* tc, Opcode opcode) {
  return check_opcode1(tc, opcode);
}

Result typechecker_on_unreachable(TypeChecker* tc) {
  return set_unreachable(tc);
}

Result typechecker_end_function(TypeChecker* tc) {
  Result result = Result::Ok;
  TypeCheckerLabel* label;
  CHECK_RESULT(top_label(tc, &label));
  COMBINE_RESULT(result, check_label_type(label, LabelType::Func));
  COMBINE_RESULT(result, on_end(tc, label, "implicit return", "function"));
  return result;
}

}  // namespace wabt

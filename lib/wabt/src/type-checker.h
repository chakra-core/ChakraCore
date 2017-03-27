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

#include <vector>

#include "common.h"

namespace wabt {

typedef void (*TypeCheckerErrorCallback)(const char* msg, void* user_data);

struct TypeCheckerErrorHandler {
  TypeCheckerErrorCallback on_error;
  void* user_data;
};

struct TypeCheckerLabel {
  TypeCheckerLabel(LabelType, const TypeVector& sig, size_t limit);

  LabelType label_type;
  TypeVector sig;
  size_t type_stack_limit;
  bool unreachable;
};

struct TypeChecker {
  TypeCheckerErrorHandler* error_handler = nullptr;
  TypeVector type_stack;
  std::vector<TypeCheckerLabel> label_stack;
  /* TODO(binji): will need to be complete signature when signatures with
   * multiple types are allowed. */
  Type br_table_sig;
};

bool typechecker_is_unreachable(TypeChecker* tc);
Result typechecker_get_label(TypeChecker* tc,
                             size_t depth,
                             TypeCheckerLabel** out_label);

Result typechecker_begin_function(TypeChecker*, const TypeVector* sig);
Result typechecker_on_binary(TypeChecker*, Opcode);
Result typechecker_on_block(TypeChecker*, const TypeVector* sig);
Result typechecker_on_br(TypeChecker*, size_t depth);
Result typechecker_on_br_if(TypeChecker*, size_t depth);
Result typechecker_begin_br_table(TypeChecker*);
Result typechecker_on_br_table_target(TypeChecker*, size_t depth);
Result typechecker_end_br_table(TypeChecker*);
Result typechecker_on_call(TypeChecker*,
                           const TypeVector* param_types,
                           const TypeVector* result_types);
Result typechecker_on_call_indirect(TypeChecker*,
                                    const TypeVector* param_types,
                                    const TypeVector* result_types);
Result typechecker_on_compare(TypeChecker*, Opcode);
Result typechecker_on_const(TypeChecker*, Type);
Result typechecker_on_convert(TypeChecker*, Opcode);
Result typechecker_on_current_memory(TypeChecker*);
Result typechecker_on_drop(TypeChecker*);
Result typechecker_on_else(TypeChecker*);
Result typechecker_on_end(TypeChecker*);
Result typechecker_on_get_global(TypeChecker*, Type);
Result typechecker_on_get_local(TypeChecker*, Type);
Result typechecker_on_grow_memory(TypeChecker*);
Result typechecker_on_if(TypeChecker*, const TypeVector* sig);
Result typechecker_on_load(TypeChecker*, Opcode);
Result typechecker_on_loop(TypeChecker*, const TypeVector* sig);
Result typechecker_on_return(TypeChecker*);
Result typechecker_on_select(TypeChecker*);
Result typechecker_on_set_global(TypeChecker*, Type);
Result typechecker_on_set_local(TypeChecker*, Type);
Result typechecker_on_store(TypeChecker*, Opcode);
Result typechecker_on_tee_local(TypeChecker*, Type);
Result typechecker_on_unary(TypeChecker*, Opcode);
Result typechecker_on_unreachable(TypeChecker*);
Result typechecker_end_function(TypeChecker*);

}  // namespace wabt

#endif /* WABT_TYPE_CHECKER_H_ */

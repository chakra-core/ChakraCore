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

#ifndef WABT_AST_PARSER_LEXER_SHARED_H_
#define WABT_AST_PARSER_LEXER_SHARED_H_

#include <stdarg.h>

#include "ast.h"
#include "ast-lexer.h"
#include "common.h"

#define WABT_AST_PARSER_STYPE Token
#define WABT_AST_PARSER_LTYPE Location
#define YYSTYPE WABT_AST_PARSER_STYPE
#define YYLTYPE WABT_AST_PARSER_LTYPE

#define WABT_INVALID_LINE_OFFSET (static_cast<size_t>(~0))

namespace wabt {

struct ExprList {
  Expr* first;
  Expr* last;
  size_t size;
};

struct TextListNode {
  StringSlice text;
  struct TextListNode* next;
};

struct TextList {
  TextListNode* first;
  TextListNode* last;
};

struct OptionalExport {
  Export export_;
  bool has_export;
};

struct ExportedFunc {
  Func* func;
  OptionalExport export_;
};

struct ExportedGlobal {
  Global global;
  OptionalExport export_;
};

struct ExportedTable {
  Table table;
  ElemSegment elem_segment;
  OptionalExport export_;
  bool has_elem_segment;
};

struct ExportedMemory {
  Memory memory;
  DataSegment data_segment;
  OptionalExport export_;
  bool has_data_segment;
};

enum class FuncFieldType {
  Exprs,
  ParamTypes,
  BoundParam,
  ResultTypes,
  LocalTypes,
  BoundLocal,
};

struct BoundType {
  Location loc;
  StringSlice name;
  Type type;
};

struct FuncField {
  FuncFieldType type;
  union {
    Expr* first_expr;     /* WABT_FUNC_FIELD_TYPE_EXPRS */
    TypeVector types;     /* WABT_FUNC_FIELD_TYPE_*_TYPES */
    BoundType bound_type; /* WABT_FUNC_FIELD_TYPE_BOUND_{LOCAL, PARAM} */
  };
  struct FuncField* next;
};

union Token {
  /* terminals */
  StringSlice text;
  Type type;
  Opcode opcode;
  Literal literal;

  /* non-terminals */
  /* some of these use pointers to keep the size of Token down; copying the
   tokens is a hotspot when parsing large files. */
  Action action;
  Block block;
  Command* command;
  CommandVector commands;
  Const const_;
  ConstVector consts;
  DataSegment data_segment;
  ElemSegment elem_segment;
  Export export_;
  ExportedFunc exported_func;
  ExportedGlobal exported_global;
  ExportedMemory exported_memory;
  ExportedTable exported_table;
  Expr* expr;
  ExprList expr_list;
  FuncField* func_fields;
  Func* func;
  FuncSignature func_sig;
  FuncType func_type;
  Global global;
  Import* import;
  Limits limits;
  OptionalExport optional_export;
  Memory memory;
  Module* module;
  RawModule raw_module;
  Script script;
  Table table;
  TextList text_list;
  TypeVector types;
  uint32_t u32;
  uint64_t u64;
  Var var;
  VarVector vars;
};

struct AstParser {
  Script script;
  SourceErrorHandler* error_handler;
  int errors;
  /* Cached pointers to reallocated parser buffers, so they don't leak. */
  int16_t* yyssa;
  YYSTYPE* yyvsa;
  YYLTYPE* yylsa;
};

int ast_lexer_lex(union Token*, struct Location*, AstLexer*, struct AstParser*);
Result ast_lexer_get_source_line(AstLexer*,
                                 const struct Location*,
                                 size_t line_max_length,
                                 char* line,
                                 size_t* out_line_length,
                                 int* out_column_offset);
void WABT_PRINTF_FORMAT(4, 5) ast_parser_error(struct Location*,
                                               AstLexer*,
                                               struct AstParser*,
                                               const char*,
                                               ...);
void ast_format_error(SourceErrorHandler*,
                      const struct Location*,
                      AstLexer*,
                      const char* format,
                      va_list);
void destroy_optional_export(OptionalExport*);
void destroy_exported_func(ExportedFunc*);
void destroy_exported_global(ExportedFunc*);
void destroy_exported_memory(ExportedMemory*);
void destroy_exported_table(ExportedTable*);
void destroy_func_fields(FuncField*);
void destroy_text_list(TextList*);

}  // namespace wabt

#endif /* WABT_AST_PARSER_LEXER_SHARED_H_ */

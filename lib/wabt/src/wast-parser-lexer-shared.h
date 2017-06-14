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

#ifndef WABT_WAST_PARSER_LEXER_SHARED_H_
#define WABT_WAST_PARSER_LEXER_SHARED_H_

#include <cstdarg>
#include <memory>

#include "common.h"
#include "ir.h"
#include "source-error-handler.h"
#include "wast-parser.h"

#define WABT_WAST_PARSER_STYPE Token
#define WABT_WAST_PARSER_LTYPE Location
#define YYSTYPE WABT_WAST_PARSER_STYPE
#define YYLTYPE WABT_WAST_PARSER_LTYPE

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

struct ModuleFieldList {
  ModuleField* first;
  ModuleField* last;
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
  Action* action;
  Block* block;
  Command* command;
  CommandPtrVector* commands;
  Const const_;
  ConstVector* consts;
  DataSegment* data_segment;
  ElemSegment* elem_segment;
  Exception* exception;
  Export* export_;
  Expr* expr;
  ExprList expr_list;
  Func* func;
  FuncSignature* func_sig;
  FuncType* func_type;
  Global* global;
  Import* import;
  Limits limits;
  Memory* memory;
  Module* module;
  ModuleField* module_field;
  ModuleFieldList module_fields;
  ScriptModule* script_module;
  Script* script;
  Table* table;
  TextList text_list;
  TypeVector* types;
  uint32_t u32;
  uint64_t u64;
  Var* var;
  VarVector* vars;
};

struct WastParser {
  Script* script;
  SourceErrorHandler* error_handler;
  int errors;
  /* Cached pointers to reallocated parser buffers, so they don't leak. */
  int16_t* yyssa;
  YYSTYPE* yyvsa;
  YYLTYPE* yylsa;
  WastParseOptions* options;
};

int wast_lexer_lex(union Token*,
                   struct Location*,
                   WastLexer*,
                   struct WastParser*);
void WABT_PRINTF_FORMAT(4, 5) wast_parser_error(struct Location*,
                                                WastLexer*,
                                                struct WastParser*,
                                                const char*,
                                                ...);
void wast_format_error(SourceErrorHandler*,
                       const struct Location*,
                       WastLexer*,
                       const char* format,
                       va_list);
void destroy_text_list(TextList*);
void destroy_module_field_list(ModuleFieldList*);

}  // namespace wabt

#endif /* WABT_WAST_PARSER_LEXER_SHARED_H_ */

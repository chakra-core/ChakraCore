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

%{
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast-parser.h"
#include "ast-parser-lexer-shared.h"
#include "binary-reader-ast.h"
#include "binary-reader.h"
#include "literal.h"

#define INVALID_VAR_INDEX (-1)

#define RELOCATE_STACK(type, array, stack_base, old_byte_size, new_size) \
  do {                                                                   \
    type* new_stack = new type[new_size]();                              \
    memcpy((new_stack), (stack_base), old_byte_size);                    \
    if ((stack_base) != (array)) {                                       \
      delete[](stack_base);                                              \
    }                                                                    \
    /* Cache the pointer in the parser struct to be deleted later. */    \
    parser->array = (stack_base) = new_stack;                            \
  } while (0)

#define yyoverflow(message, ss, ss_size, vs, vs_size, ls, ls_size, new_size) \
  do {                                                                       \
    *(new_size) *= 2;                                                        \
    RELOCATE_STACK(yytype_int16, yyssa, *(ss), (ss_size), *(new_size));      \
    RELOCATE_STACK(YYSTYPE, yyvsa, *(vs), (vs_size), *(new_size));           \
    RELOCATE_STACK(YYLTYPE, yylsa, *(ls), (ls_size), *(new_size));           \
  } while (0)

#define DUPTEXT(dst, src)                                \
  (dst).start = wabt_strndup((src).start, (src).length); \
  (dst).length = (src).length

#define YYLLOC_DEFAULT(Current, Rhs, N)                       \
  do                                                          \
    if (N) {                                                  \
      (Current).filename = YYRHSLOC(Rhs, 1).filename;         \
      (Current).line = YYRHSLOC(Rhs, 1).line;                 \
      (Current).first_column = YYRHSLOC(Rhs, 1).first_column; \
      (Current).last_column = YYRHSLOC(Rhs, N).last_column;   \
    } else {                                                  \
      (Current).filename = nullptr;                           \
      (Current).line = YYRHSLOC(Rhs, 0).line;                 \
      (Current).first_column = (Current).last_column =        \
          YYRHSLOC(Rhs, 0).last_column;                       \
    }                                                         \
  while (0)

#define APPEND_FIELD_TO_LIST(module, field, Kind, kind, loc_, item) \
  do {                                                              \
    field = append_module_field(module);                            \
    field->loc = loc_;                                              \
    field->type = ModuleFieldType::Kind;                            \
    field->kind = item;                                             \
  } while (0)

#define APPEND_ITEM_TO_VECTOR(module, Kind, kind, kinds, item_ptr) \
  do {                                                             \
    Kind* dummy = item_ptr;                                        \
    append_##kind##_ptr_value(&(module)->kinds, &dummy);           \
  } while (0)

#define INSERT_BINDING(module, kind, kinds, loc_, name)                       \
  do                                                                          \
    if ((name).start) {                                                       \
      Binding* binding = insert_binding(&(module)->kind##_bindings, &(name)); \
      binding->loc = loc_;                                                    \
      binding->index = (module)->kinds.size - 1;                              \
    }                                                                         \
  while (0)

#define APPEND_INLINE_EXPORT(module, Kind, loc_, value, index_)         \
  do                                                                    \
    if ((value).export_.has_export) {                                   \
      ModuleField* export_field;                                        \
      APPEND_FIELD_TO_LIST(module, export_field, Export, export_, loc_, \
                           (value).export_.export_);                    \
      export_field->export_.kind = ExternalKind::Kind;                  \
      export_field->export_.var.loc = loc_;                             \
      export_field->export_.var.index = index_;                         \
      APPEND_ITEM_TO_VECTOR(module, Export, export, exports,            \
                            &export_field->export_);                    \
      INSERT_BINDING(module, export, exports, export_field->loc,        \
                     export_field->export_.name);                       \
    }                                                                   \
  while (0)

#define CHECK_IMPORT_ORDERING(module, kind, kinds, loc_)           \
  do {                                                             \
    if ((module)->kinds.size != (module)->num_##kind##_imports) {  \
      ast_parser_error(                                            \
          &loc_, lexer, parser,                                    \
          "imports must occur before all non-import definitions"); \
    }                                                              \
  } while (0)

#define CHECK_END_LABEL(loc, begin_label, end_label)                       \
  do {                                                                     \
    if (!string_slice_is_empty(&(end_label))) {                            \
      if (string_slice_is_empty(&(begin_label))) {                         \
        ast_parser_error(&loc, lexer, parser,                              \
                         "unexpected label \"" PRIstringslice "\"",        \
                         WABT_PRINTF_STRING_SLICE_ARG(end_label));         \
      } else if (!string_slices_are_equal(&(begin_label), &(end_label))) { \
        ast_parser_error(&loc, lexer, parser,                              \
                         "mismatching label \"" PRIstringslice             \
                         "\" != \"" PRIstringslice "\"",                   \
                         WABT_PRINTF_STRING_SLICE_ARG(begin_label),        \
                         WABT_PRINTF_STRING_SLICE_ARG(end_label));         \
      }                                                                    \
      destroy_string_slice(&(end_label));                                  \
    }                                                                      \
  } while (0)

#define YYMALLOC(size) new char [size]
#define YYFREE(p) delete [] (p)

#define USE_NATURAL_ALIGNMENT (~0)

namespace wabt {

ExprList join_exprs1(Location* loc, Expr* expr1);
ExprList join_exprs2(Location* loc,
                         ExprList* expr1,
                         Expr* expr2);

FuncField* new_func_field(void) { return new FuncField(); }
Func* new_func(void) { return new Func(); }
Command* new_command(void) { return new Command(); }
Module* new_module(void) { return new Module(); }
Import* new_import(void) { return new Import(); }
TextListNode* new_text_list_node(void) { return new TextListNode(); }

Result parse_const(Type type,
                       LiteralType literal_type,
                       const char* s,
                       const char* end,
                       Const* out);
void dup_text_list(TextList* text_list, char** out_data, size_t* out_size);

bool is_empty_signature(FuncSignature* sig);

void append_implicit_func_declaration(Location*,
                                      Module*,
                                      FuncDeclaration*);

struct BinaryErrorCallbackData {
  Location* loc;
  AstLexer* lexer;
  AstParser* parser;
};

static void on_read_binary_error(uint32_t offset, const char* error,
                                 void* user_data);

#define wabt_ast_parser_lex ast_lexer_lex
#define wabt_ast_parser_error ast_parser_error

%}

%define api.prefix {wabt_ast_parser_}
%define api.pure true
%define api.value.type {::wabt::Token}
%define api.token.prefix {WABT_TOKEN_TYPE_}
%define parse.error verbose
%lex-param {::wabt::AstLexer* lexer} {::wabt::AstParser* parser}
%parse-param {::wabt::AstLexer* lexer} {::wabt::AstParser* parser}
%locations

%token LPAR "("
%token RPAR ")"
%token NAT INT FLOAT TEXT VAR VALUE_TYPE ANYFUNC MUT
%token NOP DROP BLOCK END IF THEN ELSE LOOP BR BR_IF BR_TABLE
%token CALL CALL_IMPORT CALL_INDIRECT RETURN
%token GET_LOCAL SET_LOCAL TEE_LOCAL GET_GLOBAL SET_GLOBAL
%token LOAD STORE OFFSET_EQ_NAT ALIGN_EQ_NAT
%token CONST UNARY BINARY COMPARE CONVERT SELECT
%token UNREACHABLE CURRENT_MEMORY GROW_MEMORY
%token FUNC START TYPE PARAM RESULT LOCAL GLOBAL
%token MODULE TABLE ELEM MEMORY DATA OFFSET IMPORT EXPORT
%token REGISTER INVOKE GET
%token ASSERT_MALFORMED ASSERT_INVALID ASSERT_UNLINKABLE
%token ASSERT_RETURN ASSERT_RETURN_NAN ASSERT_TRAP ASSERT_EXHAUSTION
%token INPUT OUTPUT
%token EOF 0 "EOF"

%type<opcode> BINARY COMPARE CONVERT LOAD STORE UNARY
%type<text> ALIGN_EQ_NAT OFFSET_EQ_NAT TEXT VAR
%type<type> SELECT
%type<type> CONST VALUE_TYPE
%type<literal> NAT INT FLOAT

%type<action> action
%type<block> block
%type<command> assertion cmd
%type<commands> cmd_list
%type<const_> const
%type<consts> const_list
%type<data_segment> data
%type<elem_segment> elem
%type<export_> export export_kind
%type<exported_func> func
%type<exported_global> global
%type<exported_table> table
%type<exported_memory> memory
%type<expr> plain_instr block_instr
%type<expr_list> instr instr_list expr expr1 expr_list if_ const_expr offset
%type<func_fields> func_fields func_body
%type<func> func_info
%type<func_sig> func_sig func_type
%type<func_type> type_def
%type<global> global_type
%type<import> import import_kind inline_import
%type<limits> limits
%type<memory> memory_sig
%type<module> module module_fields
%type<optional_export> inline_export inline_export_opt
%type<raw_module> raw_module
%type<literal> literal
%type<script> script
%type<table> table_sig
%type<text> bind_var bind_var_opt labeling_opt quoted_text
%type<text_list> non_empty_text_list text_list
%type<types> value_type_list
%type<u32> align_opt
%type<u64> nat offset_opt
%type<vars> var_list
%type<var> start type_use var script_var_opt

/* These non-terminals use the types below that have destructors, but the
 * memory is shared with the lexer, so should not be destroyed. */
%destructor {} ALIGN_EQ_NAT OFFSET_EQ_NAT TEXT VAR NAT INT FLOAT
%destructor { destroy_block(&$$); } <block>
%destructor { destroy_command($$); delete $$; } <command>
%destructor { destroy_command_vector_and_elements(&$$); } <commands>
%destructor { destroy_const_vector(&$$); } <consts>
%destructor { destroy_elem_segment(&$$); } <elem_segment>
%destructor { destroy_export(&$$); } <export_>
%destructor { destroy_exported_func(&$$); } <exported_func>
%destructor { destroy_exported_memory(&$$); } <exported_memory>
%destructor { destroy_exported_table(&$$); } <exported_table>
%destructor { destroy_expr($$); } <expr>
%destructor { destroy_expr_list($$.first); } <expr_list>
%destructor { destroy_func_fields($$); } <func_fields>
%destructor { destroy_func($$); delete $$; } <func>
%destructor { destroy_func_signature(&$$); } <func_sig>
%destructor { destroy_func_type(&$$); } <func_type>
%destructor { destroy_import($$); delete $$; } <import>
%destructor { destroy_data_segment(&$$); } <data_segment>
%destructor { destroy_module($$); delete $$; } <module>
%destructor { destroy_raw_module(&$$); } <raw_module>
%destructor { destroy_string_slice(&$$.text); } <literal>
%destructor { destroy_script(&$$); } <script>
%destructor { destroy_string_slice(&$$); } <text>
%destructor { destroy_text_list(&$$); } <text_list>
%destructor { destroy_type_vector(&$$); } <types>
%destructor { destroy_var_vector_and_elements(&$$); } <vars>
%destructor { destroy_var(&$$); } <var>


%nonassoc LOW
%nonassoc VAR

%start script_start

%%

/* Auxiliaries */

non_empty_text_list :
    TEXT {
      TextListNode* node = new_text_list_node();
      DUPTEXT(node->text, $1);
      node->next = nullptr;
      $$.first = $$.last = node;
    }
  | non_empty_text_list TEXT {
      $$ = $1;
      TextListNode* node = new_text_list_node();
      DUPTEXT(node->text, $2);
      node->next = nullptr;
      $$.last->next = node;
      $$.last = node;
    }
;
text_list :
    /* empty */ { $$.first = $$.last = nullptr; }
  | non_empty_text_list
;

quoted_text :
    TEXT {
      TextListNode node;
      node.text = $1;
      node.next = nullptr;
      TextList text_list;
      text_list.first = &node;
      text_list.last = &node;
      char* data;
      size_t size;
      dup_text_list(&text_list, &data, &size);
      $$.start = data;
      $$.length = size;
    }
;

/* Types */

value_type_list :
    /* empty */ { WABT_ZERO_MEMORY($$); }
  | value_type_list VALUE_TYPE {
      $$ = $1;
      append_type_value(&$$, &$2);
    }
;
elem_type :
    ANYFUNC {}
;
global_type :
    VALUE_TYPE {
      WABT_ZERO_MEMORY($$);
      $$.type = $1;
      $$.mutable_ = false;
    }
  | LPAR MUT VALUE_TYPE RPAR {
      WABT_ZERO_MEMORY($$);
      $$.type = $3;
      $$.mutable_ = true;
    }
;
func_type :
    LPAR FUNC func_sig RPAR { $$ = $3; }
;
func_sig :
    /* empty */ { WABT_ZERO_MEMORY($$); }
  | LPAR PARAM value_type_list RPAR {
      WABT_ZERO_MEMORY($$);
      $$.param_types = $3;
    }
  | LPAR PARAM value_type_list RPAR LPAR RESULT value_type_list RPAR {
      WABT_ZERO_MEMORY($$);
      $$.param_types = $3;
      $$.result_types = $7;
    }
  | LPAR RESULT value_type_list RPAR {
      WABT_ZERO_MEMORY($$);
      $$.result_types = $3;
    }
;

table_sig :
    limits elem_type { $$.elem_limits = $1; }
;
memory_sig :
    limits { $$.page_limits = $1; }
;
limits :
    nat {
      $$.has_max = false;
      $$.initial = $1;
      $$.max = 0;
    }
  | nat nat {
      $$.has_max = true;
      $$.initial = $1;
      $$.max = $2;
    }
;
type_use :
    LPAR TYPE var RPAR { $$ = $3; }
;

/* Expressions */

nat :
    NAT {
      if (WABT_FAILED(parse_uint64($1.text.start,
                                        $1.text.start + $1.text.length, &$$))) {
        ast_parser_error(&@1, lexer, parser,
                              "invalid int " PRIstringslice "\"",
                              WABT_PRINTF_STRING_SLICE_ARG($1.text));
      }
    }
;

literal :
    NAT {
      $$.type = $1.type;
      DUPTEXT($$.text, $1.text);
    }
  | INT {
      $$.type = $1.type;
      DUPTEXT($$.text, $1.text);
    }
  | FLOAT {
      $$.type = $1.type;
      DUPTEXT($$.text, $1.text);
    }
;

var :
    nat {
      $$.loc = @1;
      $$.type = VarType::Index;
      $$.index = $1;
    }
  | VAR {
      $$.loc = @1;
      $$.type = VarType::Name;
      DUPTEXT($$.name, $1);
    }
;
var_list :
    /* empty */ { WABT_ZERO_MEMORY($$); }
  | var_list var {
      $$ = $1;
      append_var_value(&$$, &$2);
    }
;
bind_var_opt :
    /* empty */ { WABT_ZERO_MEMORY($$); }
  | bind_var
;
bind_var :
    VAR { DUPTEXT($$, $1); }
;

labeling_opt :
    /* empty */ %prec LOW { WABT_ZERO_MEMORY($$); }
  | bind_var
;

offset_opt :
    /* empty */ { $$ = 0; }
  | OFFSET_EQ_NAT {
    if (WABT_FAILED(parse_int64($1.start, $1.start + $1.length, &$$,
                                ParseIntType::SignedAndUnsigned))) {
      ast_parser_error(&@1, lexer, parser,
                            "invalid offset \"" PRIstringslice "\"",
                            WABT_PRINTF_STRING_SLICE_ARG($1));
      }
    }
;
align_opt :
    /* empty */ { $$ = USE_NATURAL_ALIGNMENT; }
  | ALIGN_EQ_NAT {
    if (WABT_FAILED(parse_int32($1.start, $1.start + $1.length, &$$,
                                ParseIntType::UnsignedOnly))) {
      ast_parser_error(&@1, lexer, parser,
                       "invalid alignment \"" PRIstringslice "\"",
                       WABT_PRINTF_STRING_SLICE_ARG($1));
      }
    }
;

instr :
    plain_instr { $$ = join_exprs1(&@1, $1); }
  | block_instr { $$ = join_exprs1(&@1, $1); }
  | expr { $$ = $1; }
;
plain_instr :
    UNREACHABLE {
      $$ = new_unreachable_expr();
    }
  | NOP {
      $$ = new_nop_expr();
    }
  | DROP {
      $$ = new_drop_expr();
    }
  | SELECT {
      $$ = new_select_expr();
    }
  | BR var {
      $$ = new_br_expr();
      $$->br.var = $2;
    }
  | BR_IF var {
      $$ = new_br_if_expr();
      $$->br_if.var = $2;
    }
  | BR_TABLE var_list var {
      $$ = new_br_table_expr();
      $$->br_table.targets = $2;
      $$->br_table.default_target = $3;
    }
  | RETURN {
      $$ = new_return_expr();
    }
  | CALL var {
      $$ = new_call_expr();
      $$->call.var = $2;
    }
  | CALL_INDIRECT var {
      $$ = new_call_indirect_expr();
      $$->call_indirect.var = $2;
    }
  | GET_LOCAL var {
      $$ = new_get_local_expr();
      $$->get_local.var = $2;
    }
  | SET_LOCAL var {
      $$ = new_set_local_expr();
      $$->set_local.var = $2;
    }
  | TEE_LOCAL var {
      $$ = new_tee_local_expr();
      $$->tee_local.var = $2;
    }
  | GET_GLOBAL var {
      $$ = new_get_global_expr();
      $$->get_global.var = $2;
    }
  | SET_GLOBAL var {
      $$ = new_set_global_expr();
      $$->set_global.var = $2;
    }
  | LOAD offset_opt align_opt {
      $$ = new_load_expr();
      $$->load.opcode = $1;
      $$->load.offset = $2;
      $$->load.align = $3;
    }
  | STORE offset_opt align_opt {
      $$ = new_store_expr();
      $$->store.opcode = $1;
      $$->store.offset = $2;
      $$->store.align = $3;
    }
  | CONST literal {
      $$ = new_const_expr();
      $$->const_.loc = @1;
      if (WABT_FAILED(parse_const($1, $2.type, $2.text.start,
                                  $2.text.start + $2.text.length,
                                  &$$->const_))) {
        ast_parser_error(&@2, lexer, parser,
                              "invalid literal \"" PRIstringslice "\"",
                              WABT_PRINTF_STRING_SLICE_ARG($2.text));
      }
      delete [] $2.text.start;
    }
  | UNARY {
      $$ = new_unary_expr();
      $$->unary.opcode = $1;
    }
  | BINARY {
      $$ = new_binary_expr();
      $$->binary.opcode = $1;
    }
  | COMPARE {
      $$ = new_compare_expr();
      $$->compare.opcode = $1;
    }
  | CONVERT {
      $$ = new_convert_expr();
      $$->convert.opcode = $1;
    }
  | CURRENT_MEMORY {
      $$ = new_current_memory_expr();
    }
  | GROW_MEMORY {
      $$ = new_grow_memory_expr();
    }
;
block_instr :
    BLOCK labeling_opt block END labeling_opt {
      $$ = new_block_expr();
      $$->block = $3;
      $$->block.label = $2;
      CHECK_END_LABEL(@5, $$->block.label, $5);
    }
  | LOOP labeling_opt block END labeling_opt {
      $$ = new_loop_expr();
      $$->loop = $3;
      $$->loop.label = $2;
      CHECK_END_LABEL(@5, $$->block.label, $5);
    }
  | IF labeling_opt block END labeling_opt {
      $$ = new_if_expr();
      $$->if_.true_ = $3;
      $$->if_.true_.label = $2;
      CHECK_END_LABEL(@5, $$->block.label, $5);
    }
  | IF labeling_opt block ELSE labeling_opt instr_list END labeling_opt {
      $$ = new_if_expr();
      $$->if_.true_ = $3;
      $$->if_.true_.label = $2;
      $$->if_.false_ = $6.first;
      CHECK_END_LABEL(@5, $$->block.label, $5);
      CHECK_END_LABEL(@8, $$->block.label, $8);
    }
;
block :
    value_type_list instr_list {
      WABT_ZERO_MEMORY($$);
      $$.sig = $1;
      $$.first = $2.first;
    }
;

expr :
    LPAR expr1 RPAR { $$ = $2; }
;

expr1 :
    plain_instr expr_list {
      $$ = join_exprs2(&@1, &$2, $1);
    }
  | BLOCK labeling_opt block {
      Expr* expr = new_block_expr();
      expr->block = $3;
      expr->block.label = $2;
      $$ = join_exprs1(&@1, expr);
    }
  | LOOP labeling_opt block {
      Expr* expr = new_loop_expr();
      expr->loop = $3;
      expr->loop.label = $2;
      $$ = join_exprs1(&@1, expr);
    }
  | IF labeling_opt value_type_list if_ {
      $$ = $4;
      Expr* if_ = $4.last;
      assert(if_->type == ExprType::If);
      if_->if_.true_.label = $2;
      if_->if_.true_.sig = $3;
    }
;
if_ :
    LPAR THEN instr_list RPAR LPAR ELSE instr_list RPAR {
      Expr* expr = new_if_expr();
      expr->if_.true_.first = $3.first;
      expr->if_.false_ = $7.first;
      $$ = join_exprs1(&@1, expr);
    }
  | LPAR THEN instr_list RPAR {
      Expr* expr = new_if_expr();
      expr->if_.true_.first = $3.first;
      $$ = join_exprs1(&@1, expr);
    }
  | expr LPAR THEN instr_list RPAR LPAR ELSE instr_list RPAR {
      Expr* expr = new_if_expr();
      expr->if_.true_.first = $4.first;
      expr->if_.false_ = $8.first;
      $$ = join_exprs2(&@1, &$1, expr);
    }
  | expr LPAR THEN instr_list RPAR {
      Expr* expr = new_if_expr();
      expr->if_.true_.first = $4.first;
      $$ = join_exprs2(&@1, &$1, expr);
    }
  | expr expr expr {
      Expr* expr = new_if_expr();
      expr->if_.true_.first = $2.first;
      expr->if_.false_ = $3.first;
      $$ = join_exprs2(&@1, &$1, expr);
    }
  | expr expr {
      Expr* expr = new_if_expr();
      expr->if_.true_.first = $2.first;
      $$ = join_exprs2(&@1, &$1, expr);
    }
;

instr_list :
    /* empty */ { WABT_ZERO_MEMORY($$); }
  | instr instr_list {
      $$.first = $1.first;
      $1.last->next = $2.first;
      $$.last = $2.last ? $2.last : $1.last;
      $$.size = $1.size + $2.size;
    }
;
expr_list :
    /* empty */ { WABT_ZERO_MEMORY($$); }
  | expr expr_list {
      $$.first = $1.first;
      $1.last->next = $2.first;
      $$.last = $2.last ? $2.last : $1.last;
      $$.size = $1.size + $2.size;
    }

const_expr :
    instr_list
;

/* Functions */
func_fields :
    func_body
  | LPAR RESULT value_type_list RPAR func_body {
      $$ = new_func_field();
      $$->type = FuncFieldType::ResultTypes;
      $$->types = $3;
      $$->next = $5;
    }
  | LPAR PARAM value_type_list RPAR func_fields {
      $$ = new_func_field();
      $$->type = FuncFieldType::ParamTypes;
      $$->types = $3;
      $$->next = $5;
    }
  | LPAR PARAM bind_var VALUE_TYPE RPAR func_fields {
      $$ = new_func_field();
      $$->type = FuncFieldType::BoundParam;
      $$->bound_type.loc = @2;
      $$->bound_type.name = $3;
      $$->bound_type.type = $4;
      $$->next = $6;
    }
;
func_body :
    instr_list {
      $$ = new_func_field();
      $$->type = FuncFieldType::Exprs;
      $$->first_expr = $1.first;
      $$->next = nullptr;
    }
  | LPAR LOCAL value_type_list RPAR func_body {
      $$ = new_func_field();
      $$->type = FuncFieldType::LocalTypes;
      $$->types = $3;
      $$->next = $5;
    }
  | LPAR LOCAL bind_var VALUE_TYPE RPAR func_body {
      $$ = new_func_field();
      $$->type = FuncFieldType::BoundLocal;
      $$->bound_type.loc = @2;
      $$->bound_type.name = $3;
      $$->bound_type.type = $4;
      $$->next = $6;
    }
;
func_info :
    func_fields {
      $$ = new_func();
      FuncField* field = $1;

      while (field) {
        FuncField* next = field->next;
        switch (field->type) {
          case FuncFieldType::Exprs:
            $$->first_expr = field->first_expr;
            break;

          case FuncFieldType::ParamTypes:
          case FuncFieldType::LocalTypes: {
            TypeVector* types = field->type == FuncFieldType::ParamTypes
                                        ? &$$->decl.sig.param_types
                                        : &$$->local_types;
            extend_types(types, &field->types);
            destroy_type_vector(&field->types);
            break;
          }

          case FuncFieldType::BoundParam:
          case FuncFieldType::BoundLocal: {
            TypeVector* types;
            BindingHash* bindings;
            if (field->type == FuncFieldType::BoundParam) {
              types = &$$->decl.sig.param_types;
              bindings = &$$->param_bindings;
            } else {
              types = &$$->local_types;
              bindings = &$$->local_bindings;
            }

            append_type_value(types, &field->bound_type.type);
            Binding* binding =
                insert_binding(bindings, &field->bound_type.name);
            binding->loc = field->bound_type.loc;
            binding->index = types->size - 1;
            break;
          }

          case FuncFieldType::ResultTypes:
            $$->decl.sig.result_types = field->types;
            break;
        }

        /* we steal memory from the func field, but not the linked list nodes */
        delete (field);
        field = next;
      }
    }
;
func :
    LPAR FUNC bind_var_opt inline_export type_use func_info RPAR {
      WABT_ZERO_MEMORY($$);
      $$.func = $6;
      $$.func->decl.flags |= WABT_FUNC_DECLARATION_FLAG_HAS_FUNC_TYPE;
      $$.func->decl.type_var = $5;
      $$.func->name = $3;
      $$.export_ = $4;
    }
  /* Duplicate above for empty inline_export_opt to avoid LR(1) conflict. */
  | LPAR FUNC bind_var_opt type_use func_info RPAR {
      WABT_ZERO_MEMORY($$);
      $$.func = $5;
      $$.func->decl.flags |= WABT_FUNC_DECLARATION_FLAG_HAS_FUNC_TYPE;
      $$.func->decl.type_var = $4;
      $$.func->name = $3;
    }
  | LPAR FUNC bind_var_opt inline_export func_info RPAR {
      WABT_ZERO_MEMORY($$);
      $$.func = $5;
      $$.func->name = $3;
      $$.export_ = $4;
    }
  /* Duplicate above for empty inline_export_opt to avoid LR(1) conflict. */
  | LPAR FUNC bind_var_opt func_info RPAR {
      WABT_ZERO_MEMORY($$);
      $$.func = $4;
      $$.func->name = $3;
    }
;

/* Tables & Memories */

offset :
    LPAR OFFSET const_expr RPAR {
      $$ = $3;
    }
  | expr
;

elem :
    LPAR ELEM var offset var_list RPAR {
      WABT_ZERO_MEMORY($$);
      $$.table_var = $3;
      $$.offset = $4.first;
      $$.vars = $5;
    }
  | LPAR ELEM offset var_list RPAR {
      WABT_ZERO_MEMORY($$);
      $$.table_var.loc = @2;
      $$.table_var.type = VarType::Index;
      $$.table_var.index = 0;
      $$.offset = $3.first;
      $$.vars = $4;
    }
;

table :
    LPAR TABLE bind_var_opt inline_export_opt table_sig RPAR {
      $$.table = $5;
      $$.table.name = $3;
      $$.has_elem_segment = false;
      $$.export_ = $4;
    }
  | LPAR TABLE bind_var_opt inline_export_opt elem_type
         LPAR ELEM var_list RPAR RPAR {
      Expr* expr = new_const_expr();
      expr->loc = @2;
      expr->const_.type = Type::I32;
      expr->const_.u32 = 0;

      WABT_ZERO_MEMORY($$);
      $$.table.name = $3;
      $$.table.elem_limits.initial = $8.size;
      $$.table.elem_limits.max = $8.size;
      $$.table.elem_limits.has_max = true;
      $$.has_elem_segment = true;
      $$.elem_segment.offset = expr;
      $$.elem_segment.vars = $8;
      $$.export_ = $4;
    }
;

data :
    LPAR DATA var offset text_list RPAR {
      WABT_ZERO_MEMORY($$);
      $$.memory_var = $3;
      $$.offset = $4.first;
      dup_text_list(&$5, &$$.data, &$$.size);
      destroy_text_list(&$5);
    }
  | LPAR DATA offset text_list RPAR {
      WABT_ZERO_MEMORY($$);
      $$.memory_var.loc = @2;
      $$.memory_var.type = VarType::Index;
      $$.memory_var.index = 0;
      $$.offset = $3.first;
      dup_text_list(&$4, &$$.data, &$$.size);
      destroy_text_list(&$4);
    }
;

memory :
    LPAR MEMORY bind_var_opt inline_export_opt memory_sig RPAR {
      WABT_ZERO_MEMORY($$);
      $$.memory = $5;
      $$.memory.name = $3;
      $$.has_data_segment = false;
      $$.export_ = $4;
    }
  | LPAR MEMORY bind_var_opt inline_export LPAR DATA text_list RPAR RPAR {
      Expr* expr = new_const_expr();
      expr->loc = @2;
      expr->const_.type = Type::I32;
      expr->const_.u32 = 0;

      WABT_ZERO_MEMORY($$);
      $$.has_data_segment = true;
      $$.data_segment.offset = expr;
      dup_text_list(&$7, &$$.data_segment.data, &$$.data_segment.size);
      destroy_text_list(&$7);
      uint32_t byte_size = WABT_ALIGN_UP_TO_PAGE($$.data_segment.size);
      uint32_t page_size = WABT_BYTES_TO_PAGES(byte_size);
      $$.memory.name = $3;
      $$.memory.page_limits.initial = page_size;
      $$.memory.page_limits.max = page_size;
      $$.memory.page_limits.has_max = true;
      $$.export_ = $4;
    }
  /* Duplicate above for empty inline_export_opt to avoid LR(1) conflict. */
  | LPAR MEMORY bind_var_opt LPAR DATA text_list RPAR RPAR {
      Expr* expr = new_const_expr();
      expr->loc = @2;
      expr->const_.type = Type::I32;
      expr->const_.u32 = 0;

      WABT_ZERO_MEMORY($$);
      $$.has_data_segment = true;
      $$.data_segment.offset = expr;
      dup_text_list(&$6, &$$.data_segment.data, &$$.data_segment.size);
      destroy_text_list(&$6);
      uint32_t byte_size = WABT_ALIGN_UP_TO_PAGE($$.data_segment.size);
      uint32_t page_size = WABT_BYTES_TO_PAGES(byte_size);
      $$.memory.name = $3;
      $$.memory.page_limits.initial = page_size;
      $$.memory.page_limits.max = page_size;
      $$.memory.page_limits.has_max = true;
      $$.export_.has_export = false;
    }
;

global :
    LPAR GLOBAL bind_var_opt inline_export global_type const_expr RPAR {
      WABT_ZERO_MEMORY($$);
      $$.global = $5;
      $$.global.name = $3;
      $$.global.init_expr = $6.first;
      $$.export_ = $4;
    }
  | LPAR GLOBAL bind_var_opt global_type const_expr RPAR {
      WABT_ZERO_MEMORY($$);
      $$.global = $4;
      $$.global.name = $3;
      $$.global.init_expr = $5.first;
      $$.export_.has_export = false;
    }
;


/* Imports & Exports */

import_kind :
    LPAR FUNC bind_var_opt type_use RPAR {
      $$ = new_import();
      $$->kind = ExternalKind::Func;
      $$->func.name = $3;
      $$->func.decl.flags = WABT_FUNC_DECLARATION_FLAG_HAS_FUNC_TYPE;
      $$->func.decl.type_var = $4;
    }
  | LPAR FUNC bind_var_opt func_sig RPAR {
      $$ = new_import();
      $$->kind = ExternalKind::Func;
      $$->func.name = $3;
      $$->func.decl.sig = $4;
    }
  | LPAR TABLE bind_var_opt table_sig RPAR {
      $$ = new_import();
      $$->kind = ExternalKind::Table;
      $$->table = $4;
      $$->table.name = $3;
    }
  | LPAR MEMORY bind_var_opt memory_sig RPAR {
      $$ = new_import();
      $$->kind = ExternalKind::Memory;
      $$->memory = $4;
      $$->memory.name = $3;
    }
  | LPAR GLOBAL bind_var_opt global_type RPAR {
      $$ = new_import();
      $$->kind = ExternalKind::Global;
      $$->global = $4;
      $$->global.name = $3;
    }
;
import :
    LPAR IMPORT quoted_text quoted_text import_kind RPAR {
      $$ = $5;
      $$->module_name = $3;
      $$->field_name = $4;
    }
  | LPAR FUNC bind_var_opt inline_import type_use RPAR {
      $$ = $4;
      $$->kind = ExternalKind::Func;
      $$->func.name = $3;
      $$->func.decl.flags = WABT_FUNC_DECLARATION_FLAG_HAS_FUNC_TYPE;
      $$->func.decl.type_var = $5;
    }
  | LPAR FUNC bind_var_opt inline_import func_sig RPAR {
      $$ = $4;
      $$->kind = ExternalKind::Func;
      $$->func.name = $3;
      $$->func.decl.sig = $5;
    }
  | LPAR TABLE bind_var_opt inline_import table_sig RPAR {
      $$ = $4;
      $$->kind = ExternalKind::Table;
      $$->table = $5;
      $$->table.name = $3;
    }
  | LPAR MEMORY bind_var_opt inline_import memory_sig RPAR {
      $$ = $4;
      $$->kind = ExternalKind::Memory;
      $$->memory = $5;
      $$->memory.name = $3;
    }
  | LPAR GLOBAL bind_var_opt inline_import global_type RPAR {
      $$ = $4;
      $$->kind = ExternalKind::Global;
      $$->global = $5;
      $$->global.name = $3;
    }
;

inline_import :
    LPAR IMPORT quoted_text quoted_text RPAR {
      $$ = new_import();
      $$->module_name = $3;
      $$->field_name = $4;
    }
;

export_kind :
    LPAR FUNC var RPAR {
      WABT_ZERO_MEMORY($$);
      $$.kind = ExternalKind::Func;
      $$.var = $3;
    }
  | LPAR TABLE var RPAR {
      WABT_ZERO_MEMORY($$);
      $$.kind = ExternalKind::Table;
      $$.var = $3;
    }
  | LPAR MEMORY var RPAR {
      WABT_ZERO_MEMORY($$);
      $$.kind = ExternalKind::Memory;
      $$.var = $3;
    }
  | LPAR GLOBAL var RPAR {
      WABT_ZERO_MEMORY($$);
      $$.kind = ExternalKind::Global;
      $$.var = $3;
    }
;
export :
    LPAR EXPORT quoted_text export_kind RPAR {
      $$ = $4;
      $$.name = $3;
    }
;

inline_export_opt :
    /* empty */ {
      WABT_ZERO_MEMORY($$);
      $$.has_export = false;
    }
  | inline_export
;
inline_export :
    LPAR EXPORT quoted_text RPAR {
      WABT_ZERO_MEMORY($$);
      $$.has_export = true;
      $$.export_.name = $3;
    }
;


/* Modules */

type_def :
    LPAR TYPE func_type RPAR {
      WABT_ZERO_MEMORY($$);
      $$.sig = $3;
    }
  | LPAR TYPE bind_var func_type RPAR {
      $$.name = $3;
      $$.sig = $4;
    }
;

start :
    LPAR START var RPAR { $$ = $3; }
;

module_fields :
    /* empty */ {
      $$ = new_module();
    }
  | module_fields type_def {
      $$ = $1;
      ModuleField* field;
      APPEND_FIELD_TO_LIST($$, field, FuncType, func_type, @2, $2);
      APPEND_ITEM_TO_VECTOR($$, FuncType, func_type, func_types,
                            &field->func_type);
      INSERT_BINDING($$, func_type, func_types, @2, $2.name);
    }
  | module_fields global {
      $$ = $1;
      ModuleField* field;
      APPEND_FIELD_TO_LIST($$, field, Global, global, @2, $2.global);
      APPEND_ITEM_TO_VECTOR($$, Global, global, globals, &field->global);
      INSERT_BINDING($$, global, globals, @2, $2.global.name);
      APPEND_INLINE_EXPORT($$, Global, @2, $2, $$->globals.size - 1);
    }
  | module_fields table {
      $$ = $1;
      ModuleField* field;
      APPEND_FIELD_TO_LIST($$, field, Table, table, @2, $2.table);
      APPEND_ITEM_TO_VECTOR($$, Table, table, tables, &field->table);
      INSERT_BINDING($$, table, tables, @2, $2.table.name);
      APPEND_INLINE_EXPORT($$, Table, @2, $2, $$->tables.size - 1);

      if ($2.has_elem_segment) {
        ModuleField* elem_segment_field;
        APPEND_FIELD_TO_LIST($$, elem_segment_field, ElemSegment, elem_segment,
                             @2, $2.elem_segment);
        APPEND_ITEM_TO_VECTOR($$, ElemSegment, elem_segment, elem_segments,
                              &elem_segment_field->elem_segment);
      }

    }
  | module_fields memory {
      $$ = $1;
      ModuleField* field;
      APPEND_FIELD_TO_LIST($$, field, Memory, memory, @2, $2.memory);
      APPEND_ITEM_TO_VECTOR($$, Memory, memory, memories, &field->memory);
      INSERT_BINDING($$, memory, memories, @2, $2.memory.name);
      APPEND_INLINE_EXPORT($$, Memory, @2, $2, $$->memories.size - 1);

      if ($2.has_data_segment) {
        ModuleField* data_segment_field;
        APPEND_FIELD_TO_LIST($$, data_segment_field, DataSegment, data_segment,
                             @2, $2.data_segment);
        APPEND_ITEM_TO_VECTOR($$, DataSegment, data_segment, data_segments,
                              &data_segment_field->data_segment);
      }
    }
  | module_fields func {
      $$ = $1;
      ModuleField* field;
      APPEND_FIELD_TO_LIST($$, field, Func, func, @2, *$2.func);
      append_implicit_func_declaration(&@2, $$, &field->func.decl);
      APPEND_ITEM_TO_VECTOR($$, Func, func, funcs, &field->func);
      INSERT_BINDING($$, func, funcs, @2, $2.func->name);
      APPEND_INLINE_EXPORT($$, Func, @2, $2, $$->funcs.size - 1);
      delete $2.func;
    }
  | module_fields elem {
      $$ = $1;
      ModuleField* field;
      APPEND_FIELD_TO_LIST($$, field, ElemSegment, elem_segment, @2, $2);
      APPEND_ITEM_TO_VECTOR($$, ElemSegment, elem_segment, elem_segments,
                            &field->elem_segment);
    }
  | module_fields data {
      $$ = $1;
      ModuleField* field;
      APPEND_FIELD_TO_LIST($$, field, DataSegment, data_segment, @2, $2);
      APPEND_ITEM_TO_VECTOR($$, DataSegment, data_segment, data_segments,
                            &field->data_segment);
    }
  | module_fields start {
      $$ = $1;
      ModuleField* field;
      APPEND_FIELD_TO_LIST($$, field, Start, start, @2, $2);
      $$->start = &field->start;
    }
  | module_fields import {
      $$ = $1;
      ModuleField* field;
      APPEND_FIELD_TO_LIST($$, field, Import, import, @2, *$2);
      CHECK_IMPORT_ORDERING($$, func, funcs, @2);
      CHECK_IMPORT_ORDERING($$, table, tables, @2);
      CHECK_IMPORT_ORDERING($$, memory, memories, @2);
      CHECK_IMPORT_ORDERING($$, global, globals, @2);
      switch ($2->kind) {
        case ExternalKind::Func:
          append_implicit_func_declaration(&@2, $$, &field->import.func.decl);
          APPEND_ITEM_TO_VECTOR($$, Func, func, funcs, &field->import.func);
          INSERT_BINDING($$, func, funcs, @2, field->import.func.name);
          $$->num_func_imports++;
          break;
        case ExternalKind::Table:
          APPEND_ITEM_TO_VECTOR($$, Table, table, tables, &field->import.table);
          INSERT_BINDING($$, table, tables, @2, field->import.table.name);
          $$->num_table_imports++;
          break;
        case ExternalKind::Memory:
          APPEND_ITEM_TO_VECTOR($$, Memory, memory, memories,
                                &field->import.memory);
          INSERT_BINDING($$, memory, memories, @2, field->import.memory.name);
          $$->num_memory_imports++;
          break;
        case ExternalKind::Global:
          APPEND_ITEM_TO_VECTOR($$, Global, global, globals,
                                &field->import.global);
          INSERT_BINDING($$, global, globals, @2, field->import.global.name);
          $$->num_global_imports++;
          break;
      }
      delete $2;
      APPEND_ITEM_TO_VECTOR($$, Import, import, imports, &field->import);
    }
  | module_fields export {
      $$ = $1;
      ModuleField* field = append_module_field($$);
      APPEND_FIELD_TO_LIST($$, field, Export, export_, @2, $2);
      APPEND_ITEM_TO_VECTOR($$, Export, export, exports, &field->export_);
      INSERT_BINDING($$, export, exports, @2, $2.name);
    }
;

raw_module :
    LPAR MODULE bind_var_opt module_fields RPAR {
      $$.type = RawModuleType::Text;
      $$.text = $4;
      $$.text->name = $3;
      $$.text->loc = @2;

      /* resolve func type variables where the signature was not specified
       * explicitly */
      size_t i;
      for (i = 0; i < $4->funcs.size; ++i) {
        Func* func = $4->funcs.data[i];
        if (decl_has_func_type(&func->decl) &&
            is_empty_signature(&func->decl.sig)) {
          FuncType* func_type =
              get_func_type_by_var($4, &func->decl.type_var);
          if (func_type) {
            func->decl.sig = func_type->sig;
            func->decl.flags |= WABT_FUNC_DECLARATION_FLAG_SHARED_SIGNATURE;
          }
        }
      }
    }
  | LPAR MODULE bind_var_opt non_empty_text_list RPAR {
      $$.type = RawModuleType::Binary;
      $$.binary.name = $3;
      $$.binary.loc = @2;
      dup_text_list(&$4, &$$.binary.data, &$$.binary.size);
      destroy_text_list(&$4);
    }
;

module :
    raw_module {
      if ($1.type == RawModuleType::Text) {
        $$ = $1.text;
      } else {
        assert($1.type == RawModuleType::Binary);
        $$ = new_module();
        ReadBinaryOptions options = WABT_READ_BINARY_OPTIONS_DEFAULT;
        BinaryErrorCallbackData user_data;
        user_data.loc = &$1.binary.loc;
        user_data.lexer = lexer;
        user_data.parser = parser;
        BinaryErrorHandler error_handler;
        error_handler.on_error = on_read_binary_error;
        error_handler.user_data = &user_data;
        read_binary_ast($1.binary.data, $1.binary.size, &options,
                             &error_handler, $$);
        delete [] $1.binary.data;
        $$->name = $1.binary.name;
        $$->loc = $1.binary.loc;
      }
    }
;

/* Scripts */

script_var_opt :
    /* empty */ {
      WABT_ZERO_MEMORY($$);
      $$.type = VarType::Index;
      $$.index = INVALID_VAR_INDEX;
    }
  | VAR {
      WABT_ZERO_MEMORY($$);
      $$.type = VarType::Name;
      DUPTEXT($$.name, $1);
    }
;

action :
    LPAR INVOKE script_var_opt quoted_text const_list RPAR {
      WABT_ZERO_MEMORY($$);
      $$.loc = @2;
      $$.module_var = $3;
      $$.type = ActionType::Invoke;
      $$.invoke.name = $4;
      $$.invoke.args = $5;
    }
  | LPAR GET script_var_opt quoted_text RPAR {
      WABT_ZERO_MEMORY($$);
      $$.loc = @2;
      $$.module_var = $3;
      $$.type = ActionType::Get;
      $$.invoke.name = $4;
    }
;

assertion :
    LPAR ASSERT_MALFORMED raw_module quoted_text RPAR {
      $$ = new_command();
      $$->type = CommandType::AssertMalformed;
      $$->assert_malformed.module = $3;
      $$->assert_malformed.text = $4;
    }
  | LPAR ASSERT_INVALID raw_module quoted_text RPAR {
      $$ = new_command();
      $$->type = CommandType::AssertInvalid;
      $$->assert_invalid.module = $3;
      $$->assert_invalid.text = $4;
    }
  | LPAR ASSERT_UNLINKABLE raw_module quoted_text RPAR {
      $$ = new_command();
      $$->type = CommandType::AssertUnlinkable;
      $$->assert_unlinkable.module = $3;
      $$->assert_unlinkable.text = $4;
    }
  | LPAR ASSERT_TRAP raw_module quoted_text RPAR {
      $$ = new_command();
      $$->type = CommandType::AssertUninstantiable;
      $$->assert_uninstantiable.module = $3;
      $$->assert_uninstantiable.text = $4;
    }
  | LPAR ASSERT_RETURN action const_list RPAR {
      $$ = new_command();
      $$->type = CommandType::AssertReturn;
      $$->assert_return.action = $3;
      $$->assert_return.expected = $4;
    }
  | LPAR ASSERT_RETURN_NAN action RPAR {
      $$ = new_command();
      $$->type = CommandType::AssertReturnNan;
      $$->assert_return_nan.action = $3;
    }
  | LPAR ASSERT_TRAP action quoted_text RPAR {
      $$ = new_command();
      $$->type = CommandType::AssertTrap;
      $$->assert_trap.action = $3;
      $$->assert_trap.text = $4;
    }
  | LPAR ASSERT_EXHAUSTION action quoted_text RPAR {
      $$ = new_command();
      $$->type = CommandType::AssertExhaustion;
      $$->assert_trap.action = $3;
      $$->assert_trap.text = $4;
    }
;

cmd :
    action {
      $$ = new_command();
      $$->type = CommandType::Action;
      $$->action = $1;
    }
  | assertion
  | module {
      $$ = new_command();
      $$->type = CommandType::Module;
      $$->module = *$1;
      delete $1;
    }
  | LPAR REGISTER quoted_text script_var_opt RPAR {
      $$ = new_command();
      $$->type = CommandType::Register;
      $$->register_.module_name = $3;
      $$->register_.var = $4;
      $$->register_.var.loc = @4;
    }
;
cmd_list :
    /* empty */ { WABT_ZERO_MEMORY($$); }
  | cmd_list cmd {
      $$ = $1;
      append_command_value(&$$, $2);
      delete $2;
    }
;

const :
    LPAR CONST literal RPAR {
      $$.loc = @2;
      if (WABT_FAILED(parse_const($2, $3.type, $3.text.start,
                                  $3.text.start + $3.text.length, &$$))) {
        ast_parser_error(&@3, lexer, parser,
                              "invalid literal \"" PRIstringslice "\"",
                              WABT_PRINTF_STRING_SLICE_ARG($3.text));
      }
      delete [] $3.text.start;
    }
;
const_list :
    /* empty */ { WABT_ZERO_MEMORY($$); }
  | const_list const {
      $$ = $1;
      append_const_value(&$$, &$2);
    }
;

script :
    cmd_list {
      WABT_ZERO_MEMORY($$);
      $$.commands = $1;

      int last_module_index = -1;
      size_t i;
      for (i = 0; i < $$.commands.size; ++i) {
        Command* command = &$$.commands.data[i];
        Var* module_var = nullptr;
        switch (command->type) {
          case CommandType::Module: {
            last_module_index = i;

            /* Wire up module name bindings. */
            Module* module = &command->module;
            if (module->name.length == 0)
              continue;

            StringSlice module_name = dup_string_slice(module->name);
            Binding* binding =
                insert_binding(&$$.module_bindings, &module_name);
            binding->loc = module->loc;
            binding->index = i;
            break;
          }

          case CommandType::AssertReturn:
            module_var = &command->assert_return.action.module_var;
            goto has_module_var;
          case CommandType::AssertReturnNan:
            module_var = &command->assert_return_nan.action.module_var;
            goto has_module_var;
          case CommandType::AssertTrap:
          case CommandType::AssertExhaustion:
            module_var = &command->assert_trap.action.module_var;
            goto has_module_var;
          case CommandType::Action:
            module_var = &command->action.module_var;
            goto has_module_var;
          case CommandType::Register:
            module_var = &command->register_.var;
            goto has_module_var;

          has_module_var: {
            /* Resolve actions with an invalid index to use the preceding
             * module. */
            if (module_var->type == VarType::Index &&
                module_var->index == INVALID_VAR_INDEX) {
              module_var->index = last_module_index;
            }
            break;
          }

          default:
            break;
        }
      }
      parser->script = $$;
    }
;

/* bison destroys the start symbol even on a successful parse. We want to keep
 script from being destroyed, so create a dummy start symbol. */
script_start :
    script
;

%%

void append_expr_list(ExprList* expr_list, ExprList* expr) {
  if (!expr->first)
    return;
  if (expr_list->last)
    expr_list->last->next = expr->first;
  else
    expr_list->first = expr->first;
  expr_list->last = expr->last;
  expr_list->size += expr->size;
}

void append_expr(ExprList* expr_list, Expr* expr) {
  if (expr_list->last)
    expr_list->last->next = expr;
  else
    expr_list->first = expr;
  expr_list->last = expr;
  expr_list->size++;
}

ExprList join_exprs1(Location* loc, Expr* expr1) {
  ExprList result;
  WABT_ZERO_MEMORY(result);
  append_expr(&result, expr1);
  expr1->loc = *loc;
  return result;
}

ExprList join_exprs2(Location* loc, ExprList* expr1, Expr* expr2) {
  ExprList result;
  WABT_ZERO_MEMORY(result);
  append_expr_list(&result, expr1);
  append_expr(&result, expr2);
  expr2->loc = *loc;
  return result;
}

Result parse_const(Type type,
                   LiteralType literal_type,
                   const char* s,
                   const char* end,
                   Const* out) {
  out->type = type;
  switch (type) {
    case Type::I32:
      return parse_int32(s, end, &out->u32, ParseIntType::SignedAndUnsigned);
    case Type::I64:
      return parse_int64(s, end, &out->u64, ParseIntType::SignedAndUnsigned);
    case Type::F32:
      return parse_float(literal_type, s, end, &out->f32_bits);
    case Type::F64:
      return parse_double(literal_type, s, end, &out->f64_bits);
    default:
      assert(0);
      break;
  }
  return Result::Error;
}

size_t copy_string_contents(StringSlice* text, char* dest) {
  const char* src = text->start + 1;
  const char* end = text->start + text->length - 1;

  char* dest_start = dest;

  while (src < end) {
    if (*src == '\\') {
      src++;
      switch (*src) {
        case 'n':
          *dest++ = '\n';
          break;
        case 't':
          *dest++ = '\t';
          break;
        case '\\':
          *dest++ = '\\';
          break;
        case '\'':
          *dest++ = '\'';
          break;
        case '\"':
          *dest++ = '\"';
          break;
        default: {
          /* The string should be validated already, so we know this is a hex
           * sequence */
          uint32_t hi;
          uint32_t lo;
          if (WABT_SUCCEEDED(parse_hexdigit(src[0], &hi)) &&
              WABT_SUCCEEDED(parse_hexdigit(src[1], &lo))) {
            *dest++ = (hi << 4) | lo;
          } else {
            assert(0);
          }
          src++;
          break;
        }
      }
      src++;
    } else {
      *dest++ = *src++;
    }
  }
  /* return the data length */
  return dest - dest_start;
}

void dup_text_list(TextList* text_list, char** out_data, size_t* out_size) {
  /* walk the linked list to see how much total space is needed */
  size_t total_size = 0;
  TextListNode* node;
  for (node = text_list->first; node; node = node->next) {
    /* Always allocate enough space for the entire string including the escape
     * characters. It will only get shorter, and this way we only have to
     * iterate through the string once. */
    const char* src = node->text.start + 1;
    const char* end = node->text.start + node->text.length - 1;
    size_t size = (end > src) ? (end - src) : 0;
    total_size += size;
  }
  char* result = new char [total_size];
  char* dest = result;
  for (node = text_list->first; node; node = node->next) {
    size_t actual_size = copy_string_contents(&node->text, dest);
    dest += actual_size;
  }
  *out_data = result;
  *out_size = dest - result;
}

bool is_empty_signature(FuncSignature* sig) {
  return sig->result_types.size == 0 && sig->param_types.size == 0;
}

void append_implicit_func_declaration(Location* loc,
                                      Module* module,
                                      FuncDeclaration* decl) {
  if (decl_has_func_type(decl))
    return;

  int sig_index = get_func_type_index_by_decl(module, decl);
  if (sig_index == -1) {
    append_implicit_func_type(loc, module, &decl->sig);
  } else {
    /* signature already exists, share that one and destroy this one */
    destroy_func_signature(&decl->sig);
    FuncSignature* sig = &module->func_types.data[sig_index]->sig;
    decl->sig = *sig;
  }

  decl->flags |= WABT_FUNC_DECLARATION_FLAG_SHARED_SIGNATURE;
}

Result parse_ast(AstLexer * lexer, struct Script * out_script,
                 SourceErrorHandler * error_handler) {
  AstParser parser;
  WABT_ZERO_MEMORY(parser);
  parser.error_handler = error_handler;
  int result = wabt_ast_parser_parse(lexer, &parser);
  delete [] parser.yyssa;
  delete [] parser.yyvsa;
  delete [] parser.yylsa;
  *out_script = parser.script;
  return result == 0 && parser.errors == 0 ? Result::Ok : Result::Error;
}

void on_read_binary_error(uint32_t offset, const char* error, void* user_data) {
  BinaryErrorCallbackData* data = (BinaryErrorCallbackData*)user_data;
  if (offset == WABT_UNKNOWN_OFFSET) {
    ast_parser_error(data->loc, data->lexer, data->parser,
                     "error in binary module: %s", error);
  } else {
    ast_parser_error(data->loc, data->lexer, data->parser,
                     "error in binary module: @0x%08x: %s", offset, error);
  }
}

}  // namespace wabt

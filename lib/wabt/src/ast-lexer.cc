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

#include "ast-lexer.h"

#include <assert.h>
#include <stdio.h>

#include "config.h"

#include "ast-parser.h"
#include "ast-parser-lexer-shared.h"

/* must be included after so some typedefs will be defined */
#include "ast-parser-gen.hh"

/*!max:re2c */

#define INITIAL_LEXER_BUFFER_SIZE (64 * 1024)

#define YY_USER_ACTION                        \
  {                                           \
    loc->filename = lexer->filename;          \
    loc->line = lexer->line;                  \
    loc->first_column = COLUMN(lexer->token); \
    loc->last_column = COLUMN(lexer->cursor); \
  }

#define RETURN(name) \
  YY_USER_ACTION;    \
  return WABT_TOKEN_TYPE_##name

#define ERROR(...) \
  YY_USER_ACTION;  \
  ast_parser_error(loc, lexer, parser, __VA_ARGS__)

#define BEGIN(c) \
  do {           \
    cond = c;    \
  } while (0)
#define FILL(n)                                     \
  do {                                              \
    if (WABT_FAILED(fill(loc, lexer, parser, n))) { \
      RETURN(EOF);                                  \
      continue;                                     \
    }                                               \
  } while (0)

#define yytext (lexer->token)
#define yyleng (lexer->cursor - lexer->token)

/* p must be a pointer somewhere in the lexer buffer */
#define FILE_OFFSET(p) ((p) - (lexer->buffer) + lexer->buffer_file_offset)
#define COLUMN(p) (FILE_OFFSET(p) - lexer->line_file_offset + 1)

#define COMMENT_NESTING (lexer->comment_nesting)
#define NEWLINE                                           \
  do {                                                    \
    lexer->line++;                                        \
    lexer->line_file_offset = FILE_OFFSET(lexer->cursor); \
  } while (0)

#define TEXT                 \
  lval->text.start = yytext; \
  lval->text.length = yyleng

#define TEXT_AT(offset)               \
  lval->text.start = yytext + offset; \
  lval->text.length = yyleng - offset

#define TYPE(type_) lval->type = Type::type_

#define OPCODE(name) lval->opcode = Opcode::name

#define LITERAL(type_)                     \
  lval->literal.type = LiteralType::type_; \
  lval->literal.text.start = yytext;       \
  lval->literal.text.length = yyleng

namespace wabt {

static Result fill(Location* loc,
                   AstLexer* lexer,
                   AstParser* parser,
                   size_t need) {
  if (lexer->eof)
    return Result::Error;
  size_t free = lexer->token - lexer->buffer;
  assert(static_cast<size_t>(lexer->cursor - lexer->buffer) >= free);
  /* our buffer is too small, need to realloc */
  if (free < need) {
    char* old_buffer = lexer->buffer;
    size_t old_buffer_size = lexer->buffer_size;
    size_t new_buffer_size =
        old_buffer_size ? old_buffer_size * 2 : INITIAL_LEXER_BUFFER_SIZE;
    /* make sure there is enough space for the bytes requested (need) and an
     * additional YYMAXFILL bytes which is needed for the re2c lexer
     * implementation when the eof is reached */
    while ((new_buffer_size - old_buffer_size) + free < need + YYMAXFILL)
      new_buffer_size *= 2;

    char* new_buffer = new char[new_buffer_size];
    if (!new_buffer) {
      ast_parser_error(loc, lexer, parser,
                            "unable to reallocate lexer buffer.");
      return Result::Error;
    }
    memmove(new_buffer, lexer->token, lexer->limit - lexer->token);
    lexer->buffer = new_buffer;
    lexer->buffer_size = new_buffer_size;
    lexer->token = new_buffer + (lexer->token - old_buffer) - free;
    lexer->marker = new_buffer + (lexer->marker - old_buffer) - free;
    lexer->cursor = new_buffer + (lexer->cursor - old_buffer) - free;
    lexer->limit = new_buffer + (lexer->limit - old_buffer) - free;
    lexer->buffer_file_offset += free;
    free += new_buffer_size - old_buffer_size;
    delete [] old_buffer;
  } else {
    /* shift everything down to make more room in the buffer */
    memmove(lexer->buffer, lexer->token, lexer->limit - lexer->token);
    lexer->token -= free;
    lexer->marker -= free;
    lexer->cursor -= free;
    lexer->limit -= free;
    lexer->buffer_file_offset += free;
  }
  /* read the new data into the buffer */
  if (lexer->source.type == AstLexerSourceType::File) {
    lexer->limit += fread(lexer->limit, 1, free, lexer->source.file);
  } else {
    /* TODO(binji): could lex directly from buffer */
    assert(lexer->source.type == AstLexerSourceType::Buffer);
    size_t read_size = free;
    size_t offset = lexer->source.buffer.read_offset;
    size_t bytes_left = lexer->source.buffer.size - offset;
    if (read_size > bytes_left)
      read_size = bytes_left;
    memcpy(lexer->limit,
           static_cast<const char*>(lexer->source.buffer.data) + offset,
           read_size);
    lexer->source.buffer.read_offset += read_size;
    lexer->limit += read_size;
  }
  /* if at the end of file, need to fill YYMAXFILL more characters with "fake
   * characters", that are not a lexeme nor a lexeme suffix. see
   * http://re2c.org/examples/example_03.html */
  if (lexer->limit < lexer->buffer + lexer->buffer_size - YYMAXFILL) {
    lexer->eof = true;
    memset(lexer->limit, 0, YYMAXFILL);
    lexer->limit += YYMAXFILL;
  }
  return Result::Ok;
}

int ast_lexer_lex(WABT_AST_PARSER_STYPE* lval,
                  WABT_AST_PARSER_LTYPE* loc,
                  AstLexer* lexer,
                  AstParser* parser) {
  enum {
    YYCOND_INIT,
    YYCOND_BAD_TEXT,
    YYCOND_LINE_COMMENT,
    YYCOND_BLOCK_COMMENT,
    YYCOND_i = YYCOND_INIT,
  } cond = YYCOND_INIT;

  for (;;) {
    lexer->token = lexer->cursor;
    /*!re2c
      re2c:condprefix = YYCOND_;
      re2c:condenumprefix = YYCOND_;
      re2c:define:YYCTYPE = "unsigned char";
      re2c:define:YYCURSOR = lexer->cursor;
      re2c:define:YYMARKER = lexer->marker;
      re2c:define:YYLIMIT = lexer->limit;
      re2c:define:YYFILL = "FILL";
      re2c:define:YYGETCONDITION = "cond";
      re2c:define:YYGETCONDITION:naked = 1;
      re2c:define:YYSETCONDITION = "BEGIN";

      space =     [ \t];
      digit =     [0-9];
      digits =    [0-9]+;
      hexdigit =  [0-9a-fA-F];
      letter =    [a-zA-Z];
      symbol =    [+\-*\/\\\^~=<>!?@#$%&|:`.];
      tick =      "'";
      escape =    [nt\\'"];
      character = [^"\\\x00-\x1f\x7f] | "\\" escape | "\\" hexdigit hexdigit;
      sign =      [+-];
      num =       digit+;
      hexnum =    "0x" hexdigit+;
      nat =       num | hexnum;
      int =       sign nat;
      float0 =    sign? num "." digit*;
      float1 =    sign? num ("." digit*)? [eE] sign? num;
      hexfloat =  sign? "0x" hexdigit+ "."? hexdigit* "p" sign? digit+;
      infinity =  sign? ("inf" | "infinity");
      nan =       sign? "nan" | sign? "nan:0x" hexdigit+;
      float =     float0 | float1;
      text =      '"' character* '"';
      atom =      (letter | digit | "_" | tick | symbol)+;
      name =      "$" atom;
      EOF =       "\x00";

      <i> "("                   { RETURN(LPAR); }
      <i> ")"                   { RETURN(RPAR); }
      <i> nat                   { LITERAL(Int); RETURN(NAT); }
      <i> int                   { LITERAL(Int); RETURN(INT); }
      <i> float                 { LITERAL(Float); RETURN(FLOAT); }
      <i> hexfloat              { LITERAL(Hexfloat); RETURN(FLOAT); }
      <i> infinity              { LITERAL(Infinity); RETURN(FLOAT); }
      <i> nan                   { LITERAL(Nan); RETURN(FLOAT); }
      <i> text                  { TEXT; RETURN(TEXT); }
      <i> '"' => BAD_TEXT       { continue; }
      <BAD_TEXT> character      { continue; }
      <BAD_TEXT> "\n" => i      { ERROR("newline in string");
                                  NEWLINE;
                                  continue; }
      <BAD_TEXT> "\\".          { ERROR("bad escape \"%.*s\"",
                                        static_cast<int>(yyleng), yytext);
                                  continue; }
      <BAD_TEXT> '"' => i       { TEXT; RETURN(TEXT); }
      <BAD_TEXT> EOF            { ERROR("unexpected EOF"); RETURN(EOF); }
      <BAD_TEXT> [^]            { ERROR("illegal character in string");
                                  continue; }
      <i> "i32"                 { TYPE(I32); RETURN(VALUE_TYPE); }
      <i> "i64"                 { TYPE(I64); RETURN(VALUE_TYPE); }
      <i> "f32"                 { TYPE(F32); RETURN(VALUE_TYPE); }
      <i> "f64"                 { TYPE(F64); RETURN(VALUE_TYPE); }
      <i> "anyfunc"             { RETURN(ANYFUNC); }
      <i> "mut"                 { RETURN(MUT); }
      <i> "nop"                 { RETURN(NOP); }
      <i> "block"               { RETURN(BLOCK); }
      <i> "if"                  { RETURN(IF); }
      <i> "if_else"             { RETURN(IF); }
      <i> "then"                { RETURN(THEN); }
      <i> "else"                { RETURN(ELSE); }
      <i> "loop"                { RETURN(LOOP); }
      <i> "br"                  { RETURN(BR); }
      <i> "br_if"               { RETURN(BR_IF); }
      <i> "br_table"            { RETURN(BR_TABLE); }
      <i> "call"                { RETURN(CALL); }
      <i> "call_import"         { RETURN(CALL_IMPORT); }
      <i> "call_indirect"       { RETURN(CALL_INDIRECT); }
      <i> "drop"                { RETURN(DROP); }
      <i> "end"                 { RETURN(END); }
      <i> "return"              { RETURN(RETURN); }
      <i> "get_local"           { RETURN(GET_LOCAL); }
      <i> "set_local"           { RETURN(SET_LOCAL); }
      <i> "tee_local"           { RETURN(TEE_LOCAL); }
      <i> "get_global"          { RETURN(GET_GLOBAL); }
      <i> "set_global"          { RETURN(SET_GLOBAL); }
      <i> "i32.load"            { OPCODE(I32Load); RETURN(LOAD); }
      <i> "i64.load"            { OPCODE(I64Load); RETURN(LOAD); }
      <i> "f32.load"            { OPCODE(F32Load); RETURN(LOAD); }
      <i> "f64.load"            { OPCODE(F64Load); RETURN(LOAD); }
      <i> "i32.store"           { OPCODE(I32Store); RETURN(STORE); }
      <i> "i64.store"           { OPCODE(I64Store); RETURN(STORE); }
      <i> "f32.store"           { OPCODE(F32Store); RETURN(STORE); }
      <i> "f64.store"           { OPCODE(F64Store); RETURN(STORE); }
      <i> "i32.load8_s"         { OPCODE(I32Load8S); RETURN(LOAD); }
      <i> "i64.load8_s"         { OPCODE(I64Load8S); RETURN(LOAD); }
      <i> "i32.load8_u"         { OPCODE(I32Load8U); RETURN(LOAD); }
      <i> "i64.load8_u"         { OPCODE(I64Load8U); RETURN(LOAD); }
      <i> "i32.load16_s"        { OPCODE(I32Load16S); RETURN(LOAD); }
      <i> "i64.load16_s"        { OPCODE(I64Load16S); RETURN(LOAD); }
      <i> "i32.load16_u"        { OPCODE(I32Load16U); RETURN(LOAD); }
      <i> "i64.load16_u"        { OPCODE(I64Load16U); RETURN(LOAD); }
      <i> "i64.load32_s"        { OPCODE(I64Load32S); RETURN(LOAD); }
      <i> "i64.load32_u"        { OPCODE(I64Load32U); RETURN(LOAD); }
      <i> "i32.store8"          { OPCODE(I32Store8); RETURN(STORE); }
      <i> "i64.store8"          { OPCODE(I64Store8); RETURN(STORE); }
      <i> "i32.store16"         { OPCODE(I32Store16); RETURN(STORE); }
      <i> "i64.store16"         { OPCODE(I64Store16); RETURN(STORE); }
      <i> "i64.store32"         { OPCODE(I64Store32); RETURN(STORE); }
      <i> "offset=" nat         { TEXT_AT(7); RETURN(OFFSET_EQ_NAT); }
      <i> "align=" nat          { TEXT_AT(6); RETURN(ALIGN_EQ_NAT); }
      <i> "i32.const"           { TYPE(I32); RETURN(CONST); }
      <i> "i64.const"           { TYPE(I64); RETURN(CONST); }
      <i> "f32.const"           { TYPE(F32); RETURN(CONST); }
      <i> "f64.const"           { TYPE(F64); RETURN(CONST); }
      <i> "i32.eqz"             { OPCODE(I32Eqz); RETURN(CONVERT); }
      <i> "i64.eqz"             { OPCODE(I64Eqz); RETURN(CONVERT); }
      <i> "i32.clz"             { OPCODE(I32Clz); RETURN(UNARY); }
      <i> "i64.clz"             { OPCODE(I64Clz); RETURN(UNARY); }
      <i> "i32.ctz"             { OPCODE(I32Ctz); RETURN(UNARY); }
      <i> "i64.ctz"             { OPCODE(I64Ctz); RETURN(UNARY); }
      <i> "i32.popcnt"          { OPCODE(I32Popcnt); RETURN(UNARY); }
      <i> "i64.popcnt"          { OPCODE(I64Popcnt); RETURN(UNARY); }
      <i> "f32.neg"             { OPCODE(F32Neg); RETURN(UNARY); }
      <i> "f64.neg"             { OPCODE(F64Neg); RETURN(UNARY); }
      <i> "f32.abs"             { OPCODE(F32Abs); RETURN(UNARY); }
      <i> "f64.abs"             { OPCODE(F64Abs); RETURN(UNARY); }
      <i> "f32.sqrt"            { OPCODE(F32Sqrt); RETURN(UNARY); }
      <i> "f64.sqrt"            { OPCODE(F64Sqrt); RETURN(UNARY); }
      <i> "f32.ceil"            { OPCODE(F32Ceil); RETURN(UNARY); }
      <i> "f64.ceil"            { OPCODE(F64Ceil); RETURN(UNARY); }
      <i> "f32.floor"           { OPCODE(F32Floor); RETURN(UNARY); }
      <i> "f64.floor"           { OPCODE(F64Floor); RETURN(UNARY); }
      <i> "f32.trunc"           { OPCODE(F32Trunc); RETURN(UNARY); }
      <i> "f64.trunc"           { OPCODE(F64Trunc); RETURN(UNARY); }
      <i> "f32.nearest"         { OPCODE(F32Nearest); RETURN(UNARY); }
      <i> "f64.nearest"         { OPCODE(F64Nearest); RETURN(UNARY); }
      <i> "i32.add"             { OPCODE(I32Add); RETURN(BINARY); }
      <i> "i64.add"             { OPCODE(I64Add); RETURN(BINARY); }
      <i> "i32.sub"             { OPCODE(I32Sub); RETURN(BINARY); }
      <i> "i64.sub"             { OPCODE(I64Sub); RETURN(BINARY); }
      <i> "i32.mul"             { OPCODE(I32Mul); RETURN(BINARY); }
      <i> "i64.mul"             { OPCODE(I64Mul); RETURN(BINARY); }
      <i> "i32.div_s"           { OPCODE(I32DivS); RETURN(BINARY); }
      <i> "i64.div_s"           { OPCODE(I64DivS); RETURN(BINARY); }
      <i> "i32.div_u"           { OPCODE(I32DivU); RETURN(BINARY); }
      <i> "i64.div_u"           { OPCODE(I64DivU); RETURN(BINARY); }
      <i> "i32.rem_s"           { OPCODE(I32RemS); RETURN(BINARY); }
      <i> "i64.rem_s"           { OPCODE(I64RemS); RETURN(BINARY); }
      <i> "i32.rem_u"           { OPCODE(I32RemU); RETURN(BINARY); }
      <i> "i64.rem_u"           { OPCODE(I64RemU); RETURN(BINARY); }
      <i> "i32.and"             { OPCODE(I32And); RETURN(BINARY); }
      <i> "i64.and"             { OPCODE(I64And); RETURN(BINARY); }
      <i> "i32.or"              { OPCODE(I32Or); RETURN(BINARY); }
      <i> "i64.or"              { OPCODE(I64Or); RETURN(BINARY); }
      <i> "i32.xor"             { OPCODE(I32Xor); RETURN(BINARY); }
      <i> "i64.xor"             { OPCODE(I64Xor); RETURN(BINARY); }
      <i> "i32.shl"             { OPCODE(I32Shl); RETURN(BINARY); }
      <i> "i64.shl"             { OPCODE(I64Shl); RETURN(BINARY); }
      <i> "i32.shr_s"           { OPCODE(I32ShrS); RETURN(BINARY); }
      <i> "i64.shr_s"           { OPCODE(I64ShrS); RETURN(BINARY); }
      <i> "i32.shr_u"           { OPCODE(I32ShrU); RETURN(BINARY); }
      <i> "i64.shr_u"           { OPCODE(I64ShrU); RETURN(BINARY); }
      <i> "i32.rotl"            { OPCODE(I32Rotl); RETURN(BINARY); }
      <i> "i64.rotl"            { OPCODE(I64Rotl); RETURN(BINARY); }
      <i> "i32.rotr"            { OPCODE(I32Rotr); RETURN(BINARY); }
      <i> "i64.rotr"            { OPCODE(I64Rotr); RETURN(BINARY); }
      <i> "f32.add"             { OPCODE(F32Add); RETURN(BINARY); }
      <i> "f64.add"             { OPCODE(F64Add); RETURN(BINARY); }
      <i> "f32.sub"             { OPCODE(F32Sub); RETURN(BINARY); }
      <i> "f64.sub"             { OPCODE(F64Sub); RETURN(BINARY); }
      <i> "f32.mul"             { OPCODE(F32Mul); RETURN(BINARY); }
      <i> "f64.mul"             { OPCODE(F64Mul); RETURN(BINARY); }
      <i> "f32.div"             { OPCODE(F32Div); RETURN(BINARY); }
      <i> "f64.div"             { OPCODE(F64Div); RETURN(BINARY); }
      <i> "f32.min"             { OPCODE(F32Min); RETURN(BINARY); }
      <i> "f64.min"             { OPCODE(F64Min); RETURN(BINARY); }
      <i> "f32.max"             { OPCODE(F32Max); RETURN(BINARY); }
      <i> "f64.max"             { OPCODE(F64Max); RETURN(BINARY); }
      <i> "f32.copysign"        { OPCODE(F32Copysign); RETURN(BINARY); }
      <i> "f64.copysign"        { OPCODE(F64Copysign); RETURN(BINARY); }
      <i> "i32.eq"              { OPCODE(I32Eq); RETURN(COMPARE); }
      <i> "i64.eq"              { OPCODE(I64Eq); RETURN(COMPARE); }
      <i> "i32.ne"              { OPCODE(I32Ne); RETURN(COMPARE); }
      <i> "i64.ne"              { OPCODE(I64Ne); RETURN(COMPARE); }
      <i> "i32.lt_s"            { OPCODE(I32LtS); RETURN(COMPARE); }
      <i> "i64.lt_s"            { OPCODE(I64LtS); RETURN(COMPARE); }
      <i> "i32.lt_u"            { OPCODE(I32LtU); RETURN(COMPARE); }
      <i> "i64.lt_u"            { OPCODE(I64LtU); RETURN(COMPARE); }
      <i> "i32.le_s"            { OPCODE(I32LeS); RETURN(COMPARE); }
      <i> "i64.le_s"            { OPCODE(I64LeS); RETURN(COMPARE); }
      <i> "i32.le_u"            { OPCODE(I32LeU); RETURN(COMPARE); }
      <i> "i64.le_u"            { OPCODE(I64LeU); RETURN(COMPARE); }
      <i> "i32.gt_s"            { OPCODE(I32GtS); RETURN(COMPARE); }
      <i> "i64.gt_s"            { OPCODE(I64GtS); RETURN(COMPARE); }
      <i> "i32.gt_u"            { OPCODE(I32GtU); RETURN(COMPARE); }
      <i> "i64.gt_u"            { OPCODE(I64GtU); RETURN(COMPARE); }
      <i> "i32.ge_s"            { OPCODE(I32GeS); RETURN(COMPARE); }
      <i> "i64.ge_s"            { OPCODE(I64GeS); RETURN(COMPARE); }
      <i> "i32.ge_u"            { OPCODE(I32GeU); RETURN(COMPARE); }
      <i> "i64.ge_u"            { OPCODE(I64GeU); RETURN(COMPARE); }
      <i> "f32.eq"              { OPCODE(F32Eq); RETURN(COMPARE); }
      <i> "f64.eq"              { OPCODE(F64Eq); RETURN(COMPARE); }
      <i> "f32.ne"              { OPCODE(F32Ne); RETURN(COMPARE); }
      <i> "f64.ne"              { OPCODE(F64Ne); RETURN(COMPARE); }
      <i> "f32.lt"              { OPCODE(F32Lt); RETURN(COMPARE); }
      <i> "f64.lt"              { OPCODE(F64Lt); RETURN(COMPARE); }
      <i> "f32.le"              { OPCODE(F32Le); RETURN(COMPARE); }
      <i> "f64.le"              { OPCODE(F64Le); RETURN(COMPARE); }
      <i> "f32.gt"              { OPCODE(F32Gt); RETURN(COMPARE); }
      <i> "f64.gt"              { OPCODE(F64Gt); RETURN(COMPARE); }
      <i> "f32.ge"              { OPCODE(F32Ge); RETURN(COMPARE); }
      <i> "f64.ge"              { OPCODE(F64Ge); RETURN(COMPARE); }
      <i> "i64.extend_s/i32"    { OPCODE(I64ExtendSI32); RETURN(CONVERT); }
      <i> "i64.extend_u/i32"    { OPCODE(I64ExtendUI32); RETURN(CONVERT); }
      <i> "i32.wrap/i64"        { OPCODE(I32WrapI64); RETURN(CONVERT); }
      <i> "i32.trunc_s/f32"     { OPCODE(I32TruncSF32); RETURN(CONVERT); }
      <i> "i64.trunc_s/f32"     { OPCODE(I64TruncSF32); RETURN(CONVERT); }
      <i> "i32.trunc_s/f64"     { OPCODE(I32TruncSF64); RETURN(CONVERT); }
      <i> "i64.trunc_s/f64"     { OPCODE(I64TruncSF64); RETURN(CONVERT); }
      <i> "i32.trunc_u/f32"     { OPCODE(I32TruncUF32); RETURN(CONVERT); }
      <i> "i64.trunc_u/f32"     { OPCODE(I64TruncUF32); RETURN(CONVERT); }
      <i> "i32.trunc_u/f64"     { OPCODE(I32TruncUF64); RETURN(CONVERT); }
      <i> "i64.trunc_u/f64"     { OPCODE(I64TruncUF64); RETURN(CONVERT); }
      <i> "f32.convert_s/i32"   { OPCODE(F32ConvertSI32); RETURN(CONVERT); }
      <i> "f64.convert_s/i32"   { OPCODE(F64ConvertSI32); RETURN(CONVERT); }
      <i> "f32.convert_s/i64"   { OPCODE(F32ConvertSI64); RETURN(CONVERT); }
      <i> "f64.convert_s/i64"   { OPCODE(F64ConvertSI64); RETURN(CONVERT); }
      <i> "f32.convert_u/i32"   { OPCODE(F32ConvertUI32); RETURN(CONVERT); }
      <i> "f64.convert_u/i32"   { OPCODE(F64ConvertUI32); RETURN(CONVERT); }
      <i> "f32.convert_u/i64"   { OPCODE(F32ConvertUI64); RETURN(CONVERT); }
      <i> "f64.convert_u/i64"   { OPCODE(F64ConvertUI64); RETURN(CONVERT); }
      <i> "f64.promote/f32"     { OPCODE(F64PromoteF32); RETURN(CONVERT); }
      <i> "f32.demote/f64"      { OPCODE(F32DemoteF64); RETURN(CONVERT); }
      <i> "f32.reinterpret/i32" { OPCODE(F32ReinterpretI32); RETURN(CONVERT); }
      <i> "i32.reinterpret/f32" { OPCODE(I32ReinterpretF32); RETURN(CONVERT); }
      <i> "f64.reinterpret/i64" { OPCODE(F64ReinterpretI64); RETURN(CONVERT); }
      <i> "i64.reinterpret/f64" { OPCODE(I64ReinterpretF64); RETURN(CONVERT); }
      <i> "select"              { RETURN(SELECT); }
      <i> "unreachable"         { RETURN(UNREACHABLE); }
      <i> "current_memory"      { RETURN(CURRENT_MEMORY); }
      <i> "grow_memory"         { RETURN(GROW_MEMORY); }
      <i> "type"                { RETURN(TYPE); }
      <i> "func"                { RETURN(FUNC); }
      <i> "param"               { RETURN(PARAM); }
      <i> "result"              { RETURN(RESULT); }
      <i> "local"               { RETURN(LOCAL); }
      <i> "global"              { RETURN(GLOBAL); }
      <i> "module"              { RETURN(MODULE); }
      <i> "table"               { RETURN(TABLE); }
      <i> "memory"              { RETURN(MEMORY); }
      <i> "start"               { RETURN(START); }
      <i> "elem"                { RETURN(ELEM); }
      <i> "data"                { RETURN(DATA); }
      <i> "offset"              { RETURN(OFFSET); }
      <i> "import"              { RETURN(IMPORT); }
      <i> "export"              { RETURN(EXPORT); }
      <i> "register"            { RETURN(REGISTER); }
      <i> "invoke"              { RETURN(INVOKE); }
      <i> "get"                 { RETURN(GET); }
      <i> "assert_malformed"    { RETURN(ASSERT_MALFORMED); }
      <i> "assert_invalid"      { RETURN(ASSERT_INVALID); }
      <i> "assert_unlinkable"   { RETURN(ASSERT_UNLINKABLE); }
      <i> "assert_return"       { RETURN(ASSERT_RETURN); }
      <i> "assert_return_canonical_nan" {
                                  RETURN(ASSERT_RETURN_CANONICAL_NAN); }
      <i> "assert_return_arithmetic_nan" {
                                  RETURN(ASSERT_RETURN_ARITHMETIC_NAN); }
      <i> "assert_trap"         { RETURN(ASSERT_TRAP); }
      <i> "assert_exhaustion"   { RETURN(ASSERT_EXHAUSTION); }
      <i> "input"               { RETURN(INPUT); }
      <i> "output"              { RETURN(OUTPUT); }
      <i> name                  { TEXT; RETURN(VAR); }

      <i> ";;" => LINE_COMMENT  { continue; }
      <LINE_COMMENT> "\n" => i  { NEWLINE; continue; }
      <LINE_COMMENT> [^\n]*     { continue; }
      <i> "(;" => BLOCK_COMMENT { COMMENT_NESTING = 1; continue; }
      <BLOCK_COMMENT> "(;"      { COMMENT_NESTING++; continue; }
      <BLOCK_COMMENT> ";)"      { if (--COMMENT_NESTING == 0)
                                    BEGIN(YYCOND_INIT);
                                  continue; }
      <BLOCK_COMMENT> "\n"      { NEWLINE; continue; }
      <BLOCK_COMMENT> EOF       { ERROR("unexpected EOF"); RETURN(EOF); }
      <BLOCK_COMMENT> [^]       { continue; }
      <i> "\n"                  { NEWLINE; continue; }
      <i> [ \t\r]+              { continue; }
      <i> atom                  { ERROR("unexpected token \"%.*s\"",
                                        static_cast<int>(yyleng), yytext);
                                  continue; }
      <*> EOF                   { RETURN(EOF); }
      <*> [^]                   { ERROR("unexpected char"); continue; }
     */
  }
}

static AstLexer* new_lexer(AstLexerSourceType type,
                                    const char* filename) {
  AstLexer* lexer = new AstLexer();
  lexer->line = 1;
  lexer->filename = filename;
  lexer->source.type = type;
  return lexer;
}

AstLexer* new_ast_file_lexer(const char* filename) {
  AstLexer* lexer = new_lexer(AstLexerSourceType::File, filename);
  lexer->source.file = fopen(filename, "rb");
  if (!lexer->source.file) {
    destroy_ast_lexer(lexer);
    return nullptr;
  }
  return lexer;
}

AstLexer* new_ast_buffer_lexer(const char* filename,
                                        const void* data,
                                        size_t size) {
  AstLexer* lexer =
      new_lexer(AstLexerSourceType::Buffer, filename);
  lexer->source.buffer.data = data;
  lexer->source.buffer.size = size;
  lexer->source.buffer.read_offset = 0;
  return lexer;
}

void destroy_ast_lexer(AstLexer* lexer) {
  if (lexer->source.type == AstLexerSourceType::File && lexer->source.file)
    fclose(lexer->source.file);
  delete [] lexer->buffer;
  delete lexer;
}

enum class LineOffsetPosition {
  Start,
  End,
};

static Result scan_forward_for_line_offset_in_buffer(
    const char* buffer_start,
    const char* buffer_end,
    int buffer_line,
    size_t buffer_file_offset,
    LineOffsetPosition find_position,
    int find_line,
    int* out_line,
    size_t* out_line_offset) {
  int line = buffer_line;
  int line_offset = 0;
  const char* p;
  bool is_previous_carriage = 0;
  for (p = buffer_start; p < buffer_end; ++p) {
    if (*p == '\n') {
      if (find_position == LineOffsetPosition::Start) {
        if (++line == find_line) {
          line_offset = buffer_file_offset + (p - buffer_start) + 1;
          break;
        }
      } else {
        if (line++ == find_line) {
          line_offset =
              buffer_file_offset + (p - buffer_start) - is_previous_carriage;
          break;
        }
      }
    }
    is_previous_carriage = *p == '\r';
  }

  Result result = Result::Ok;
  if (p == buffer_end) {
    /* end of buffer */
    if (find_position == LineOffsetPosition::Start) {
      result = Result::Error;
    } else {
      line_offset = buffer_file_offset + (buffer_end - buffer_start);
    }
  }

  *out_line = line;
  *out_line_offset = line_offset;
  return result;
}

static Result scan_forward_for_line_offset_in_file(
    AstLexer* lexer,
    int line,
    size_t line_start_offset,
    LineOffsetPosition find_position,
    int find_line,
    size_t* out_line_offset) {
  FILE* lexer_file = lexer->source.file;
  Result result = Result::Error;
  long old_offset = ftell(lexer_file);
  if (old_offset == -1)
    return Result::Error;
  size_t buffer_file_offset = line_start_offset;
  if (fseek(lexer_file, buffer_file_offset, SEEK_SET) == -1)
    goto cleanup;

  while (1) {
    char buffer[8 * 1024];
    const size_t buffer_size = WABT_ARRAY_SIZE(buffer);
    size_t read_bytes = fread(buffer, 1, buffer_size, lexer_file);
    if (read_bytes == 0) {
      /* end of buffer */
      if (find_position == LineOffsetPosition::Start) {
        result = Result::Error;
      } else {
        *out_line_offset = buffer_file_offset + read_bytes;
        result = Result::Ok;
      }
      goto cleanup;
    }

    const char* buffer_end = buffer + read_bytes;
    result = scan_forward_for_line_offset_in_buffer(
        buffer, buffer_end, line, buffer_file_offset, find_position, find_line,
        &line, out_line_offset);
    if (result == Result::Ok)
      goto cleanup;

    buffer_file_offset += read_bytes;
  }

cleanup:
  /* if this fails, we're screwed */
  if (fseek(lexer_file, old_offset, SEEK_SET) == -1)
    return Result::Error;
  return result;
}

static Result scan_forward_for_line_offset(
    AstLexer* lexer,
    int line,
    size_t line_start_offset,
    LineOffsetPosition find_position,
    int find_line,
    size_t* out_line_offset) {
  assert(line <= find_line);
  if (lexer->source.type == AstLexerSourceType::Buffer) {
    const char* source_buffer =
        static_cast<const char*>(lexer->source.buffer.data);
    const char* buffer_start = source_buffer + line_start_offset;
    const char* buffer_end = source_buffer + lexer->source.buffer.size;
    return scan_forward_for_line_offset_in_buffer(
        buffer_start, buffer_end, line, line_start_offset, find_position,
        find_line, &line, out_line_offset);
  } else {
    assert(lexer->source.type == AstLexerSourceType::File);
    return scan_forward_for_line_offset_in_file(lexer, line, line_start_offset,
                                                find_position, find_line,
                                                out_line_offset);
  }
}

static Result get_line_start_offset(AstLexer* lexer,
                                        int line,
                                        size_t* out_offset) {
  int first_line = 1;
  size_t first_offset = 0;
  int current_line = lexer->line;
  size_t current_offset = lexer->line_file_offset;

  if (line == current_line) {
    *out_offset = current_offset;
    return Result::Ok;
  } else if (line == first_line) {
    *out_offset = first_offset;
    return Result::Ok;
  } else if (line > current_line) {
    return scan_forward_for_line_offset(lexer, current_line, current_offset,
                                        LineOffsetPosition::Start, line,
                                        out_offset);
  } else {
    /* TODO(binji): optimize by storing more known line/offset pairs */
    return scan_forward_for_line_offset(lexer, first_line, first_offset,
                                        LineOffsetPosition::Start, line,
                                        out_offset);
  }
}

static Result get_offsets_from_line(AstLexer* lexer,
                                        int line,
                                        size_t* out_line_start,
                                        size_t* out_line_end) {
  size_t line_start;
  if (WABT_FAILED(get_line_start_offset(lexer, line, &line_start)))
    return Result::Error;

  size_t line_end;
  if (WABT_FAILED(scan_forward_for_line_offset(lexer, line, line_start,
                                               LineOffsetPosition::End,
                                               line, &line_end)))
    return Result::Error;
  *out_line_start = line_start;
  *out_line_end = line_end;
  return Result::Ok;
}

static void clamp_source_line_offsets_to_location(size_t line_start,
                                                  size_t line_end,
                                                  int first_column,
                                                  int last_column,
                                                  size_t max_line_length,
                                                  size_t* out_new_line_start,
                                                  size_t* out_new_line_end) {
  size_t line_length = line_end - line_start;
  if (line_length > max_line_length) {
    size_t column_range = last_column - first_column;
    size_t center_on;
    if (column_range > max_line_length) {
      /* the column range doesn't fit, just center on first_column */
      center_on = first_column - 1;
    } else {
      /* the entire range fits, display it all in the center */
      center_on = (first_column + last_column) / 2 - 1;
    }
    if (center_on > max_line_length / 2)
      line_start += center_on - max_line_length / 2;
    if (line_start > line_end - max_line_length)
      line_start = line_end - max_line_length;
    line_end = line_start + max_line_length;
  }

  *out_new_line_start = line_start;
  *out_new_line_end = line_end;
}

Result ast_lexer_get_source_line(AstLexer* lexer,
                                          const Location* loc,
                                          size_t line_max_length,
                                          char* line,
                                          size_t* out_line_length,
                                          int* out_column_offset) {
  Result result;
  size_t line_start; /* inclusive */
  size_t line_end;   /* exclusive */
  result = get_offsets_from_line(lexer, loc->line, &line_start, &line_end);
  if (WABT_FAILED(result))
    return result;

  size_t new_line_start;
  size_t new_line_end;
  clamp_source_line_offsets_to_location(line_start, line_end, loc->first_column,
                                        loc->last_column, line_max_length,
                                        &new_line_start, &new_line_end);
  bool has_start_ellipsis = line_start != new_line_start;
  bool has_end_ellipsis = line_end != new_line_end;

  char* write_start = line;
  size_t line_length = new_line_end - new_line_start;
  size_t read_start = new_line_start;
  size_t read_length = line_length;
  if (has_start_ellipsis) {
    memcpy(line, "...", 3);
    read_start += 3;
    write_start += 3;
    read_length -= 3;
  }
  if (has_end_ellipsis) {
    memcpy(line + line_length - 3, "...", 3);
    read_length -= 3;
  }

  if (lexer->source.type == AstLexerSourceType::Buffer) {
    const char* buffer_read_start =
        static_cast<const char*>(lexer->source.buffer.data) + read_start;
    memcpy(write_start, buffer_read_start, read_length);
  } else {
    assert(lexer->source.type == AstLexerSourceType::File);
    FILE* lexer_file = lexer->source.file;
    long old_offset = ftell(lexer_file);
    if (old_offset == -1)
      return Result::Error;
    if (fseek(lexer_file, read_start, SEEK_SET) == -1)
      return Result::Error;
    if (fread(write_start, 1, read_length, lexer_file) < read_length)
      return Result::Error;
    if (fseek(lexer_file, old_offset, SEEK_SET) == -1)
      return Result::Error;
  }

  line[line_length] = '\0';

  *out_line_length = line_length;
  *out_column_offset = new_line_start - line_start;
  return Result::Ok;
}

}  // namespace wabt

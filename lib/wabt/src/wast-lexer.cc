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

#include "wast-lexer.h"

#include <cassert>
#include <cstdio>

#include "config.h"

#include "lexer-source.h"
#include "wast-parser.h"
#include "wast-parser-lexer-shared.h"

/* must be included after so some typedefs will be defined */
#include "wast-parser-gen.hh"

/*!max:re2c */

#define INITIAL_LEXER_BUFFER_SIZE (64 * 1024)

#define YY_USER_ACTION                  \
  {                                     \
    loc->filename = filename_;          \
    loc->line = line_;                  \
    loc->first_column = COLUMN(token_); \
    loc->last_column = COLUMN(cursor_); \
  }

#define RETURN(name) \
  YY_USER_ACTION;    \
  return WABT_TOKEN_TYPE_##name

#define ERROR(...) \
  YY_USER_ACTION;  \
  wast_parser_error(loc, this, parser, __VA_ARGS__)

#define BEGIN(c) cond = (c)
#define FILL(n)                                \
  do {                                         \
    if (WABT_FAILED(Fill(loc, parser, (n)))) { \
      RETURN(EOF);                             \
    }                                          \
  } while (0)

#define MAYBE_MALFORMED_UTF8(desc)                \
  if (!(eof_ && limit_ - cursor_ <= YYMAXFILL)) { \
    ERROR("malformed utf-8%s", desc);             \
  }                                               \
  continue

#define yytext (token_)
#define yyleng (cursor_ - token_)

/* p must be a pointer somewhere in the lexer buffer */
#define FILE_OFFSET(p) ((p) - (buffer_) + buffer_file_offset_)
#define COLUMN(p) (FILE_OFFSET(p) - line_file_offset_ + 1)

#define COMMENT_NESTING (comment_nesting_)
#define NEWLINE                               \
  do {                                        \
    line_++;                                  \
    line_file_offset_ = FILE_OFFSET(cursor_); \
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

WastLexer::WastLexer(std::unique_ptr<LexerSource> source, const char* filename)
    : source_(std::move(source)),
      line_finder_(source_->Clone()),
      filename_(filename),
      line_(1),
      comment_nesting_(0),
      buffer_file_offset_(0),
      line_file_offset_(0),
      eof_(false),
      buffer_(nullptr),
      buffer_size_(0),
      marker_(nullptr),
      token_(nullptr),
      cursor_(nullptr),
      limit_(nullptr) {}

WastLexer::~WastLexer() {
  delete[] buffer_;
}

// static
std::unique_ptr<WastLexer> WastLexer::CreateFileLexer(const char* filename) {
  std::unique_ptr<LexerSource> source(new LexerSourceFile(filename));
  return std::unique_ptr<WastLexer>(new WastLexer(std::move(source), filename));
}

// static
std::unique_ptr<WastLexer> WastLexer::CreateBufferLexer(const char* filename,
                                                        const void* data,
                                                        size_t size) {
  std::unique_ptr<LexerSource> source(new LexerSourceBuffer(data, size));
  return std::unique_ptr<WastLexer>(new WastLexer(std::move(source), filename));
}

Result WastLexer::Fill(Location* loc, WastParser* parser, size_t need) {
  if (eof_)
    return Result::Error;
  size_t free = token_ - buffer_;
  assert(static_cast<size_t>(cursor_ - buffer_) >= free);
  // Our buffer is too small, need to realloc.
  if (free < need) {
    char* old_buffer = buffer_;
    size_t old_buffer_size = buffer_size_;
    size_t new_buffer_size =
        old_buffer_size ? old_buffer_size * 2 : INITIAL_LEXER_BUFFER_SIZE;
    // Make sure there is enough space for the bytes requested (need) and an
    // additional YYMAXFILL bytes which is needed for the re2c lexer
    // implementation when the eof is reached.
    while ((new_buffer_size - old_buffer_size) + free < need + YYMAXFILL)
      new_buffer_size *= 2;

    char* new_buffer = new char[new_buffer_size];
    if (limit_ > token_)
      memmove(new_buffer, token_, limit_ - token_);
    buffer_ = new_buffer;
    buffer_size_ = new_buffer_size;
    token_ = new_buffer + (token_ - old_buffer) - free;
    marker_ = new_buffer + (marker_ - old_buffer) - free;
    cursor_ = new_buffer + (cursor_ - old_buffer) - free;
    limit_ = new_buffer + (limit_ - old_buffer) - free;
    buffer_file_offset_ += free;
    free += new_buffer_size - old_buffer_size;
    delete[] old_buffer;
  } else {
    // Shift everything down to make more room in the buffer.
    if (limit_ > token_)
      memmove(buffer_, token_, limit_ - token_);
    token_ -= free;
    marker_ -= free;
    cursor_ -= free;
    limit_ -= free;
    buffer_file_offset_ += free;
  }
  // Read the new data into the buffer.
  limit_ += source_->Fill(limit_, free);

  // If at the end of file, need to fill YYMAXFILL more characters with "fake
  // characters", that are not a lexeme nor a lexeme suffix. see
  // http://re2c.org/examples/example_03.html.
  if (limit_ < buffer_ + buffer_size_ - YYMAXFILL) {
    eof_ = true;
    // Fill with 0xff, since that is an invalid utf-8 byte.
    memset(limit_, 0xff, YYMAXFILL);
    limit_ += YYMAXFILL;
  }
  return Result::Ok;
}

int WastLexer::GetToken(Token* lval, Location* loc, WastParser* parser) {
  enum {
    YYCOND_INIT,
    YYCOND_BAD_TEXT,
    YYCOND_LINE_COMMENT,
    YYCOND_BLOCK_COMMENT,
    YYCOND_i = YYCOND_INIT,
  } cond = YYCOND_INIT;

  for (;;) {
    token_ = cursor_;
    /*!re2c
      re2c:condprefix = YYCOND_;
      re2c:condenumprefix = YYCOND_;
      re2c:define:YYCTYPE = "unsigned char";
      re2c:define:YYCURSOR = cursor_;
      re2c:define:YYMARKER = marker_;
      re2c:define:YYLIMIT = limit_;
      re2c:define:YYFILL = "FILL";
      re2c:define:YYGETCONDITION = "cond";
      re2c:define:YYGETCONDITION:naked = 1;
      re2c:define:YYSETCONDITION = "BEGIN";

      digit =     [0-9];
      hexdigit =  [0-9a-fA-F];
      num =       digit+;
      hexnum =    hexdigit+;
      letter =    [a-zA-Z];
      symbol =    [+\-*\/\\\^~=<>!?@#$%&|:`.'];
      character = [^"\\\x00-\x1f]
                | "\\" [nrt\\'"]
                | "\\" hexdigit hexdigit;
      sign =      [+-];
      nat =       num | "0x" hexnum;
      int =       sign nat;
      hexfloat =  sign? "0x" hexnum ("." hexdigit*)? "p" sign? num;
      infinity =  sign? "inf";
      nan =       sign? "nan"
          |       sign? "nan:0x" hexnum;
      float =     sign? num "." digit*
            |     sign? num ("." digit*)? [eE] sign? num;
      text =      '"' character* '"';
      name =      "$" (letter | digit | "_" | symbol)+;

      // Should be ([\x21-\x7e] \ [()"; ])+ , but re2c doesn't like this...
      reserved =  [\x21\x23-\x27\x2a-\x3a\x3c-\x7e]+;  

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
      <BAD_TEXT> [^]            { ERROR("illegal character in string");
                                  continue; }
      <BAD_TEXT> *              { MAYBE_MALFORMED_UTF8(" in string"); }
      <i> "i32"                 { TYPE(I32); RETURN(VALUE_TYPE); }
      <i> "i64"                 { TYPE(I64); RETURN(VALUE_TYPE); }
      <i> "f32"                 { TYPE(F32); RETURN(VALUE_TYPE); }
      <i> "f64"                 { TYPE(F64); RETURN(VALUE_TYPE); }
      <i> "anyfunc"             { RETURN(ANYFUNC); }
      <i> "mut"                 { RETURN(MUT); }
      <i> "nop"                 { RETURN(NOP); }
      <i> "block"               { RETURN(BLOCK); }
      <i> "if"                  { RETURN(IF); }
      <i> "then"                { RETURN(THEN); }
      <i> "else"                { RETURN(ELSE); }
      <i> "loop"                { RETURN(LOOP); }
      <i> "br"                  { RETURN(BR); }
      <i> "br_if"               { RETURN(BR_IF); }
      <i> "br_table"            { RETURN(BR_TABLE); }
      <i> "call"                { RETURN(CALL); }
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
      <i> "binary"              { RETURN(BIN); }
      <i> "quote"               { RETURN(QUOTE); }
      <i> "table"               { RETURN(TABLE); }
      <i> "memory"              { RETURN(MEMORY); }
      <i> "start"               { RETURN(START); }
      <i> "elem"                { RETURN(ELEM); }
      <i> "data"                { RETURN(DATA); }
      <i> "offset"              { RETURN(OFFSET); }
      <i> "import"              { RETURN(IMPORT); }
      <i> "export"              { RETURN(EXPORT); }
      <i> "except"              { RETURN(EXCEPT); }
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
      <i> "try"                 { RETURN(TRY); }
      <i> "catch"               { RETURN(CATCH); }
      <i> "catch_all"           { RETURN(CATCH_ALL); }
      <i> "throw"               { RETURN(THROW); }
      <i> "rethrow"             { RETURN(RETHROW); }
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
      <BLOCK_COMMENT> [^]       { continue; }
      <BLOCK_COMMENT> *         { MAYBE_MALFORMED_UTF8(" in block comment"); }
      <i> "\n"                  { NEWLINE; continue; }
      <i> [ \t\r]+              { continue; }
      <i> reserved              { ERROR("unexpected token \"%.*s\"",
                                        static_cast<int>(yyleng), yytext);
                                  continue; }
      <*> [^]                   { ERROR("unexpected char"); continue; }
      <*> *                     { MAYBE_MALFORMED_UTF8(""); }
     */
  }
}

}  // namespace wabt

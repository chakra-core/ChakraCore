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

#include "src/wast-lexer.h"

#include <cassert>
#include <cstdio>

#include "config.h"

#include "src/error-handler.h"
#include "src/lexer-source.h"
#include "src/wast-parser.h"

/*!max:re2c */

#define INITIAL_LEXER_BUFFER_SIZE (64 * 1024)

#define ERROR(...) parser->Error(GetLocation(), __VA_ARGS__)

#define BEGIN(c) cond = (c)
#define FILL(n)              \
  do {                       \
    if (Failed(Fill((n)))) { \
      RETURN(Eof);           \
    }                        \
  } while (0)

#define MAYBE_MALFORMED_UTF8(desc)                \
  if (!(eof_ && limit_ - cursor_ <= YYMAXFILL)) { \
    ERROR("malformed utf-8%s", desc);             \
  }                                               \
  continue

#define yytext (next_pos_)
#define yyleng (cursor_ - next_pos_)

/* p must be a pointer somewhere in the lexer buffer */
#define FILE_OFFSET(p) ((p) - (buffer_) + buffer_file_offset_)
#define COLUMN(p) (FILE_OFFSET(p) - line_file_offset_ + 1)

#define COMMENT_NESTING (comment_nesting_)
#define NEWLINE                               \
  do {                                        \
    line_++;                                  \
    line_file_offset_ = FILE_OFFSET(cursor_); \
  } while (0)

#define RETURN(token) return Token(GetLocation(), TokenType::token);

#define RETURN_LITERAL(token, literal)          \
  return Token(GetLocation(), TokenType::token, \
               MakeLiteral(LiteralType::literal))

#define RETURN_TYPE(token, type) \
  return Token(GetLocation(), TokenType::token, Type::type)

#define RETURN_OPCODE0(token) \
  return Token(GetLocation(), TokenType::token, Opcode::token)

#define RETURN_OPCODE(token, opcode) \
  return Token(GetLocation(), TokenType::token, Opcode::opcode)

#define RETURN_TEXT(token) \
  return Token(GetLocation(), TokenType::token, GetText())

#define RETURN_TEXT_AT(token, at) \
  return Token(GetLocation(), TokenType::token, GetText(at))

namespace wabt {

WastLexer::WastLexer(std::unique_ptr<LexerSource> source, string_view filename)
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
      next_pos_(nullptr),
      cursor_(nullptr),
      limit_(nullptr) {}

WastLexer::~WastLexer() {
  delete[] buffer_;
}

// static
std::unique_ptr<WastLexer> WastLexer::CreateFileLexer(string_view filename) {
  auto source = MakeUnique<LexerSourceFile>(filename);
  if (!source->IsOpen()) {
    return std::unique_ptr<WastLexer>();
  }
  return MakeUnique<WastLexer>(std::move(source), filename);
}

// static
std::unique_ptr<WastLexer> WastLexer::CreateBufferLexer(string_view filename,
                                                        const void* data,
                                                        size_t size) {
  return MakeUnique<WastLexer>(MakeUnique<LexerSourceBuffer>(data, size),
                               filename);
}

Location WastLexer::GetLocation() {
  return Location(filename_, line_, COLUMN(next_pos_), COLUMN(cursor_));
}

Literal WastLexer::MakeLiteral(LiteralType type) {
  return Literal(type, GetText());
}

std::string WastLexer::GetText(size_t offset) {
  return std::string(yytext + offset, yyleng - offset);
}

Result WastLexer::Fill(size_t need) {
  if (eof_) {
    return Result::Error;
  }
  size_t free = next_pos_ - buffer_;
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
    if (limit_ > next_pos_) {
      memmove(new_buffer, next_pos_, limit_ - next_pos_);
    }
    buffer_ = new_buffer;
    buffer_size_ = new_buffer_size;
    next_pos_ = new_buffer + (next_pos_ - old_buffer) - free;
    marker_ = new_buffer + (marker_ - old_buffer) - free;
    cursor_ = new_buffer + (cursor_ - old_buffer) - free;
    limit_ = new_buffer + (limit_ - old_buffer) - free;
    buffer_file_offset_ += free;
    free += new_buffer_size - old_buffer_size;
    delete[] old_buffer;
  } else {
    // Shift everything down to make more room in the buffer.
    if (limit_ > next_pos_) {
      memmove(buffer_, next_pos_, limit_ - next_pos_);
    }
    next_pos_ -= free;
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

Token WastLexer::GetToken(WastParser* parser) {
  /*!types:re2c*/
  YYCONDTYPE cond = YYCOND_i;  // i is the initial state.

  for (;;) {
    next_pos_ = cursor_;
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
      num =       digit ("_"? digit)*;
      hexnum =    hexdigit ("_"? hexdigit)*;
      letter =    [a-zA-Z];
      symbol =    [+\-*\\/^~=<>!?@#$%&|:`.'];
      character = [^"\\\x00-\x1f]
                | "\\" [nrt\\'"]
                | "\\" hexdigit hexdigit;
      sign =      [+-];
      nat =       num | "0x" hexnum;
      int =       sign nat;
      frac =      num;
      hexfrac =   hexnum;
      hexfloat =  sign? "0x" hexnum "." hexfrac?
               |  sign? "0x" hexnum ("." hexfrac?)? [pP] sign? num;
      infinity =  sign? "inf";
      nan =       sign? "nan"
          |       sign? "nan:0x" hexnum;
      float =     sign? num "." frac?
            |     sign? num ("." frac?)? [eE] sign? num;
      text =      '"' character* '"';
      name =      "$" (letter | digit | "_" | symbol)+;

      // Should be ([\x21-\x7e] \ [()"; ])+ , but re2c doesn't like this...
      reserved =  [\x21\x23-\x27\x2a-\x3a\x3c-\x7e]+;

      <i> "("                   { RETURN(Lpar); }
      <i> ")"                   { RETURN(Rpar); }
      <i> nat                   { RETURN_LITERAL(Nat, Int); }
      <i> int                   { RETURN_LITERAL(Int, Int); }
      <i> float                 { RETURN_LITERAL(Float, Float); }
      <i> hexfloat              { RETURN_LITERAL(Float, Hexfloat); }
      <i> infinity              { RETURN_LITERAL(Float, Infinity); }
      <i> nan                   { RETURN_LITERAL(Float, Nan); }
      <i> text                  { RETURN_TEXT(Text); }
      <i> '"' => BAD_TEXT       { continue; }
      <BAD_TEXT> character      { continue; }
      <BAD_TEXT> "\n" => i      { ERROR("newline in string");
                                  NEWLINE;
                                  continue; }
      <BAD_TEXT> "\\".          { ERROR("bad escape \"%.*s\"",
                                        static_cast<int>(yyleng), yytext);
                                  continue; }
      <BAD_TEXT> '"' => i       { RETURN_TEXT(Text); }
      <BAD_TEXT> [^]            { ERROR("illegal character in string");
                                  continue; }
      <BAD_TEXT> *              { MAYBE_MALFORMED_UTF8(" in string"); }
      <i> "i32"                 { RETURN_TYPE(ValueType, I32); }
      <i> "i64"                 { RETURN_TYPE(ValueType, I64); }
      <i> "f32"                 { RETURN_TYPE(ValueType, F32); }
      <i> "f64"                 { RETURN_TYPE(ValueType, F64); }
      <i> "v128"                { RETURN_TYPE(ValueType, V128); }
      <i> "anyfunc"             { RETURN(Anyfunc); }
      <i> "mut"                 { RETURN(Mut); }
      <i> "nop"                 { RETURN_OPCODE0(Nop); }
      <i> "block"               { RETURN_OPCODE0(Block); }
      <i> "if"                  { RETURN_OPCODE0(If); }
      <i> "then"                { RETURN(Then); }
      <i> "else"                { RETURN_OPCODE0(Else); }
      <i> "loop"                { RETURN_OPCODE0(Loop); }
      <i> "br"                  { RETURN_OPCODE0(Br); }
      <i> "br_if"               { RETURN_OPCODE0(BrIf); }
      <i> "br_table"            { RETURN_OPCODE0(BrTable); }
      <i> "call"                { RETURN_OPCODE0(Call); }
      <i> "call_indirect"       { RETURN_OPCODE0(CallIndirect); }
      <i> "drop"                { RETURN_OPCODE0(Drop); }
      <i> "end"                 { RETURN_OPCODE0(End); }
      <i> "return"              { RETURN_OPCODE0(Return); }
      <i> "get_local"           { RETURN_OPCODE0(GetLocal); }
      <i> "set_local"           { RETURN_OPCODE0(SetLocal); }
      <i> "tee_local"           { RETURN_OPCODE0(TeeLocal); }
      <i> "get_global"          { RETURN_OPCODE0(GetGlobal); }
      <i> "set_global"          { RETURN_OPCODE0(SetGlobal); }
      <i> "i32.load"            { RETURN_OPCODE(Load, I32Load); }
      <i> "i64.load"            { RETURN_OPCODE(Load, I64Load); }
      <i> "f32.load"            { RETURN_OPCODE(Load, F32Load); }
      <i> "f64.load"            { RETURN_OPCODE(Load, F64Load); }
      <i> "i32.store"           { RETURN_OPCODE(Store, I32Store); }
      <i> "i64.store"           { RETURN_OPCODE(Store, I64Store); }
      <i> "f32.store"           { RETURN_OPCODE(Store, F32Store); }
      <i> "f64.store"           { RETURN_OPCODE(Store, F64Store); }
      <i> "i32.load8_s"         { RETURN_OPCODE(Load, I32Load8S); }
      <i> "i64.load8_s"         { RETURN_OPCODE(Load, I64Load8S); }
      <i> "i32.load8_u"         { RETURN_OPCODE(Load, I32Load8U); }
      <i> "i64.load8_u"         { RETURN_OPCODE(Load, I64Load8U); }
      <i> "i32.load16_s"        { RETURN_OPCODE(Load, I32Load16S); }
      <i> "i64.load16_s"        { RETURN_OPCODE(Load, I64Load16S); }
      <i> "i32.load16_u"        { RETURN_OPCODE(Load, I32Load16U); }
      <i> "i64.load16_u"        { RETURN_OPCODE(Load, I64Load16U); }
      <i> "i64.load32_s"        { RETURN_OPCODE(Load, I64Load32S); }
      <i> "i64.load32_u"        { RETURN_OPCODE(Load, I64Load32U); }
      <i> "i32.store8"          { RETURN_OPCODE(Store, I32Store8); }
      <i> "i64.store8"          { RETURN_OPCODE(Store, I64Store8); }
      <i> "i32.store16"         { RETURN_OPCODE(Store, I32Store16); }
      <i> "i64.store16"         { RETURN_OPCODE(Store, I64Store16); }
      <i> "i64.store32"         { RETURN_OPCODE(Store, I64Store32); }
      <i> "offset=" nat         { RETURN_TEXT_AT(OffsetEqNat, 7); }
      <i> "align=" nat          { RETURN_TEXT_AT(AlignEqNat, 6); }
      <i> "i32.const"           { RETURN_OPCODE(Const, I32Const); }
      <i> "i64.const"           { RETURN_OPCODE(Const, I64Const); }
      <i> "f32.const"           { RETURN_OPCODE(Const, F32Const); }
      <i> "f64.const"           { RETURN_OPCODE(Const, F64Const); }
      <i> "i32.eqz"             { RETURN_OPCODE(Convert, I32Eqz); }
      <i> "i64.eqz"             { RETURN_OPCODE(Convert, I64Eqz); }
      <i> "i32.clz"             { RETURN_OPCODE(Unary, I32Clz); }
      <i> "i64.clz"             { RETURN_OPCODE(Unary, I64Clz); }
      <i> "i32.ctz"             { RETURN_OPCODE(Unary, I32Ctz); }
      <i> "i64.ctz"             { RETURN_OPCODE(Unary, I64Ctz); }
      <i> "i32.popcnt"          { RETURN_OPCODE(Unary, I32Popcnt); }
      <i> "i64.popcnt"          { RETURN_OPCODE(Unary, I64Popcnt); }
      <i> "f32.neg"             { RETURN_OPCODE(Unary, F32Neg); }
      <i> "f64.neg"             { RETURN_OPCODE(Unary, F64Neg); }
      <i> "f32.abs"             { RETURN_OPCODE(Unary, F32Abs); }
      <i> "f64.abs"             { RETURN_OPCODE(Unary, F64Abs); }
      <i> "f32.sqrt"            { RETURN_OPCODE(Unary, F32Sqrt); }
      <i> "f64.sqrt"            { RETURN_OPCODE(Unary, F64Sqrt); }
      <i> "f32.ceil"            { RETURN_OPCODE(Unary, F32Ceil); }
      <i> "f64.ceil"            { RETURN_OPCODE(Unary, F64Ceil); }
      <i> "f32.floor"           { RETURN_OPCODE(Unary, F32Floor); }
      <i> "f64.floor"           { RETURN_OPCODE(Unary, F64Floor); }
      <i> "f32.trunc"           { RETURN_OPCODE(Unary, F32Trunc); }
      <i> "f64.trunc"           { RETURN_OPCODE(Unary, F64Trunc); }
      <i> "f32.nearest"         { RETURN_OPCODE(Unary, F32Nearest); }
      <i> "f64.nearest"         { RETURN_OPCODE(Unary, F64Nearest); }
      <i> "i32.extend8_s"       { RETURN_OPCODE(Unary, I32Extend8S); }
      <i> "i32.extend16_s"      { RETURN_OPCODE(Unary, I32Extend16S); }
      <i> "i64.extend8_s"       { RETURN_OPCODE(Unary, I64Extend8S); }
      <i> "i64.extend16_s"      { RETURN_OPCODE(Unary, I64Extend16S); }
      <i> "i64.extend32_s"      { RETURN_OPCODE(Unary, I64Extend32S); }
      <i> "i32.add"             { RETURN_OPCODE(Binary, I32Add); }
      <i> "i64.add"             { RETURN_OPCODE(Binary, I64Add); }
      <i> "i32.sub"             { RETURN_OPCODE(Binary, I32Sub); }
      <i> "i64.sub"             { RETURN_OPCODE(Binary, I64Sub); }
      <i> "i32.mul"             { RETURN_OPCODE(Binary, I32Mul); }
      <i> "i64.mul"             { RETURN_OPCODE(Binary, I64Mul); }
      <i> "i32.div_s"           { RETURN_OPCODE(Binary, I32DivS); }
      <i> "i64.div_s"           { RETURN_OPCODE(Binary, I64DivS); }
      <i> "i32.div_u"           { RETURN_OPCODE(Binary, I32DivU); }
      <i> "i64.div_u"           { RETURN_OPCODE(Binary, I64DivU); }
      <i> "i32.rem_s"           { RETURN_OPCODE(Binary, I32RemS); }
      <i> "i64.rem_s"           { RETURN_OPCODE(Binary, I64RemS); }
      <i> "i32.rem_u"           { RETURN_OPCODE(Binary, I32RemU); }
      <i> "i64.rem_u"           { RETURN_OPCODE(Binary, I64RemU); }
      <i> "i32.and"             { RETURN_OPCODE(Binary, I32And); }
      <i> "i64.and"             { RETURN_OPCODE(Binary, I64And); }
      <i> "i32.or"              { RETURN_OPCODE(Binary, I32Or); }
      <i> "i64.or"              { RETURN_OPCODE(Binary, I64Or); }
      <i> "i32.xor"             { RETURN_OPCODE(Binary, I32Xor); }
      <i> "i64.xor"             { RETURN_OPCODE(Binary, I64Xor); }
      <i> "i32.shl"             { RETURN_OPCODE(Binary, I32Shl); }
      <i> "i64.shl"             { RETURN_OPCODE(Binary, I64Shl); }
      <i> "i32.shr_s"           { RETURN_OPCODE(Binary, I32ShrS); }
      <i> "i64.shr_s"           { RETURN_OPCODE(Binary, I64ShrS); }
      <i> "i32.shr_u"           { RETURN_OPCODE(Binary, I32ShrU); }
      <i> "i64.shr_u"           { RETURN_OPCODE(Binary, I64ShrU); }
      <i> "i32.rotl"            { RETURN_OPCODE(Binary, I32Rotl); }
      <i> "i64.rotl"            { RETURN_OPCODE(Binary, I64Rotl); }
      <i> "i32.rotr"            { RETURN_OPCODE(Binary, I32Rotr); }
      <i> "i64.rotr"            { RETURN_OPCODE(Binary, I64Rotr); }
      <i> "f32.add"             { RETURN_OPCODE(Binary, F32Add); }
      <i> "f64.add"             { RETURN_OPCODE(Binary, F64Add); }
      <i> "f32.sub"             { RETURN_OPCODE(Binary, F32Sub); }
      <i> "f64.sub"             { RETURN_OPCODE(Binary, F64Sub); }
      <i> "f32.mul"             { RETURN_OPCODE(Binary, F32Mul); }
      <i> "f64.mul"             { RETURN_OPCODE(Binary, F64Mul); }
      <i> "f32.div"             { RETURN_OPCODE(Binary, F32Div); }
      <i> "f64.div"             { RETURN_OPCODE(Binary, F64Div); }
      <i> "f32.min"             { RETURN_OPCODE(Binary, F32Min); }
      <i> "f64.min"             { RETURN_OPCODE(Binary, F64Min); }
      <i> "f32.max"             { RETURN_OPCODE(Binary, F32Max); }
      <i> "f64.max"             { RETURN_OPCODE(Binary, F64Max); }
      <i> "f32.copysign"        { RETURN_OPCODE(Binary, F32Copysign); }
      <i> "f64.copysign"        { RETURN_OPCODE(Binary, F64Copysign); }
      <i> "i32.eq"              { RETURN_OPCODE(Compare, I32Eq); }
      <i> "i64.eq"              { RETURN_OPCODE(Compare, I64Eq); }
      <i> "i32.ne"              { RETURN_OPCODE(Compare, I32Ne); }
      <i> "i64.ne"              { RETURN_OPCODE(Compare, I64Ne); }
      <i> "i32.lt_s"            { RETURN_OPCODE(Compare, I32LtS); }
      <i> "i64.lt_s"            { RETURN_OPCODE(Compare, I64LtS); }
      <i> "i32.lt_u"            { RETURN_OPCODE(Compare, I32LtU); }
      <i> "i64.lt_u"            { RETURN_OPCODE(Compare, I64LtU); }
      <i> "i32.le_s"            { RETURN_OPCODE(Compare, I32LeS); }
      <i> "i64.le_s"            { RETURN_OPCODE(Compare, I64LeS); }
      <i> "i32.le_u"            { RETURN_OPCODE(Compare, I32LeU); }
      <i> "i64.le_u"            { RETURN_OPCODE(Compare, I64LeU); }
      <i> "i32.gt_s"            { RETURN_OPCODE(Compare, I32GtS); }
      <i> "i64.gt_s"            { RETURN_OPCODE(Compare, I64GtS); }
      <i> "i32.gt_u"            { RETURN_OPCODE(Compare, I32GtU); }
      <i> "i64.gt_u"            { RETURN_OPCODE(Compare, I64GtU); }
      <i> "i32.ge_s"            { RETURN_OPCODE(Compare, I32GeS); }
      <i> "i64.ge_s"            { RETURN_OPCODE(Compare, I64GeS); }
      <i> "i32.ge_u"            { RETURN_OPCODE(Compare, I32GeU); }
      <i> "i64.ge_u"            { RETURN_OPCODE(Compare, I64GeU); }
      <i> "f32.eq"              { RETURN_OPCODE(Compare, F32Eq); }
      <i> "f64.eq"              { RETURN_OPCODE(Compare, F64Eq); }
      <i> "f32.ne"              { RETURN_OPCODE(Compare, F32Ne); }
      <i> "f64.ne"              { RETURN_OPCODE(Compare, F64Ne); }
      <i> "f32.lt"              { RETURN_OPCODE(Compare, F32Lt); }
      <i> "f64.lt"              { RETURN_OPCODE(Compare, F64Lt); }
      <i> "f32.le"              { RETURN_OPCODE(Compare, F32Le); }
      <i> "f64.le"              { RETURN_OPCODE(Compare, F64Le); }
      <i> "f32.gt"              { RETURN_OPCODE(Compare, F32Gt); }
      <i> "f64.gt"              { RETURN_OPCODE(Compare, F64Gt); }
      <i> "f32.ge"              { RETURN_OPCODE(Compare, F32Ge); }
      <i> "f64.ge"              { RETURN_OPCODE(Compare, F64Ge); }
      <i> "i64.extend_s/i32"    { RETURN_OPCODE(Convert, I64ExtendSI32); }
      <i> "i64.extend_u/i32"    { RETURN_OPCODE(Convert, I64ExtendUI32); }
      <i> "i32.wrap/i64"        { RETURN_OPCODE(Convert, I32WrapI64); }
      <i> "i32.trunc_s/f32"     { RETURN_OPCODE(Convert, I32TruncSF32); }
      <i> "i64.trunc_s/f32"     { RETURN_OPCODE(Convert, I64TruncSF32); }
      <i> "i32.trunc_s/f64"     { RETURN_OPCODE(Convert, I32TruncSF64); }
      <i> "i64.trunc_s/f64"     { RETURN_OPCODE(Convert, I64TruncSF64); }
      <i> "i32.trunc_u/f32"     { RETURN_OPCODE(Convert, I32TruncUF32); }
      <i> "i64.trunc_u/f32"     { RETURN_OPCODE(Convert, I64TruncUF32); }
      <i> "i32.trunc_u/f64"     { RETURN_OPCODE(Convert, I32TruncUF64); }
      <i> "i64.trunc_u/f64"     { RETURN_OPCODE(Convert, I64TruncUF64); }
      <i> "i32.trunc_s:sat/f32" { RETURN_OPCODE(Convert, I32TruncSSatF32); }
      <i> "i64.trunc_s:sat/f32" { RETURN_OPCODE(Convert, I64TruncSSatF32); }
      <i> "i32.trunc_s:sat/f64" { RETURN_OPCODE(Convert, I32TruncSSatF64); }
      <i> "i64.trunc_s:sat/f64" { RETURN_OPCODE(Convert, I64TruncSSatF64); }
      <i> "i32.trunc_u:sat/f32" { RETURN_OPCODE(Convert, I32TruncUSatF32); }
      <i> "i64.trunc_u:sat/f32" { RETURN_OPCODE(Convert, I64TruncUSatF32); }
      <i> "i32.trunc_u:sat/f64" { RETURN_OPCODE(Convert, I32TruncUSatF64); }
      <i> "i64.trunc_u:sat/f64" { RETURN_OPCODE(Convert, I64TruncUSatF64); }
      <i> "f32.convert_s/i32"   { RETURN_OPCODE(Convert, F32ConvertSI32); }
      <i> "f64.convert_s/i32"   { RETURN_OPCODE(Convert, F64ConvertSI32); }
      <i> "f32.convert_s/i64"   { RETURN_OPCODE(Convert, F32ConvertSI64); }
      <i> "f64.convert_s/i64"   { RETURN_OPCODE(Convert, F64ConvertSI64); }
      <i> "f32.convert_u/i32"   { RETURN_OPCODE(Convert, F32ConvertUI32); }
      <i> "f64.convert_u/i32"   { RETURN_OPCODE(Convert, F64ConvertUI32); }
      <i> "f32.convert_u/i64"   { RETURN_OPCODE(Convert, F32ConvertUI64); }
      <i> "f64.convert_u/i64"   { RETURN_OPCODE(Convert, F64ConvertUI64); }
      <i> "f64.promote/f32"     { RETURN_OPCODE(Convert, F64PromoteF32); }
      <i> "f32.demote/f64"      { RETURN_OPCODE(Convert, F32DemoteF64); }
      <i> "f32.reinterpret/i32" { RETURN_OPCODE(Convert, F32ReinterpretI32); }
      <i> "i32.reinterpret/f32" { RETURN_OPCODE(Convert, I32ReinterpretF32); }
      <i> "f64.reinterpret/i64" { RETURN_OPCODE(Convert, F64ReinterpretI64); }
      <i> "i64.reinterpret/f64" { RETURN_OPCODE(Convert, I64ReinterpretF64); }
      <i> "select"              { RETURN_OPCODE0(Select); }
      <i> "unreachable"         { RETURN_OPCODE0(Unreachable); }
      <i> "memory.size"         { RETURN_OPCODE0(MemorySize); }
      <i> "memory.grow"         { RETURN_OPCODE0(MemoryGrow); }
      <i> "current_memory"      { RETURN_OPCODE0(MemorySize); }
      <i> "grow_memory"         { RETURN_OPCODE0(MemoryGrow); }

      <i> "i32.atomic.wait"     { RETURN_OPCODE(AtomicWait, I32AtomicWait); }
      <i> "i64.atomic.wait"     { RETURN_OPCODE(AtomicWait, I64AtomicWait); }
      <i> "atomic.wake"         { RETURN_OPCODE0(AtomicWake); }
      <i> "i32.atomic.load"     { RETURN_OPCODE(AtomicLoad, I32AtomicLoad); }
      <i> "i64.atomic.load"     { RETURN_OPCODE(AtomicLoad, I64AtomicLoad); }
      <i> "i32.atomic.load8_u"  { RETURN_OPCODE(AtomicLoad, I32AtomicLoad8U); }
      <i> "i32.atomic.load16_u" { RETURN_OPCODE(AtomicLoad, I32AtomicLoad16U); }
      <i> "i64.atomic.load8_u"  { RETURN_OPCODE(AtomicLoad, I64AtomicLoad8U); }
      <i> "i64.atomic.load16_u" { RETURN_OPCODE(AtomicLoad, I64AtomicLoad16U); }
      <i> "i64.atomic.load32_u" { RETURN_OPCODE(AtomicLoad, I64AtomicLoad32U); }
      <i> "i32.atomic.store"    { RETURN_OPCODE(AtomicStore, I32AtomicStore); }
      <i> "i64.atomic.store"    { RETURN_OPCODE(AtomicStore, I64AtomicStore); }
      <i> "i32.atomic.store8"   { RETURN_OPCODE(AtomicStore, I32AtomicStore8); }
      <i> "i32.atomic.store16"  { RETURN_OPCODE(AtomicStore, I32AtomicStore16); }
      <i> "i64.atomic.store8"   { RETURN_OPCODE(AtomicStore, I64AtomicStore8); }
      <i> "i64.atomic.store16"  { RETURN_OPCODE(AtomicStore, I64AtomicStore16); }
      <i> "i64.atomic.store32"  { RETURN_OPCODE(AtomicStore, I64AtomicStore32); }
      <i> "i32.atomic.rmw.add"         { RETURN_OPCODE(AtomicRmw, I32AtomicRmwAdd); }
      <i> "i64.atomic.rmw.add"         { RETURN_OPCODE(AtomicRmw, I64AtomicRmwAdd); }
      <i> "i32.atomic.rmw8_u.add"      { RETURN_OPCODE(AtomicRmw, I32AtomicRmw8UAdd); }
      <i> "i32.atomic.rmw16_u.add"     { RETURN_OPCODE(AtomicRmw, I32AtomicRmw16UAdd); }
      <i> "i64.atomic.rmw8_u.add"      { RETURN_OPCODE(AtomicRmw, I64AtomicRmw8UAdd); }
      <i> "i64.atomic.rmw16_u.add"     { RETURN_OPCODE(AtomicRmw, I64AtomicRmw16UAdd); }
      <i> "i64.atomic.rmw32_u.add"     { RETURN_OPCODE(AtomicRmw, I64AtomicRmw32UAdd); }
      <i> "i32.atomic.rmw.sub"         { RETURN_OPCODE(AtomicRmw, I32AtomicRmwSub); }
      <i> "i64.atomic.rmw.sub"         { RETURN_OPCODE(AtomicRmw, I64AtomicRmwSub); }
      <i> "i32.atomic.rmw8_u.sub"      { RETURN_OPCODE(AtomicRmw, I32AtomicRmw8USub); }
      <i> "i32.atomic.rmw16_u.sub"     { RETURN_OPCODE(AtomicRmw, I32AtomicRmw16USub); }
      <i> "i64.atomic.rmw8_u.sub"      { RETURN_OPCODE(AtomicRmw, I64AtomicRmw8USub); }
      <i> "i64.atomic.rmw16_u.sub"     { RETURN_OPCODE(AtomicRmw, I64AtomicRmw16USub); }
      <i> "i64.atomic.rmw32_u.sub"     { RETURN_OPCODE(AtomicRmw, I64AtomicRmw32USub); }
      <i> "i32.atomic.rmw.and"         { RETURN_OPCODE(AtomicRmw, I32AtomicRmwAnd); }
      <i> "i64.atomic.rmw.and"         { RETURN_OPCODE(AtomicRmw, I64AtomicRmwAnd); }
      <i> "i32.atomic.rmw8_u.and"      { RETURN_OPCODE(AtomicRmw, I32AtomicRmw8UAnd); }
      <i> "i32.atomic.rmw16_u.and"     { RETURN_OPCODE(AtomicRmw, I32AtomicRmw16UAnd); }
      <i> "i64.atomic.rmw8_u.and"      { RETURN_OPCODE(AtomicRmw, I64AtomicRmw8UAnd); }
      <i> "i64.atomic.rmw16_u.and"     { RETURN_OPCODE(AtomicRmw, I64AtomicRmw16UAnd); }
      <i> "i64.atomic.rmw32_u.and"     { RETURN_OPCODE(AtomicRmw, I64AtomicRmw32UAnd); }
      <i> "i32.atomic.rmw.or"          { RETURN_OPCODE(AtomicRmw, I32AtomicRmwOr); }
      <i> "i64.atomic.rmw.or"          { RETURN_OPCODE(AtomicRmw, I64AtomicRmwOr); }
      <i> "i32.atomic.rmw8_u.or"       { RETURN_OPCODE(AtomicRmw, I32AtomicRmw8UOr); }
      <i> "i32.atomic.rmw16_u.or"      { RETURN_OPCODE(AtomicRmw, I32AtomicRmw16UOr); }
      <i> "i64.atomic.rmw8_u.or"       { RETURN_OPCODE(AtomicRmw, I64AtomicRmw8UOr); }
      <i> "i64.atomic.rmw16_u.or"      { RETURN_OPCODE(AtomicRmw, I64AtomicRmw16UOr); }
      <i> "i64.atomic.rmw32_u.or"      { RETURN_OPCODE(AtomicRmw, I64AtomicRmw32UOr); }
      <i> "i32.atomic.rmw.xor"         { RETURN_OPCODE(AtomicRmw, I32AtomicRmwXor); }
      <i> "i64.atomic.rmw.xor"         { RETURN_OPCODE(AtomicRmw, I64AtomicRmwXor); }
      <i> "i32.atomic.rmw8_u.xor"      { RETURN_OPCODE(AtomicRmw, I32AtomicRmw8UXor); }
      <i> "i32.atomic.rmw16_u.xor"     { RETURN_OPCODE(AtomicRmw, I32AtomicRmw16UXor); }
      <i> "i64.atomic.rmw8_u.xor"      { RETURN_OPCODE(AtomicRmw, I64AtomicRmw8UXor); }
      <i> "i64.atomic.rmw16_u.xor"     { RETURN_OPCODE(AtomicRmw, I64AtomicRmw16UXor); }
      <i> "i64.atomic.rmw32_u.xor"     { RETURN_OPCODE(AtomicRmw, I64AtomicRmw32UXor); }
      <i> "i32.atomic.rmw.xchg"        { RETURN_OPCODE(AtomicRmw, I32AtomicRmwXchg); }
      <i> "i64.atomic.rmw.xchg"        { RETURN_OPCODE(AtomicRmw, I64AtomicRmwXchg); }
      <i> "i32.atomic.rmw8_u.xchg"     { RETURN_OPCODE(AtomicRmw, I32AtomicRmw8UXchg); }
      <i> "i32.atomic.rmw16_u.xchg"    { RETURN_OPCODE(AtomicRmw, I32AtomicRmw16UXchg); }
      <i> "i64.atomic.rmw8_u.xchg"     { RETURN_OPCODE(AtomicRmw, I64AtomicRmw8UXchg); }
      <i> "i64.atomic.rmw16_u.xchg"    { RETURN_OPCODE(AtomicRmw, I64AtomicRmw16UXchg); }
      <i> "i64.atomic.rmw32_u.xchg"    { RETURN_OPCODE(AtomicRmw, I64AtomicRmw32UXchg); }
      <i> "i32.atomic.rmw.cmpxchg"     { RETURN_OPCODE(AtomicRmwCmpxchg, I32AtomicRmwCmpxchg); }
      <i> "i64.atomic.rmw.cmpxchg"     { RETURN_OPCODE(AtomicRmwCmpxchg, I64AtomicRmwCmpxchg); }
      <i> "i32.atomic.rmw8_u.cmpxchg"  { RETURN_OPCODE(AtomicRmwCmpxchg, I32AtomicRmw8UCmpxchg); }
      <i> "i32.atomic.rmw16_u.cmpxchg" { RETURN_OPCODE(AtomicRmwCmpxchg, I32AtomicRmw16UCmpxchg); }
      <i> "i64.atomic.rmw8_u.cmpxchg"  { RETURN_OPCODE(AtomicRmwCmpxchg, I64AtomicRmw8UCmpxchg); }
      <i> "i64.atomic.rmw16_u.cmpxchg" { RETURN_OPCODE(AtomicRmwCmpxchg, I64AtomicRmw16UCmpxchg); }
      <i> "i64.atomic.rmw32_u.cmpxchg" { RETURN_OPCODE(AtomicRmwCmpxchg, I64AtomicRmw32UCmpxchg); }
      <i> "v128.const"           { RETURN_OPCODE(Const, V128Const); }
      <i> "v128.load"            { RETURN_OPCODE(Load,  V128Load); }
      <i> "v128.store"           { RETURN_OPCODE(Store, V128Store); }
      <i> "i8x16.splat"          { RETURN_OPCODE(Unary, I8X16Splat); }
      <i> "i16x8.splat"          { RETURN_OPCODE(Unary, I16X8Splat); }
      <i> "i32x4.splat"          { RETURN_OPCODE(Unary, I32X4Splat); }
      <i> "i64x2.splat"          { RETURN_OPCODE(Unary, I64X2Splat); }
      <i> "f32x4.splat"          { RETURN_OPCODE(Unary, F32X4Splat); }
      <i> "f64x2.splat"          { RETURN_OPCODE(Unary, F64X2Splat); }
      <i> "i8x16.extract_lane_s" { RETURN_OPCODE(SimdLaneOp, I8X16ExtractLaneS); }
      <i> "i8x16.extract_lane_u" { RETURN_OPCODE(SimdLaneOp, I8X16ExtractLaneU); }
      <i> "i16x8.extract_lane_s" { RETURN_OPCODE(SimdLaneOp, I16X8ExtractLaneS); }
      <i> "i16x8.extract_lane_u" { RETURN_OPCODE(SimdLaneOp, I16X8ExtractLaneU); }
      <i> "i32x4.extract_lane"   { RETURN_OPCODE(SimdLaneOp, I32X4ExtractLane); }
      <i> "i64x2.extract_lane"   { RETURN_OPCODE(SimdLaneOp, I64X2ExtractLane); }
      <i> "f32x4.extract_lane"   { RETURN_OPCODE(SimdLaneOp, F32X4ExtractLane); }
      <i> "f64x2.extract_lane"   { RETURN_OPCODE(SimdLaneOp, F64X2ExtractLane); }
      <i> "i8x16.replace_lane"   { RETURN_OPCODE(SimdLaneOp, I8X16ReplaceLane); }
      <i> "i16x8.replace_lane"   { RETURN_OPCODE(SimdLaneOp, I16X8ReplaceLane); }
      <i> "i32x4.replace_lane"   { RETURN_OPCODE(SimdLaneOp, I32X4ReplaceLane); }
      <i> "i64x2.replace_lane"   { RETURN_OPCODE(SimdLaneOp, I64X2ReplaceLane); }
      <i> "f32x4.replace_lane"   { RETURN_OPCODE(SimdLaneOp, F32X4ReplaceLane); }
      <i> "f64x2.replace_lane"   { RETURN_OPCODE(SimdLaneOp, F64X2ReplaceLane); }
      <i> "v8x16.shuffle"        { RETURN_OPCODE(SimdShuffleOp, V8X16Shuffle); }
      <i> "i8x16.add"            { RETURN_OPCODE(Binary, I8X16Add); }
      <i> "i16x8.add"            { RETURN_OPCODE(Binary, I16X8Add); }
      <i> "i32x4.add"            { RETURN_OPCODE(Binary, I32X4Add); }
      <i> "i64x2.add"            { RETURN_OPCODE(Binary, I64X2Add); }
      <i> "i8x16.sub"            { RETURN_OPCODE(Binary, I8X16Sub); }
      <i> "i16x8.sub"            { RETURN_OPCODE(Binary, I16X8Sub); }
      <i> "i32x4.sub"            { RETURN_OPCODE(Binary, I32X4Sub); }
      <i> "i64x2.sub"            { RETURN_OPCODE(Binary, I64X2Sub); }
      <i> "i8x16.mul"            { RETURN_OPCODE(Binary, I8X16Mul); }
      <i> "i16x8.mul"            { RETURN_OPCODE(Binary, I16X8Mul); }
      <i> "i32x4.mul"            { RETURN_OPCODE(Binary, I32X4Mul); }
      <i> "i8x16.neg"            { RETURN_OPCODE(Unary, I8X16Neg); }
      <i> "i16x8.neg"            { RETURN_OPCODE(Unary, I16X8Neg); }
      <i> "i32x4.neg"            { RETURN_OPCODE(Unary, I32X4Neg); }
      <i> "i64x2.neg"            { RETURN_OPCODE(Unary, I64X2Neg); }
      <i> "i8x16.add_saturate_s" { RETURN_OPCODE(Binary, I8X16AddSaturateS); }
      <i> "i8x16.add_saturate_u" { RETURN_OPCODE(Binary, I8X16AddSaturateU); }
      <i> "i16x8.add_saturate_s" { RETURN_OPCODE(Binary, I16X8AddSaturateS); }
      <i> "i16x8.add_saturate_u" { RETURN_OPCODE(Binary, I16X8AddSaturateU); }
      <i> "i8x16.sub_saturate_s" { RETURN_OPCODE(Binary, I8X16SubSaturateS); }
      <i> "i8x16.sub_saturate_u" { RETURN_OPCODE(Binary, I8X16SubSaturateU); }
      <i> "i16x8.sub_saturate_s" { RETURN_OPCODE(Binary, I16X8SubSaturateS); }
      <i> "i16x8.sub_saturate_u" { RETURN_OPCODE(Binary, I16X8SubSaturateU); }
      <i> "i8x16.shl"            { RETURN_OPCODE(Binary, I8X16Shl); }
      <i> "i16x8.shl"            { RETURN_OPCODE(Binary, I16X8Shl); }
      <i> "i32x4.shl"            { RETURN_OPCODE(Binary, I32X4Shl); }
      <i> "i64x2.shl"            { RETURN_OPCODE(Binary, I64X2Shl); }
      <i> "i8x16.shr_s"          { RETURN_OPCODE(Binary, I8X16ShrS); }
      <i> "i8x16.shr_u"          { RETURN_OPCODE(Binary, I8X16ShrU); }
      <i> "i16x8.shr_s"          { RETURN_OPCODE(Binary, I16X8ShrS); }
      <i> "i16x8.shr_u"          { RETURN_OPCODE(Binary, I16X8ShrU); }
      <i> "i32x4.shr_s"          { RETURN_OPCODE(Binary, I32X4ShrS); }
      <i> "i32x4.shr_u"          { RETURN_OPCODE(Binary, I32X4ShrU); }
      <i> "i64x2.shr_s"          { RETURN_OPCODE(Binary, I64X2ShrS); }
      <i> "i64x2.shr_u"          { RETURN_OPCODE(Binary, I64X2ShrU); }
      <i> "v128.and"             { RETURN_OPCODE(Binary, V128And); }
      <i> "v128.or"              { RETURN_OPCODE(Binary, V128Or); }
      <i> "v128.xor"             { RETURN_OPCODE(Binary, V128Xor); }
      <i> "v128.not"             { RETURN_OPCODE(Unary, V128Not); }
      <i> "v128.bitselect"       { RETURN_OPCODE(Ternary, V128BitSelect); }
      <i> "i8x16.any_true"       { RETURN_OPCODE(Unary,  I8X16AnyTrue); }
      <i> "i16x8.any_true"       { RETURN_OPCODE(Unary,  I16X8AnyTrue); }
      <i> "i32x4.any_true"       { RETURN_OPCODE(Unary,  I32X4AnyTrue); }
      <i> "i64x2.any_true"       { RETURN_OPCODE(Unary,  I64X2AnyTrue); }
      <i> "i8x16.all_true"       { RETURN_OPCODE(Unary,  I8X16AllTrue); }
      <i> "i16x8.all_true"       { RETURN_OPCODE(Unary,  I16X8AllTrue); }
      <i> "i32x4.all_true"       { RETURN_OPCODE(Unary,  I32X4AllTrue); }
      <i> "i64x2.all_true"       { RETURN_OPCODE(Unary,  I64X2AllTrue); }
      <i> "i8x16.eq"             { RETURN_OPCODE(Compare, I8X16Eq); }
      <i> "i16x8.eq"             { RETURN_OPCODE(Compare, I16X8Eq); }
      <i> "i32x4.eq"             { RETURN_OPCODE(Compare, I32X4Eq); }
      <i> "f32x4.eq"             { RETURN_OPCODE(Compare, F32X4Eq); }
      <i> "f64x2.eq"             { RETURN_OPCODE(Compare, F64X2Eq); }
      <i> "i8x16.ne"             { RETURN_OPCODE(Compare, I8X16Ne); }
      <i> "i16x8.ne"             { RETURN_OPCODE(Compare, I16X8Ne); }
      <i> "i32x4.ne"             { RETURN_OPCODE(Compare, I32X4Ne); }
      <i> "f32x4.ne"             { RETURN_OPCODE(Compare, F32X4Ne); }
      <i> "f64x2.ne"             { RETURN_OPCODE(Compare, F64X2Ne); }
      <i> "i8x16.lt_s"           { RETURN_OPCODE(Compare, I8X16LtS); }
      <i> "i8x16.lt_u"           { RETURN_OPCODE(Compare, I8X16LtU); }
      <i> "i16x8.lt_s"           { RETURN_OPCODE(Compare, I16X8LtS); }
      <i> "i16x8.lt_u"           { RETURN_OPCODE(Compare, I16X8LtU); }
      <i> "i32x4.lt_s"           { RETURN_OPCODE(Compare, I32X4LtS); }
      <i> "i32x4.lt_u"           { RETURN_OPCODE(Compare, I32X4LtU); }
      <i> "f32x4.lt"             { RETURN_OPCODE(Compare, F32X4Lt); }
      <i> "f64x2.lt"             { RETURN_OPCODE(Compare, F64X2Lt); }
      <i> "i8x16.le_s"           { RETURN_OPCODE(Compare, I8X16LeS); }
      <i> "i8x16.le_u"           { RETURN_OPCODE(Compare, I8X16LeU); }
      <i> "i16x8.le_s"           { RETURN_OPCODE(Compare, I16X8LeS); }
      <i> "i16x8.le_u"           { RETURN_OPCODE(Compare, I16X8LeU); }
      <i> "i32x4.le_s"           { RETURN_OPCODE(Compare, I32X4LeS); }
      <i> "i32x4.le_u"           { RETURN_OPCODE(Compare, I32X4LeU); }
      <i> "f32x4.le"             { RETURN_OPCODE(Compare, F32X4Le); }
      <i> "f64x2.le"             { RETURN_OPCODE(Compare, F64X2Le); }
      <i> "i8x16.gt_s"           { RETURN_OPCODE(Compare, I8X16GtS); }
      <i> "i8x16.gt_u"           { RETURN_OPCODE(Compare, I8X16GtU); }
      <i> "i16x8.gt_s"           { RETURN_OPCODE(Compare, I16X8GtS); }
      <i> "i16x8.gt_u"           { RETURN_OPCODE(Compare, I16X8GtU); }
      <i> "i32x4.gt_s"           { RETURN_OPCODE(Compare, I32X4GtS); }
      <i> "i32x4.gt_u"           { RETURN_OPCODE(Compare, I32X4GtU); }
      <i> "f32x4.gt"             { RETURN_OPCODE(Compare, F32X4Gt); }
      <i> "f64x2.gt"             { RETURN_OPCODE(Compare, F64X2Gt); }
      <i> "i8x16.ge_s"           { RETURN_OPCODE(Compare, I8X16GeS); }
      <i> "i8x16.ge_u"           { RETURN_OPCODE(Compare, I8X16GeU); }
      <i> "i16x8.ge_s"           { RETURN_OPCODE(Compare, I16X8GeS); }
      <i> "i16x8.ge_u"           { RETURN_OPCODE(Compare, I16X8GeU); }
      <i> "i32x4.ge_s"           { RETURN_OPCODE(Compare, I32X4GeS); }
      <i> "i32x4.ge_u"           { RETURN_OPCODE(Compare, I32X4GeU); }
      <i> "f32x4.ge"             { RETURN_OPCODE(Compare, F32X4Ge); }
      <i> "f64x2.ge"             { RETURN_OPCODE(Compare, F64X2Ge); }
      <i> "f32x4.neg"            { RETURN_OPCODE(Unary, F32X4Neg); }
      <i> "f64x2.neg"            { RETURN_OPCODE(Unary, F64X2Neg); }
      <i> "f32x4.abs"            { RETURN_OPCODE(Unary, F32X4Abs); }
      <i> "f64x2.abs"            { RETURN_OPCODE(Unary, F64X2Abs); }
      <i> "f32x4.min"            { RETURN_OPCODE(Binary, F32X4Min); }
      <i> "f64x2.min"            { RETURN_OPCODE(Binary, F64X2Min); }
      <i> "f32x4.max"            { RETURN_OPCODE(Binary, F32X4Max); }
      <i> "f64x2.max"            { RETURN_OPCODE(Binary, F64X2Max); }
      <i> "f32x4.add"            { RETURN_OPCODE(Binary, F32X4Add); }
      <i> "f64x2.add"            { RETURN_OPCODE(Binary, F64X2Add); }
      <i> "f32x4.sub"            { RETURN_OPCODE(Binary, F32X4Sub); }
      <i> "f64x2.sub"            { RETURN_OPCODE(Binary, F64X2Sub); }
      <i> "f32x4.div"            { RETURN_OPCODE(Binary, F32X4Div); }
      <i> "f64x2.div"            { RETURN_OPCODE(Binary, F64X2Div); }
      <i> "f32x4.mul"            { RETURN_OPCODE(Binary, F32X4Mul); }
      <i> "f64x2.mul"            { RETURN_OPCODE(Binary, F64X2Mul); }
      <i> "f32x4.sqrt"            { RETURN_OPCODE(Unary, F32X4Sqrt); }
      <i> "f64x2.sqrt"            { RETURN_OPCODE(Unary, F64X2Sqrt); }
      <i> "f32x4.convert_s/i32x4" { RETURN_OPCODE(Unary, F32X4ConvertSI32X4); }
      <i> "f32x4.convert_u/i32x4" { RETURN_OPCODE(Unary, F32X4ConvertUI32X4); }
      <i> "f64x2.convert_s/i64x2" { RETURN_OPCODE(Unary, F64X2ConvertSI64X2); }
      <i> "f64x2.convert_u/i64x2" { RETURN_OPCODE(Unary, F64X2ConvertUI64X2); }
      <i> "i32x4.trunc_s/f32x4:sat" { RETURN_OPCODE(Unary, I32X4TruncSF32X4Sat); }
      <i> "i32x4.trunc_u/f32x4:sat" { RETURN_OPCODE(Unary, I32X4TruncUF32X4Sat); }
      <i> "i64x2.trunc_s/f64x2:sat" { RETURN_OPCODE(Unary, I64X2TruncSF64X2Sat); }
      <i> "i64x2.trunc_u/f64x2:sat" { RETURN_OPCODE(Unary, I64X2TruncUF64X2Sat); }

      <i> "type"                { RETURN(Type); }
      <i> "func"                { RETURN(Func); }
      <i> "param"               { RETURN(Param); }
      <i> "result"              { RETURN(Result); }
      <i> "local"               { RETURN(Local); }
      <i> "global"              { RETURN(Global); }
      <i> "module"              { RETURN(Module); }
      <i> "binary"              { RETURN(Bin); }
      <i> "quote"               { RETURN(Quote); }
      <i> "table"               { RETURN(Table); }
      <i> "memory"              { RETURN(Memory); }
      <i> "start"               { RETURN(Start); }
      <i> "elem"                { RETURN(Elem); }
      <i> "data"                { RETURN(Data); }
      <i> "offset"              { RETURN(Offset); }
      <i> "import"              { RETURN(Import); }
      <i> "export"              { RETURN(Export); }
      <i> "except"              { RETURN(Except); }
      <i> "register"            { RETURN(Register); }
      <i> "invoke"              { RETURN(Invoke); }
      <i> "get"                 { RETURN(Get); }
      <i> "assert_malformed"    { RETURN(AssertMalformed); }
      <i> "assert_invalid"      { RETURN(AssertInvalid); }
      <i> "assert_unlinkable"   { RETURN(AssertUnlinkable); }
      <i> "assert_return"       { RETURN(AssertReturn); }
      <i> "assert_return_canonical_nan" { RETURN(AssertReturnCanonicalNan); }
      <i> "assert_return_arithmetic_nan" { RETURN(AssertReturnArithmeticNan); }
      <i> "assert_trap"         { RETURN(AssertTrap); }
      <i> "assert_exhaustion"   { RETURN(AssertExhaustion); }
      <i> "try"                 { RETURN_OPCODE0(Try); }
      <i> "catch"               { RETURN_OPCODE0(Catch); }
      <i> "throw"               { RETURN_OPCODE0(Throw); }
      <i> "rethrow"             { RETURN_OPCODE0(Rethrow); }
      <i> "if_except"           { RETURN_OPCODE0(IfExcept); }
      <i> name                  { RETURN_TEXT(Var); }
      <i> "shared"              { RETURN(Shared); }

      <i> ";;" => LINE_COMMENT  { continue; }
      <LINE_COMMENT> "\n" => i  { NEWLINE; continue; }
      <LINE_COMMENT> [^\n]+     { continue; }
      <i> "(;" => BLOCK_COMMENT { COMMENT_NESTING = 1; continue; }
      <BLOCK_COMMENT> "(;"      { COMMENT_NESTING++; continue; }
      <BLOCK_COMMENT> ";)"      { if (--COMMENT_NESTING == 0) {
                                    BEGIN(YYCOND_i);
                                  }
                                  continue; }
      <BLOCK_COMMENT> "\n"      { NEWLINE; continue; }
      <BLOCK_COMMENT> [^]       { continue; }
      <BLOCK_COMMENT> *         { MAYBE_MALFORMED_UTF8(" in block comment"); }
      <i> "\n"                  { NEWLINE; continue; }
      <i> [ \t\r]+              { continue; }
      <i> reserved              { RETURN_TEXT(Reserved); }
      <i> [^]                   { ERROR("unexpected char"); continue; }
      <*> *                     { MAYBE_MALFORMED_UTF8(""); }
     */
  }
}

}  // namespace wabt

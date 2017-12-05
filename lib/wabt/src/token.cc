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

#include "src/token.h"

namespace wabt {

const char* GetTokenTypeName(TokenType token_type) {
  static const char* s_names[] = {
      // Bare.
      "Invalid",
      "anyfunc",
      "assert_exhaustion",
      "assert_invalid",
      "assert_malformed",
      "assert_return",
      "assert_return_arithmetic_nan",
      "assert_return_canonical_nan",
      "assert_trap",
      "assert_unlinkable",
      "bin",
      "data",
      "elem",
      "EOF",
      "except",
      "export",
      "func",
      "get",
      "global",
      "import",
      "invoke",
      "local",
      "(",
      "memory",
      "module",
      "mut",
      "offset",
      "param",
      "quote",
      "register",
      "result",
      ")",
      "shared",
      "start",
      "table",
      "then",
      "type",

      // Literal.
      "FLOAT",
      "NAT",
      "INT",

      // Opcode.
      "ATOMIC_LOAD",
      "ATOMIC_RMW",
      "ATOMIC_RMW_CMPXCHG",
      "ATOMIC_STORE",
      "BINARY",
      "block",
      "br",
      "br_if",
      "br_table",
      "call",
      "call_indirect",
      "catch",
      "catch_all",
      "COMPARE",
      "CONST",
      "CONVERT",
      "current_memory",
      "drop",
      "else",
      "end",
      "get_global",
      "get_local",
      "grow_memory",
      "if",
      "LOAD",
      "loop",
      "nop",
      "rethrow",
      "return",
      "select",
      "set_global",
      "set_local",
      "STORE",
      "tee_local",
      "throw",
      "try",
      "UNARY",
      "unreachable",

      // String.
      "align=",
      "offset=",
      "Reserved",
      "TEXT",
      "VAR",

      // Type.
      "VALUETYPE",
  };

  static_assert(
      WABT_ARRAY_SIZE(s_names) == WABT_ENUM_COUNT(TokenType),
      "Expected TokenType names list length to match number of TokenTypes.");

  int x = static_cast<int>(token_type);
  if (x < WABT_ENUM_COUNT(TokenType))
    return s_names[x];

  return "Invalid";
}

Token::Token(Location loc, TokenType token_type)
    : loc(loc), token_type_(token_type) {
  assert(IsTokenTypeBare(token_type_));
}

Token::Token(Location loc, TokenType token_type, Type type)
    : loc(loc), token_type_(token_type) {
  assert(HasType());
  Construct(type_, type);
}

Token::Token(Location loc, TokenType token_type, const std::string& text)
    : loc(loc), token_type_(token_type) {
  assert(HasText());
  Construct(text_, text);
}

Token::Token(Location loc, TokenType token_type, Opcode opcode)
    : loc(loc), token_type_(token_type) {
  assert(HasOpcode());
  Construct(opcode_, opcode);
}

Token::Token(Location loc, TokenType token_type, const Literal& literal)
    : loc(loc), token_type_(token_type) {
  assert(HasLiteral());
  Construct(literal_, literal);
}

Token::Token(const Token& other) : Token() {
  *this = other;
}

Token::Token(Token&& other) : Token() {
  *this = std::move(other);
}

Token& Token::operator=(const Token& other) {
  Destroy();
  loc = other.loc;
  token_type_ = other.token_type_;

  if (HasLiteral()) {
    Construct(literal_, other.literal_);
  } else if (HasOpcode()) {
    Construct(opcode_, other.opcode_);
  } else if (HasText()) {
    Construct(text_, other.text_);
  } else if (HasType()) {
    Construct(type_, other.type_);
  }

  return *this;
}

Token& Token::operator=(Token&& other) {
  Destroy();
  loc = other.loc;
  token_type_ = other.token_type_;

  if (HasLiteral()) {
    Construct(literal_, std::move(other.literal_));
  } else if (HasOpcode()) {
    Construct(opcode_, std::move(other.opcode_));
  } else if (HasText()) {
    Construct(text_, std::move(other.text_));
  } else if (HasType()) {
    Construct(type_, std::move(other.type_));
  }

  other.token_type_ = TokenType::Invalid;

  return *this;
}

Token::~Token() {
  Destroy();
}

void Token::Destroy() {
  if (HasLiteral()) {
    Destruct(literal_);
  } else if (HasOpcode()) {
    Destruct(opcode_);
  } else if (HasText()) {
    Destruct(text_);
  } else if (HasType()) {
    Destruct(type_);
  }
  token_type_ = TokenType::Invalid;
}

std::string Token::to_string() const {
  if (IsTokenTypeBare(token_type_)) {
    return GetTokenTypeName(token_type_);
  } else if (HasLiteral()) {
    return literal_.text;
  } else if (HasOpcode()) {
    return opcode_.GetName();
  } else if (HasText()) {
    return text_;
  } else {
    assert(HasType());
    return GetTypeName(type_);
  }
}

std::string Token::to_string_clamp(size_t max_length) const {
  std::string s = to_string();
  if (s.length() > max_length) {
    return s.substr(0, max_length - 3) + "...";
  } else {
    return s;
  }
}

}  // namespace wabt

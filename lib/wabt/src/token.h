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

#ifndef WABT_TOKEN_H_
#define WABT_TOKEN_H_

#include <string>

#include "src/literal.h"
#include "src/opcode.h"

namespace wabt {

struct Literal {
  Literal() = default;
  Literal(LiteralType type, const std::string& text) : type(type), text(text) {}

  LiteralType type;
  std::string text;
};

enum class TokenType {
#define WABT_TOKEN(name, string) name,
#define WABT_TOKEN_FIRST(group, first) First_##group = first,
#define WABT_TOKEN_LAST(group, last) Last_##group = last,
#include "token.def"
#undef WABT_TOKEN
#undef WABT_TOKEN_FIRST
#undef WABT_TOKEN_LAST

  First = First_Bare,
  Last = Last_Type,
};

const char* GetTokenTypeName(TokenType);

inline bool IsTokenTypeBare(TokenType token_type) {
  return token_type >= TokenType::First_Bare &&
         token_type <= TokenType::Last_Bare;
}

inline bool IsTokenTypeString(TokenType token_type) {
  return token_type >= TokenType::First_String &&
         token_type <= TokenType::Last_String;
}

inline bool IsTokenTypeType(TokenType token_type) {
  return token_type == TokenType::ValueType;
}

inline bool IsTokenTypeOpcode(TokenType token_type) {
  return token_type >= TokenType::First_Opcode &&
         token_type <= TokenType::Last_Opcode;
}

inline bool IsTokenTypeLiteral(TokenType token_type) {
  return token_type >= TokenType::First_Literal &&
         token_type <= TokenType::Last_Literal;
}

struct Token {
  Token() : token_type_(TokenType::Invalid) {}
  Token(Location, TokenType);
  Token(Location, TokenType, Type);
  Token(Location, TokenType, const std::string&);
  Token(Location, TokenType, Opcode);
  Token(Location, TokenType, const Literal&);
  Token(const Token&);
  Token(Token&&);
  Token& operator=(const Token&);
  Token& operator=(Token&&);
  ~Token();

  Location loc;

  TokenType token_type() const { return token_type_; }

  bool HasText() const { return IsTokenTypeString(token_type_); }
  bool HasType() const { return IsTokenTypeType(token_type_); }
  bool HasOpcode() const { return IsTokenTypeOpcode(token_type_); }
  bool HasLiteral() const { return IsTokenTypeLiteral(token_type_); }

  const std::string& text() const {
    assert(HasText());
    return text_;
  }

  Type type() const {
    assert(HasType());
    return type_;
  }

  Opcode opcode() const {
    assert(HasOpcode());
    return opcode_;
  }

  const Literal& literal() const {
    assert(HasLiteral());
    return literal_;
  }

  std::string to_string() const;
  std::string to_string_clamp(size_t max_length) const;

 private:
  void Destroy();

  TokenType token_type_;

  union {
    std::string text_;
    Type type_;
    Opcode opcode_;
    Literal literal_;
  };
};

}  // namespace wabt

#endif  // WABT_TOKEN_H_

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

#ifndef WABT_WAST_PARSER_H_
#define WABT_WAST_PARSER_H_

#include <array>

#include "src/circular-array.h"
#include "src/feature.h"
#include "src/ir.h"
#include "src/intrusive-list.h"
#include "src/wast-lexer.h"

namespace wabt {

class ErrorHandler;

struct WastParseOptions {
  WastParseOptions(const Features& features) : features(features) {}

  Features features;
  bool debug_parsing = false;
};

typedef std::array<TokenType, 2> TokenTypePair;

class WastParser {
 public:
  WastParser(WastLexer*, ErrorHandler*, WastParseOptions*);

  void WABT_PRINTF_FORMAT(3, 4) Error(Location, const char* format, ...);
  Result ParseModule(std::unique_ptr<Module>* out_module);
  Result ParseScript(std::unique_ptr<Script>* out_script);

  std::unique_ptr<Script> ReleaseScript();

 private:
  void ErrorUnlessOpcodeEnabled(const Token&);

  // Print an error message listing the expected tokens, as well as an example
  // of expected input.
  Result ErrorExpected(const std::vector<std::string>& expected,
                       const char* example = nullptr);

  // Print an error message, and and return Result::Error if the next token is
  // '('. This is commonly used after parsing a sequence of s-expressions -- if
  // no more can be parsed, we know that a following '(' is invalid. This
  // function consumes the '(' so a better error message can be provided
  // (assuming the following token was unexpected).
  Result ErrorIfLpar(const std::vector<std::string>& expected,
                     const char* example = nullptr);

  // Returns the next token without consuming it.
  Token GetToken();

  // Returns the location of the next token.
  Location GetLocation();

  // Returns the type of the next token.
  TokenType Peek(size_t n = 0);

  // Returns the types of the next two tokens.
  TokenTypePair PeekPair();

  // Returns true if the next token's type is equal to the parameter.
  bool PeekMatch(TokenType);

  // Returns true if the next token's type is '(' and the following token is
  // equal to the parameter.
  bool PeekMatchLpar(TokenType);

  // Returns true if the next two tokens can start an Expr. This allows for
  // folded expressions, plain instructions and block instructions.
  bool PeekMatchExpr();

  // Returns true if the next token's type is equal to the parameter. If so,
  // then the token is consumed.
  bool Match(TokenType);

  // Returns true if the next token's type is equal to '(' and the following
  // token is equal to the parameter. If so, then the token is consumed.
  bool MatchLpar(TokenType);

  // Like Match(), but prints an error message if the token doesn't match, and
  // returns Result::Error.
  Result Expect(TokenType);

  // Consume one token and return it.
  Token Consume();

  // Give the Match() function a clearer name when used to optionally consume a
  // token (used for printing better error messages).
  void ConsumeIfLpar() { Match(TokenType::Lpar); }

  typedef bool SynchronizeFunc(TokenTypePair pair);

  // Attempt to synchronize the token stream by dropping tokens until the
  // SynchronizeFunc returns true, or until a token limit is reached. This
  // function returns Result::Error if the stream was not able to be
  // synchronized.
  Result Synchronize(SynchronizeFunc);

  void ParseBindVarOpt(std::string* name);
  Result ParseVar(Var* out_var);
  bool ParseVarOpt(Var* out_var, Var default_var = Var());
  Result ParseOffsetExpr(ExprList* out_expr_list);
  Result ParseTextList(std::vector<uint8_t>* out_data);
  bool ParseTextListOpt(std::vector<uint8_t>* out_data);
  Result ParseVarList(VarVector* out_var_list);
  bool ParseVarListOpt(VarVector* out_var_list);
  Result ParseValueType(Type* out_type);
  Result ParseValueTypeList(TypeVector* out_type_list);
  Result ParseQuotedText(std::string* text);
  bool ParseOffsetOpt(uint32_t* offset);
  bool ParseAlignOpt(uint32_t* align);
  Result ParseLimits(Limits*);
  Result ParseNat(uint64_t*);

  Result ParseModuleFieldList(Module*);
  Result ParseModuleField(Module*);
  Result ParseDataModuleField(Module*);
  Result ParseElemModuleField(Module*);
  Result ParseExceptModuleField(Module*);
  Result ParseExportModuleField(Module*);
  Result ParseFuncModuleField(Module*);
  Result ParseTypeModuleField(Module*);
  Result ParseGlobalModuleField(Module*);
  Result ParseImportModuleField(Module*);
  Result ParseMemoryModuleField(Module*);
  Result ParseStartModuleField(Module*);
  Result ParseTableModuleField(Module*);

  Result ParseExportDesc(Export*);
  Result ParseInlineExports(ModuleFieldList*, ExternalKind);
  Result ParseInlineImport(Import*);
  Result ParseTypeUseOpt(FuncDeclaration*);
  Result ParseFuncSignature(FuncSignature*, BindingHash* param_bindings);
  Result ParseBoundValueTypeList(TokenType, TypeVector*, BindingHash*);
  Result ParseResultList(TypeVector*);
  Result ParseInstrList(ExprList*);
  Result ParseTerminatingInstrList(ExprList*);
  Result ParseInstr(ExprList*);
  Result ParsePlainInstr(std::unique_ptr<Expr>*);
  Result ParseConst(Const*);
  Result ParseConstList(ConstVector*);
  Result ParseBlockInstr(std::unique_ptr<Expr>*);
  Result ParseLabelOpt(std::string*);
  Result ParseEndLabelOpt(const std::string&);
  Result ParseBlock(Block*);
  Result ParseExprList(ExprList*);
  Result ParseExpr(ExprList*);
  Result ParseCatchInstrList(CatchVector* catches);
  Result ParseCatchExprList(CatchVector* catches);
  Result ParseGlobalType(Global*);

  template <typename T>
  Result ParsePlainInstrVar(Location, std::unique_ptr<Expr>*);
  template <typename T>
  Result ParsePlainLoadStoreInstr(Location, Token, std::unique_ptr<Expr>*);

  Result ParseCommandList(Script*, CommandPtrVector*);
  Result ParseCommand(Script*, CommandPtr*);
  Result ParseAssertExhaustionCommand(CommandPtr*);
  Result ParseAssertInvalidCommand(CommandPtr*);
  Result ParseAssertMalformedCommand(CommandPtr*);
  Result ParseAssertReturnCommand(CommandPtr*);
  Result ParseAssertReturnArithmeticNanCommand(CommandPtr*);
  Result ParseAssertReturnCanonicalNanCommand(CommandPtr*);
  Result ParseAssertTrapCommand(CommandPtr*);
  Result ParseAssertUnlinkableCommand(CommandPtr*);
  Result ParseActionCommand(CommandPtr*);
  Result ParseModuleCommand(Script*, CommandPtr*);
  Result ParseRegisterCommand(CommandPtr*);

  Result ParseAction(ActionPtr*);
  Result ParseScriptModule(std::unique_ptr<ScriptModule>*);

  template <typename T>
  Result ParseActionCommand(TokenType, CommandPtr*);
  template <typename T>
  Result ParseAssertActionCommand(TokenType, CommandPtr*);
  template <typename T>
  Result ParseAssertActionTextCommand(TokenType, CommandPtr*);
  template <typename T>
  Result ParseAssertScriptModuleCommand(TokenType, CommandPtr*);

  void CheckImportOrdering(Module*);

  WastLexer* lexer_;
  Index last_module_index_ = kInvalidIndex;
  ErrorHandler* error_handler_;
  int errors_ = 0;
  WastParseOptions* options_;

  CircularArray<Token, 2> tokens_;
};

Result ParseWatModule(WastLexer* lexer,
                      std::unique_ptr<Module>* out_module,
                      ErrorHandler*,
                      WastParseOptions* options = nullptr);

Result ParseWastScript(WastLexer* lexer,
                       std::unique_ptr<Script>* out_script,
                       ErrorHandler*,
                       WastParseOptions* options = nullptr);

}  // namespace wabt

#endif /* WABT_WAST_PARSER_H_ */

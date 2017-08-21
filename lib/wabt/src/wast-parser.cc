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

#include "wast-parser.h"

#include "binary-reader.h"
#include "binary-reader-ir.h"
#include "cast.h"
#include "error-handler.h"
#include "wast-parser-lexer-shared.h"

#define WABT_TRACING 0
#include "tracing.h"

#define CHECK_RESULT(expr)  \
  do {                      \
    if (Failed(expr))       \
      return Result::Error; \
  } while (0)

#define EXPECT(token_type) CHECK_RESULT(Expect(TokenType::token_type))

namespace wabt {

namespace {

bool IsPowerOfTwo(uint32_t x) {
  return x && ((x & (x - 1)) == 0);
}

template <typename OutputIter>
void RemoveEscapes(string_view text, OutputIter dest) {
  // Remove surrounding quotes; if any. This may be empty if the string was
  // invalid (e.g. if it contained a bad escape sequence).
  if (text.size() <= 2)
    return;

  text = text.substr(1, text.size() - 2);

  const char* src = text.data();
  const char* end = text.data() + text.size();

  while (src < end) {
    if (*src == '\\') {
      src++;
      switch (*src) {
        case 'n':
          *dest++ = '\n';
          break;
        case 'r':
          *dest++ = '\r';
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
          // The string should be validated already, so we know this is a hex
          // sequence.
          uint32_t hi;
          uint32_t lo;
          if (Succeeded(ParseHexdigit(src[0], &hi)) &&
              Succeeded(ParseHexdigit(src[1], &lo))) {
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
}

typedef std::vector<std::string> TextVector;

template <typename OutputIter>
void RemoveEscapes(const TextVector& texts, OutputIter out) {
  for (const std::string& text: texts)
    RemoveEscapes(text, out);
}

class BinaryErrorHandlerModule : public ErrorHandler {
 public:
  BinaryErrorHandlerModule(Location* loc, WastParser* parser);
  bool OnError(const Location&,
               const std::string& error,
               const std::string& source_line,
               size_t source_line_column_offset) override;

  // Unused.
  size_t source_line_max_length() const override { return 0; }

 private:
  Location* loc_;
  WastParser* parser_;
};

BinaryErrorHandlerModule::BinaryErrorHandlerModule(Location* loc,
                                                   WastParser* parser)
    : ErrorHandler(Location::Type::Binary), loc_(loc), parser_(parser) {}

bool BinaryErrorHandlerModule::OnError(const Location& binary_loc,
                                       const std::string& error,
                                       const std::string& source_line,
                                       size_t source_line_column_offset) {
  if (binary_loc.offset == kInvalidOffset) {
    parser_->Error(*loc_, "error in binary module: %s", error.c_str());
  } else {
    parser_->Error(*loc_, "error in binary module: @0x%08" PRIzx ": %s",
                   binary_loc.offset, error.c_str());
  }
  return true;
}

bool IsPlainInstr(TokenType token_type) {
  switch (token_type) {
    case TokenType::Unreachable:
    case TokenType::Nop:
    case TokenType::Drop:
    case TokenType::Select:
    case TokenType::Br:
    case TokenType::BrIf:
    case TokenType::BrTable:
    case TokenType::Return:
    case TokenType::Call:
    case TokenType::CallIndirect:
    case TokenType::GetLocal:
    case TokenType::SetLocal:
    case TokenType::TeeLocal:
    case TokenType::GetGlobal:
    case TokenType::SetGlobal:
    case TokenType::Load:
    case TokenType::Store:
    case TokenType::Const:
    case TokenType::Unary:
    case TokenType::Binary:
    case TokenType::Compare:
    case TokenType::Convert:
    case TokenType::CurrentMemory:
    case TokenType::GrowMemory:
    case TokenType::Throw:
    case TokenType::Rethrow:
      return true;
    default:
      return false;
  }
}

bool IsBlockInstr(TokenType token_type) {
  switch (token_type) {
    case TokenType::Block:
    case TokenType::Loop:
    case TokenType::If:
    case TokenType::Try:
      return true;
    default:
      return false;
  }
}

bool IsPlainOrBlockInstr(TokenType token_type) {
  return IsPlainInstr(token_type) || IsBlockInstr(token_type);
}

bool IsExpr(TokenTypePair pair) {
  return pair[0] == TokenType::Lpar && IsPlainOrBlockInstr(pair[1]);
}

bool IsInstr(TokenTypePair pair) {
  return IsPlainOrBlockInstr(pair[0]) || IsExpr(pair);
}

bool IsCatch(TokenType token_type) {
  return token_type == TokenType::Catch || token_type == TokenType::CatchAll;
}

bool IsModuleField(TokenTypePair pair) {
  if (pair[0] != TokenType::Lpar)
    return false;

  switch (pair[1]) {
    case TokenType::Data:
    case TokenType::Elem:
    case TokenType::Except:
    case TokenType::Export:
    case TokenType::Func:
    case TokenType::Type:
    case TokenType::Global:
    case TokenType::Import:
    case TokenType::Memory:
    case TokenType::Start:
    case TokenType::Table:
      return true;
    default:
      return false;
  }
}

bool IsCommand(TokenTypePair pair) {
  if (pair[0] != TokenType::Lpar)
    return false;

  switch (pair[1]) {
    case TokenType::AssertExhaustion:
    case TokenType::AssertInvalid:
    case TokenType::AssertMalformed:
    case TokenType::AssertReturn:
    case TokenType::AssertReturnArithmeticNan:
    case TokenType::AssertReturnCanonicalNan:
    case TokenType::AssertTrap:
    case TokenType::AssertUnlinkable:
    case TokenType::Get:
    case TokenType::Invoke:
    case TokenType::Module:
    case TokenType::Register:
      return true;
    default:
      return false;
  }
}

bool IsEmptySignature(const FuncSignature* sig) {
  return sig->result_types.empty() && sig->param_types.empty();
}

void ResolveFuncTypes(Module* module) {
  for (ModuleField& field : module->fields) {
    Func* func = nullptr;
    if (field.type == ModuleFieldType::Func) {
      func = dyn_cast<FuncModuleField>(&field)->func;
    } else if (field.type == ModuleFieldType::Import) {
      Import* import = dyn_cast<ImportModuleField>(&field)->import;
      if (import->kind == ExternalKind::Func) {
        func = import->func;
      } else {
        continue;
      }
    } else {
      continue;
    }

    // Resolve func type variables where the signature was not specified
    // explicitly, e.g.: (func (type 1) ...)
    if (func->decl.has_func_type && IsEmptySignature(&func->decl.sig)) {
      FuncType* func_type = module->GetFuncType(func->decl.type_var);
      if (func_type) {
        func->decl.sig = func_type->sig;
      }
    }

    // Resolve implicitly defined function types, e.g.: (func (param i32) ...)
    if (!func->decl.has_func_type) {
      Index func_type_index = module->GetFuncTypeIndex(func->decl.sig);
      if (func_type_index == kInvalidIndex) {
        auto func_type = new FuncType();
        func_type->sig = func->decl.sig;
        module->AppendField(new FuncTypeModuleField(func_type, field.loc));
      }
    }
  }
}

void AppendInlineExportFields(Module* module,
                              ModuleFieldList* fields,
                              Index index) {
  Location last_field_loc = module->fields.back().loc;

  for (const ModuleField& field: *fields) {
    auto* export_field = cast<ExportModuleField>(&field);
    export_field->export_->var = Var(index, last_field_loc);
  }

  module->AppendFields(fields);
}

}  // End of anonymous namespace

WastParser::WastParser(WastLexer* lexer,
                       ErrorHandler* error_handler,
                       WastParseOptions* options)
    : lexer_(lexer), error_handler_(error_handler), options_(options) {}

void WastParser::Error(Location loc, const char* format, ...) {
  errors_++;
  va_list args;
  va_start(args, format);
  WastFormatError(error_handler_, &loc, lexer_, format, args);
  va_end(args);
}

Token WastParser::GetToken() {
  if (tokens_.empty())
    tokens_.push_back(lexer_->GetToken(this));
  return tokens_.front();
}

Location WastParser::GetLocation() {
  return GetToken().loc;
}

TokenType WastParser::Peek(size_t n) {
  while (tokens_.size() <= n)
    tokens_.push_back(lexer_->GetToken(this));
  return tokens_.at(n).token_type;
}

TokenTypePair WastParser::PeekPair() {
  return TokenTypePair{{Peek(), Peek(1)}};
}

bool WastParser::PeekMatch(TokenType type) {
  return Peek() == type;
}

bool WastParser::PeekMatchLpar(TokenType type) {
  return Peek() == TokenType::Lpar && Peek(1) == type;
}

bool WastParser::PeekMatchExpr() {
  return IsExpr(PeekPair());
}

bool WastParser::Match(TokenType type) {
  if (PeekMatch(type)) {
    Consume();
    return true;
  }
  return false;
}

bool WastParser::MatchLpar(TokenType type) {
  if (PeekMatchLpar(type)) {
    Consume();
    Consume();
    return true;
  }
  return false;
}

Result WastParser::Expect(TokenType type) {
  if (!Match(type)) {
    auto token = Consume();
    Error(token.loc, "unexpected token %s, expected %s.",
          token.to_string().c_str(), GetTokenTypeName(type));
    return Result::Error;
  }

  return Result::Ok;
}

Token WastParser::Consume() {
  assert(!tokens_.empty());
  auto token = tokens_.front();
  tokens_.pop_front();
  return token;
}

Result WastParser::Synchronize(SynchronizeFunc func) {
  static const int kMaxConsumed = 10;
  for (int i = 0; i < kMaxConsumed; ++i) {
    if (func(PeekPair()))
      return Result::Ok;

    auto token = Consume();
    if (token.token_type == TokenType::Reserved) {
      Error(token.loc, "unexpected token %s.", token.to_string().c_str());
    }
  }

  return Result::Error;
}

void WastParser::ErrorUnlessOpcodeEnabled(const Token& token) {
  Opcode opcode = token.opcode;
  if (!opcode.IsEnabled(options_->features))
    Error(token.loc, "opcode not allowed: %s", opcode.GetName());
}

Result WastParser::ErrorExpected(const std::vector<std::string>& expected,
                                 const char* example) {
  auto token = Consume();
  std::string expected_str;
  if (!expected.empty()) {
    expected_str = ", expected ";
    for (size_t i = 0; i < expected.size(); ++i) {
      if (i != 0) {
        if (i == expected.size() - 1)
          expected_str += " or ";
        else
          expected_str += ", ";
      }

      expected_str += expected[i];
    }

    if (example) {
      expected_str += " (e.g. ";
      expected_str += example;
      expected_str += ")";
    }
  }

  Error(token.loc, "unexpected token \"%s\"%s.", token.to_string().c_str(),
        expected_str.c_str());
  return Result::Error;
}

Result WastParser::ErrorIfLpar(const std::vector<std::string>& expected,
                               const char* example) {
  if (Match(TokenType::Lpar))
    return ErrorExpected(expected, example);
  return Result::Ok;
}

void WastParser::ParseBindVarOpt(std::string* name) {
  WABT_TRACE(ParseBindVarOpt);
  if (PeekMatch(TokenType::Var)) {
    auto token = Consume();
    *name = token.text.to_string();
  }
}

Result WastParser::ParseVar(Var* out_var) {
  WABT_TRACE(ParseVar);
  if (PeekMatch(TokenType::Nat)) {
    auto token = Consume();
    string_view sv = token.literal.text.to_string_view();
    uint64_t index = kInvalidIndex;
    if (Failed(ParseUint64(sv.begin(), sv.end(), &index))) {
      // Print an error, but don't fail parsing.
      Error(token.loc, "invalid int \"" PRIstringview "\"",
            WABT_PRINTF_STRING_VIEW_ARG(sv));
    }

    *out_var = Var(index, token.loc);
    return Result::Ok;
  } else if (PeekMatch(TokenType::Var)) {
    auto token = Consume();
    *out_var = Var(token.text.to_string_view(), token.loc);
    return Result::Ok;
  } else {
    return ErrorExpected({"a numeric index", "a name"}, "12 or $foo");
  }
}

bool WastParser::ParseVarOpt(Var* out_var, Var default_var) {
  WABT_TRACE(ParseVarOpt);
  if (PeekMatch(TokenType::Nat) || PeekMatch(TokenType::Var)) {
    Result result = ParseVar(out_var);
    // Should always succeed, the only way it could fail is if the token
    // doesn't match.
    assert(Succeeded(result));
    WABT_USE(result);
    return true;
  } else {
    *out_var = default_var;
    return false;
  }
}

Result WastParser::ParseOffsetExpr(ExprList* out_expr_list) {
  WABT_TRACE(ParseOffsetExpr);
  if (MatchLpar(TokenType::Offset)) {
    CHECK_RESULT(ParseTerminatingInstrList(out_expr_list));
    EXPECT(Rpar);
  } else if (PeekMatchExpr()) {
    CHECK_RESULT(ParseExpr(out_expr_list));
  } else {
    return ErrorExpected({"an offset expr"}, "(i32.const 123)");
  }
  return Result::Ok;
}

Result WastParser::ParseTextList(std::vector<uint8_t>* out_data) {
  WABT_TRACE(ParseTextList);
  if (!ParseTextListOpt(out_data)) {
    return Result::Error;
  }

  return Result::Ok;
}

bool WastParser::ParseTextListOpt(std::vector<uint8_t>* out_data) {
  WABT_TRACE(ParseTextListOpt);
  TextVector texts;
  while (PeekMatch(TokenType::Text))
    texts.push_back(Consume().text.to_string());

  RemoveEscapes(texts, std::back_inserter(*out_data));
  return !texts.empty();
}

Result WastParser::ParseVarList(VarVector* out_var_list) {
  WABT_TRACE(ParseVarList);
  if (!ParseVarListOpt(out_var_list)) {
    return Result::Error;
  }

  return Result::Ok;
}

bool WastParser::ParseVarListOpt(VarVector* out_var_list) {
  WABT_TRACE(ParseVarListOpt);
  Var var;
  while (ParseVarOpt(&var))
    out_var_list->push_back(var);

  return !out_var_list->empty();
}

Result WastParser::ParseValueType(Type* out_type) {
  WABT_TRACE(ParseValueType);
  if (!PeekMatch(TokenType::ValueType))
    return ErrorExpected({"i32", "i64", "f32", "f64"});

  *out_type = Consume().type;
  return Result::Ok;
}

Result WastParser::ParseValueTypeList(TypeVector* out_type_list) {
  WABT_TRACE(ParseValueTypeList);
  while (PeekMatch(TokenType::ValueType))
    out_type_list->push_back(Consume().type);

  CHECK_RESULT(ErrorIfLpar({"i32", "i64", "f32", "f64"}));
  return Result::Ok;
}

Result WastParser::ParseQuotedText(std::string* text) {
  WABT_TRACE(ParseQuotedText);
  if (!PeekMatch(TokenType::Text))
    return ErrorExpected({"a quoted string"}, "\"foo\"");

  RemoveEscapes(Consume().text.to_string_view(), std::back_inserter(*text));
  return Result::Ok;
}

bool WastParser::ParseOffsetOpt(uint32_t* out_offset) {
  WABT_TRACE(ParseOffsetOpt);
  if (PeekMatch(TokenType::OffsetEqNat)) {
    auto token = Consume();
    uint64_t offset64;
    string_view sv = token.text.to_string_view();
    if (Failed(ParseInt64(sv.begin(), sv.end(), &offset64,
                          ParseIntType::SignedAndUnsigned))) {
      Error(token.loc, "invalid offset \"" PRIstringview "\"",
            WABT_PRINTF_STRING_VIEW_ARG(sv));
    }
    if (offset64 > UINT32_MAX) {
      Error(token.loc, "offset must be less than or equal to 0xffffffff");
    }

    *out_offset = static_cast<uint32_t>(offset64);
    return true;
  } else {
    *out_offset = 0;
    return false;
  }
}

bool WastParser::ParseAlignOpt(uint32_t* out_align) {
  WABT_TRACE(ParseAlignOpt);
  if (PeekMatch(TokenType::AlignEqNat)) {
    auto token = Consume();
    string_view sv = token.text.to_string_view();
    if (Failed(ParseInt32(sv.begin(), sv.end(), out_align,
                          ParseIntType::UnsignedOnly))) {
      Error(token.loc, "invalid alignment \"" PRIstringview "\"",
            WABT_PRINTF_STRING_VIEW_ARG(sv));
    }

    if (!IsPowerOfTwo(*out_align)) {
      Error(token.loc, "alignment must be power-of-two");
    }

    return true;
  } else {
    *out_align = WABT_USE_NATURAL_ALIGNMENT;
    return false;
  }
}

Result WastParser::ParseLimits(Limits* out_limits) {
  WABT_TRACE(ParseLimits);
  CHECK_RESULT(ParseNat(&out_limits->initial));

  if (PeekMatch(TokenType::Nat)) {
    CHECK_RESULT(ParseNat(&out_limits->max));
    out_limits->has_max = true;
  } else {
    out_limits->has_max = false;
  }

  return Result::Ok;
}

Result WastParser::ParseNat(uint64_t* out_nat) {
  WABT_TRACE(ParseNat);
  if (!PeekMatch(TokenType::Nat))
    return ErrorExpected({"a natural number"}, "123");

  auto token = Consume();
  string_view sv = token.literal.text.to_string_view();
  if (Failed(ParseUint64(sv.begin(), sv.end(), out_nat))) {
    Error(token.loc, "invalid int \"" PRIstringview "\"",
          WABT_PRINTF_STRING_VIEW_ARG(sv));
  }

  return Result::Ok;
}

Result WastParser::ParseScript(Script* script) {
  WABT_TRACE(ParseScript);
  script_ = script;

  // Don't consume the Lpar yet, even though it is required. This way the
  // sub-parser functions (e.g. ParseFuncModuleField) can consume it and keep
  // the parsing structure more regular.
  if (IsModuleField(PeekPair())) {
    // Parse an inline module (i.e. one with no surrounding (module)).
    auto module = make_unique<Module>();
    module->loc = GetLocation();
    CHECK_RESULT(ParseModuleFieldList(module.get()));
    script->commands.emplace_back(make_unique<ModuleCommand>(module.release()));
  } else if (IsCommand(PeekPair())) {
    CHECK_RESULT(ParseCommandList(&script->commands));
  } else {
    ConsumeIfLpar();
    ErrorExpected({"a module field", "a command"});
  }

  EXPECT(Eof);
  return errors_ == 0 ? Result::Ok : Result::Error;
}

Result WastParser::ParseModuleFieldList(Module* module) {
  WABT_TRACE(ParseModuleFieldList);
  while (PeekMatch(TokenType::Lpar)) {
    if (Failed(ParseModuleField(module))) {
      CHECK_RESULT(Synchronize(IsModuleField));
    }
  }
  ResolveFuncTypes(module);
  return Result::Ok;
}

Result WastParser::ParseModuleField(Module* module) {
  WABT_TRACE(ParseModuleField);
  switch (Peek(1)) {
    case TokenType::Data:   return ParseDataModuleField(module);
    case TokenType::Elem:   return ParseElemModuleField(module);
    case TokenType::Except: return ParseExceptModuleField(module);
    case TokenType::Export: return ParseExportModuleField(module);
    case TokenType::Func:   return ParseFuncModuleField(module);
    case TokenType::Type:   return ParseTypeModuleField(module);
    case TokenType::Global: return ParseGlobalModuleField(module);
    case TokenType::Import: return ParseImportModuleField(module);
    case TokenType::Memory: return ParseMemoryModuleField(module);
    case TokenType::Start:  return ParseStartModuleField(module);
    case TokenType::Table:  return ParseTableModuleField(module);
    default:
      assert(
          !"ParseModuleField should only be called if IsModuleField() is true");
      return Result::Error;
  }
}

Result WastParser::ParseDataModuleField(Module* module) {
  WABT_TRACE(ParseDataModuleField);
  EXPECT(Lpar);
  auto loc = GetLocation();
  EXPECT(Data);
  auto data_segment = make_unique<DataSegment>();
  ParseVarOpt(&data_segment->memory_var, Var(0, loc));
  CHECK_RESULT(ParseOffsetExpr(&data_segment->offset));
  ParseTextListOpt(&data_segment->data);
  EXPECT(Rpar);
  module->AppendField(new DataSegmentModuleField(data_segment.release(), loc));
  return Result::Ok;
}

Result WastParser::ParseElemModuleField(Module* module) {
  WABT_TRACE(ParseElemModuleField);
  EXPECT(Lpar);
  auto loc = GetLocation();
  EXPECT(Elem);
  auto elem_segment = make_unique<ElemSegment>();
  ParseVarOpt(&elem_segment->table_var, Var(0, loc));
  CHECK_RESULT(ParseOffsetExpr(&elem_segment->offset));
  ParseVarListOpt(&elem_segment->vars);
  EXPECT(Rpar);
  module->AppendField(new ElemSegmentModuleField(elem_segment.release(), loc));
  return Result::Ok;
}

Result WastParser::ParseExceptModuleField(Module* module) {
  WABT_TRACE(ParseExceptModuleField);
  EXPECT(Lpar);
  auto loc = GetLocation();
  EXPECT(Except);
  auto exception = make_unique<Exception>();
  ParseBindVarOpt(&exception->name);
  ParseValueTypeList(&exception->sig);
  EXPECT(Rpar);
  module->AppendField(new ExceptionModuleField(exception.release(), loc));
  return Result::Ok;
}

Result WastParser::ParseExportModuleField(Module* module) {
  WABT_TRACE(ParseExportModuleField);
  EXPECT(Lpar);
  auto loc = GetLocation();
  EXPECT(Export);
  auto export_ = make_unique<Export>();
  CHECK_RESULT(ParseQuotedText(&export_->name));
  CHECK_RESULT(ParseExportDesc(export_.get()));
  EXPECT(Rpar);
  module->AppendField(new ExportModuleField(export_.release(), loc));
  return Result::Ok;
}

Result WastParser::ParseFuncModuleField(Module* module) {
  WABT_TRACE(ParseFuncModuleField);
  EXPECT(Lpar);
  auto loc = GetLocation();
  EXPECT(Func);
  auto func = make_unique<Func>();
  ParseBindVarOpt(&func->name);

  ModuleFieldList export_fields;
  CHECK_RESULT(ParseInlineExports(&export_fields, ExternalKind::Func));

  if (PeekMatchLpar(TokenType::Import)) {
    CheckImportOrdering(module);
    auto import_loc = GetLocation();
    auto import = make_unique<Import>();
    import->kind = ExternalKind::Func;
    import->func = func.release();
    CHECK_RESULT(ParseInlineImport(import.get()));
    CHECK_RESULT(ParseTypeUseOpt(&import->func->decl));
    CHECK_RESULT(ParseFuncSignature(&import->func->decl.sig,
                                    &import->func->param_bindings));
    CHECK_RESULT(ErrorIfLpar({"type", "param", "result"}));
    module->AppendField(new ImportModuleField(import.release(), import_loc));
  } else {
    CHECK_RESULT(ParseTypeUseOpt(&func->decl));
    CHECK_RESULT(ParseFuncSignature(&func->decl.sig, &func->param_bindings));
    CHECK_RESULT(ParseBoundValueTypeList(TokenType::Local, &func->local_types,
                                         &func->local_bindings));
    CHECK_RESULT(ParseTerminatingInstrList(&func->exprs));
    module->AppendField(new FuncModuleField(func.release(), loc));
  }

  AppendInlineExportFields(module, &export_fields, module->funcs.size() - 1);

  EXPECT(Rpar);
  return Result::Ok;
}

Result WastParser::ParseTypeModuleField(Module* module) {
  WABT_TRACE(ParseTypeModuleField);
  EXPECT(Lpar);
  auto loc = GetLocation();
  EXPECT(Type);
  auto func_type = make_unique<FuncType>();
  ParseBindVarOpt(&func_type->name);
  EXPECT(Lpar);
  EXPECT(Func);
  BindingHash bindings;
  CHECK_RESULT(ParseFuncSignature(&func_type->sig, &bindings));
  CHECK_RESULT(ErrorIfLpar({"param", "result"}));
  EXPECT(Rpar);
  EXPECT(Rpar);
  module->AppendField(new FuncTypeModuleField(func_type.release(), loc));
  return Result::Ok;
}

Result WastParser::ParseGlobalModuleField(Module* module) {
  WABT_TRACE(ParseGlobalModuleField);
  EXPECT(Lpar);
  auto loc = GetLocation();
  EXPECT(Global);
  auto global = make_unique<Global>();
  ParseBindVarOpt(&global->name);

  ModuleFieldList export_fields;
  CHECK_RESULT(ParseInlineExports(&export_fields, ExternalKind::Global));

  if (PeekMatchLpar(TokenType::Import)) {
    CheckImportOrdering(module);
    auto import_loc = GetLocation();
    auto import = make_unique<Import>();
    import->kind = ExternalKind::Global;
    import->global = global.release();
    CHECK_RESULT(ParseInlineImport(import.get()));
    CHECK_RESULT(ParseGlobalType(import->global));
    module->AppendField(new ImportModuleField(import.release(), import_loc));
  } else {
    CHECK_RESULT(ParseGlobalType(global.get()));
    CHECK_RESULT(ParseTerminatingInstrList(&global->init_expr));
    module->AppendField(new GlobalModuleField(global.release(), loc));
  }

  AppendInlineExportFields(module, &export_fields, module->globals.size() - 1);

  EXPECT(Rpar);
  return Result::Ok;
}

Result WastParser::ParseImportModuleField(Module* module) {
  WABT_TRACE(ParseImportModuleField);
  EXPECT(Lpar);
  auto loc = GetLocation();
  CheckImportOrdering(module);
  EXPECT(Import);
  auto import = make_unique<Import>();
  CHECK_RESULT(ParseQuotedText(&import->module_name));
  CHECK_RESULT(ParseQuotedText(&import->field_name));
  EXPECT(Lpar);
  switch (Peek()) {
    case TokenType::Func:
      Consume();
      import->kind = ExternalKind::Func;
      import->func = new Func();
      ParseBindVarOpt(&import->func->name);
      if (PeekMatchLpar(TokenType::Type)) {
        import->func->decl.has_func_type = true;
        CHECK_RESULT(ParseTypeUseOpt(&import->func->decl));
        EXPECT(Rpar);
      } else {
        CHECK_RESULT(ParseFuncSignature(&import->func->decl.sig,
                                        &import->func->param_bindings));
        CHECK_RESULT(ErrorIfLpar({"param", "result"}));
        EXPECT(Rpar);
      }
      break;

    case TokenType::Table:
      Consume();
      import->kind = ExternalKind::Table;
      import->table = new Table();
      ParseBindVarOpt(&import->table->name);
      CHECK_RESULT(ParseLimits(&import->table->elem_limits));
      EXPECT(Anyfunc);
      EXPECT(Rpar);
      break;

    case TokenType::Memory:
      Consume();
      import->kind = ExternalKind::Memory;
      import->memory = new Memory();
      ParseBindVarOpt(&import->memory->name);
      CHECK_RESULT(ParseLimits(&import->memory->page_limits));
      EXPECT(Rpar);
      break;

    case TokenType::Global:
      Consume();
      import->kind = ExternalKind::Global;
      import->global = new Global();
      ParseBindVarOpt(&import->global->name);
      CHECK_RESULT(ParseGlobalType(import->global));
      EXPECT(Rpar);
      break;

    case TokenType::Except:
      Consume();
      import->kind = ExternalKind::Except;
      import->except = new Exception();
      ParseBindVarOpt(&import->except->name);
      ParseValueTypeList(&import->except->sig);
      EXPECT(Rpar);
      break;

    default:
      return ErrorExpected({"an external kind"});
  }

  module->AppendField(new ImportModuleField(import.release(), loc));
  EXPECT(Rpar);
  return Result::Ok;
}

Result WastParser::ParseMemoryModuleField(Module* module) {
  WABT_TRACE(ParseMemoryModuleField);
  EXPECT(Lpar);
  auto loc = GetLocation();
  EXPECT(Memory);
  auto memory = make_unique<Memory>();
  ParseBindVarOpt(&memory->name);

  ModuleFieldList export_fields;
  CHECK_RESULT(ParseInlineExports(&export_fields, ExternalKind::Memory));

  if (PeekMatchLpar(TokenType::Import)) {
    CheckImportOrdering(module);
    auto import_loc = GetLocation();
    auto import = make_unique<Import>();
    import->kind = ExternalKind::Memory;
    import->memory = memory.release();
    CHECK_RESULT(ParseInlineImport(import.get()));
    CHECK_RESULT(ParseLimits(&import->memory->page_limits));
    module->AppendField(new ImportModuleField(import.release(), import_loc));
  } else if (MatchLpar(TokenType::Data)) {
    auto data_segment = make_unique<DataSegment>();
    data_segment->memory_var = Var(module->memories.size());
    data_segment->offset.push_back(new ConstExpr(Const(Const::I32(), 0)));
    data_segment->offset.back().loc = loc;
    ParseTextListOpt(&data_segment->data);
    EXPECT(Rpar);

    uint32_t byte_size = WABT_ALIGN_UP_TO_PAGE(data_segment->data.size());
    uint32_t page_size = WABT_BYTES_TO_PAGES(byte_size);
    memory->page_limits.initial = page_size;
    memory->page_limits.max = page_size;
    memory->page_limits.has_max = true;

    module->AppendField(new MemoryModuleField(memory.release(), loc));
    module->AppendField(
        new DataSegmentModuleField(data_segment.release(), loc));
  } else {
    CHECK_RESULT(ParseLimits(&memory->page_limits));
    module->AppendField(new MemoryModuleField(memory.release(), loc));
  }

  AppendInlineExportFields(module, &export_fields, module->memories.size() - 1);

  EXPECT(Rpar);
  return Result::Ok;
}

Result WastParser::ParseStartModuleField(Module* module) {
  WABT_TRACE(ParseStartModuleField);
  EXPECT(Lpar);
  auto loc = GetLocation();
  EXPECT(Start);
  Var var;
  CHECK_RESULT(ParseVar(&var));
  EXPECT(Rpar);
  module->AppendField(new StartModuleField(var, loc));
  return Result::Ok;
}

Result WastParser::ParseTableModuleField(Module* module) {
  WABT_TRACE(ParseTableModuleField);
  EXPECT(Lpar);
  auto loc = GetLocation();
  EXPECT(Table);
  auto table = make_unique<Table>();
  ParseBindVarOpt(&table->name);

  ModuleFieldList export_fields;
  CHECK_RESULT(ParseInlineExports(&export_fields, ExternalKind::Table));

  if (PeekMatchLpar(TokenType::Import)) {
    CheckImportOrdering(module);
    auto import_loc = GetLocation();
    auto import = make_unique<Import>();
    import->kind = ExternalKind::Table;
    import->table = table.release();
    CHECK_RESULT(ParseInlineImport(import.get()));
    CHECK_RESULT(ParseLimits(&import->table->elem_limits));
    EXPECT(Anyfunc);
    module->AppendField(new ImportModuleField(import.release(), import_loc));
  } else if (Match(TokenType::Anyfunc)) {
    EXPECT(Lpar);
    EXPECT(Elem);

    auto elem_segment = make_unique<ElemSegment>();
    elem_segment->table_var = Var(module->tables.size());
    elem_segment->offset.push_back(new ConstExpr(Const(Const::I32(), 0)));
    elem_segment->offset.back().loc = loc;
    CHECK_RESULT(ParseVarList(&elem_segment->vars));
    EXPECT(Rpar);

    table->elem_limits.initial = elem_segment->vars.size();
    table->elem_limits.max = elem_segment->vars.size();
    table->elem_limits.has_max = true;
    module->AppendField(new TableModuleField(table.release(), loc));
    module->AppendField(
        new ElemSegmentModuleField(elem_segment.release(), loc));
  } else {
    CHECK_RESULT(ParseLimits(&table->elem_limits));
    EXPECT(Anyfunc);
    module->AppendField(new TableModuleField(table.release(), loc));
  }

  AppendInlineExportFields(module, &export_fields, module->tables.size() - 1);

  EXPECT(Rpar);
  return Result::Ok;
}

Result WastParser::ParseExportDesc(Export* export_) {
  WABT_TRACE(ParseExportDesc);
  EXPECT(Lpar);
  switch (Peek()) {
    case TokenType::Func:   export_->kind = ExternalKind::Func; break;
    case TokenType::Table:  export_->kind = ExternalKind::Table; break;
    case TokenType::Memory: export_->kind = ExternalKind::Memory; break;
    case TokenType::Global: export_->kind = ExternalKind::Global; break;
    case TokenType::Except: export_->kind = ExternalKind::Except; break;
    default:
      return ErrorExpected({"an external kind"});
  }
  Consume();
  CHECK_RESULT(ParseVar(&export_->var));
  EXPECT(Rpar);
  return Result::Ok;
}

Result WastParser::ParseInlineExports(ModuleFieldList* fields,
                                      ExternalKind kind) {
  WABT_TRACE(ParseInlineExports);
  while (PeekMatchLpar(TokenType::Export)) {
    EXPECT(Lpar);
    auto loc = GetLocation();
    EXPECT(Export);
    auto export_ = make_unique<Export>();
    export_->kind = kind;
    CHECK_RESULT(ParseQuotedText(&export_->name));
    EXPECT(Rpar);
    fields->push_back(new ExportModuleField(export_.release(), loc));
  }
  return Result::Ok;
}

Result WastParser::ParseInlineImport(Import* import) {
  WABT_TRACE(ParseInlineImport);
  EXPECT(Lpar);
  EXPECT(Import);
  CHECK_RESULT(ParseQuotedText(&import->module_name));
  CHECK_RESULT(ParseQuotedText(&import->field_name));
  EXPECT(Rpar);
  return Result::Ok;
}

Result WastParser::ParseTypeUseOpt(FuncDeclaration* decl) {
  WABT_TRACE(ParseTypeUseOpt);
  if (MatchLpar(TokenType::Type)) {
    decl->has_func_type = true;;
    CHECK_RESULT(ParseVar(&decl->type_var));
    EXPECT(Rpar);
  } else {
    decl->has_func_type = false;
  }
  return Result::Ok;
}

Result WastParser::ParseFuncSignature(FuncSignature* sig,
                                      BindingHash* param_bindings) {
  WABT_TRACE(ParseFuncSignature);
  CHECK_RESULT(ParseBoundValueTypeList(TokenType::Param, &sig->param_types,
                                       param_bindings));
  CHECK_RESULT(ParseResultList(&sig->result_types));
  return Result::Ok;
}

Result WastParser::ParseBoundValueTypeList(TokenType token,
                                           TypeVector* types,
                                           BindingHash* bindings) {
  WABT_TRACE(ParseBoundValueTypeList);
  while (MatchLpar(token)) {
    if (PeekMatch(TokenType::Var)) {
      std::string name;
      Type type;
      auto loc = GetLocation();
      ParseBindVarOpt(&name);
      CHECK_RESULT(ParseValueType(&type));
      bindings->emplace(name, Binding(loc, types->size()));
      types->push_back(type);
    } else {
      ParseValueTypeList(types);
    }
    EXPECT(Rpar);
  }
  return Result::Ok;
}

Result WastParser::ParseResultList(TypeVector* result_types) {
  WABT_TRACE(ParseResultList);
  while (MatchLpar(TokenType::Result)) {
    ParseValueTypeList(result_types);
    EXPECT(Rpar);
  }
  return Result::Ok;
}

Result WastParser::ParseInstrList(ExprList* exprs) {
  WABT_TRACE(ParseInstrList);
  ExprList new_exprs;
  while (IsInstr(PeekPair())) {
    if (Succeeded(ParseInstr(&new_exprs))) {
      exprs->splice(exprs->end(), new_exprs);
    } else {
      CHECK_RESULT(Synchronize(IsInstr));
    }
  }
  return Result::Ok;
}

Result WastParser::ParseTerminatingInstrList(ExprList* exprs) {
  WABT_TRACE(ParseTerminatingInstrList);
  Result result = ParseInstrList(exprs);
  // An InstrList often has no further Lpar following it, because it would have
  // gobbled it up. So if there is a following Lpar it is an error. If we
  // handle it here we can produce a nicer error message.
  CHECK_RESULT(ErrorIfLpar({"an instr"}));
  return result;
}

Result WastParser::ParseInstr(ExprList* exprs) {
  WABT_TRACE(ParseInstr);
  if (IsPlainInstr(Peek())) {
    std::unique_ptr<Expr> expr;
    CHECK_RESULT(ParsePlainInstr(&expr));
    exprs->push_back(expr.release());
    return Result::Ok;
  } else if (IsBlockInstr(Peek())) {
    std::unique_ptr<Expr> expr;
    CHECK_RESULT(ParseBlockInstr(&expr));
    exprs->push_back(expr.release());
    return Result::Ok;
  } else if (PeekMatchExpr()) {
    return ParseExpr(exprs);
  } else {
    assert(!"ParseInstr should only be called when IsInstr() is true");
    return Result::Error;
  }
}

template <typename T>
Result WastParser::ParsePlainInstrVar(Location loc,
                                      std::unique_ptr<Expr>* out_expr) {
  Var var;
  CHECK_RESULT(ParseVar(&var));
  out_expr->reset(new T(var, loc));
  return Result::Ok;
}

Result WastParser::ParsePlainInstr(std::unique_ptr<Expr>* out_expr) {
  WABT_TRACE(ParsePlainInstr);
  auto loc = GetLocation();
  switch (Peek()) {
    case TokenType::Unreachable:
      Consume();
      out_expr->reset(new UnreachableExpr(loc));
      break;

    case TokenType::Nop:
      Consume();
      out_expr->reset(new NopExpr(loc));
      break;

    case TokenType::Drop:
      Consume();
      out_expr->reset(new DropExpr(loc));
      break;

    case TokenType::Select:
      Consume();
      out_expr->reset(new SelectExpr(loc));
      break;

    case TokenType::Br:
      Consume();
      CHECK_RESULT(ParsePlainInstrVar<BrExpr>(loc, out_expr));
      break;

    case TokenType::BrIf:
      Consume();
      CHECK_RESULT(ParsePlainInstrVar<BrIfExpr>(loc, out_expr));
      break;

    case TokenType::BrTable: {
      Consume();
      auto var_list = make_unique<VarVector>();
      CHECK_RESULT(ParseVarList(var_list.get()));
      Var var = var_list->back();
      var_list->pop_back();
      out_expr->reset(new BrTableExpr(var_list.release(), var, loc));
      break;
    }

    case TokenType::Return:
      Consume();
      out_expr->reset(new ReturnExpr(loc));
      break;

    case TokenType::Call:
      Consume();
      CHECK_RESULT(ParsePlainInstrVar<CallExpr>(loc, out_expr));
      break;

    case TokenType::CallIndirect:
      Consume();
      CHECK_RESULT(ParsePlainInstrVar<CallIndirectExpr>(loc, out_expr));
      break;

    case TokenType::GetLocal:
      Consume();
      CHECK_RESULT(ParsePlainInstrVar<GetLocalExpr>(loc, out_expr));
      break;

    case TokenType::SetLocal:
      Consume();
      CHECK_RESULT(ParsePlainInstrVar<SetLocalExpr>(loc, out_expr));
      break;

    case TokenType::TeeLocal:
      Consume();
      CHECK_RESULT(ParsePlainInstrVar<TeeLocalExpr>(loc, out_expr));
      break;

    case TokenType::GetGlobal:
      Consume();
      CHECK_RESULT(ParsePlainInstrVar<GetGlobalExpr>(loc, out_expr));
      break;

    case TokenType::SetGlobal:
      Consume();
      CHECK_RESULT(ParsePlainInstrVar<SetGlobalExpr>(loc, out_expr));
      break;

    case TokenType::Load: {
      Opcode opcode = Consume().opcode;
      uint32_t offset;
      uint32_t align;
      ParseOffsetOpt(&offset);
      ParseAlignOpt(&align);
      out_expr->reset(new LoadExpr(opcode, align, offset, loc));
      break;
    }

    case TokenType::Store: {
      Opcode opcode = Consume().opcode;
      uint32_t offset;
      uint32_t align;
      ParseOffsetOpt(&offset);
      ParseAlignOpt(&align);
      out_expr->reset(new StoreExpr(opcode, align, offset, loc));
      break;
    }

    case TokenType::Const: {
      Const const_;
      CHECK_RESULT(ParseConst(&const_));
      out_expr->reset(new ConstExpr(const_, loc));
      break;
    }

    case TokenType::Unary:
      out_expr->reset(new UnaryExpr(Consume().opcode, loc));
      break;

    case TokenType::Binary:
      out_expr->reset(new BinaryExpr(Consume().opcode, loc));
      break;

    case TokenType::Compare:
      out_expr->reset(new CompareExpr(Consume().opcode, loc));
      break;

    case TokenType::Convert: {
      auto token = Consume();
      ErrorUnlessOpcodeEnabled(token);
      out_expr->reset(new ConvertExpr(token.opcode, loc));
      break;
    }

    case TokenType::CurrentMemory:
      Consume();
      out_expr->reset(new CurrentMemoryExpr(loc));
      break;

    case TokenType::GrowMemory:
      Consume();
      out_expr->reset(new GrowMemoryExpr(loc));
      break;

    case TokenType::Throw:
      ErrorUnlessOpcodeEnabled(Consume());
      CHECK_RESULT(ParsePlainInstrVar<ThrowExpr>(loc, out_expr));
      break;

    case TokenType::Rethrow:
      ErrorUnlessOpcodeEnabled(Consume());
      CHECK_RESULT(ParsePlainInstrVar<RethrowExpr>(loc, out_expr));
      break;

    default:
      assert(
          !"ParsePlainInstr should only be called when IsPlainInstr() is true");
      return Result::Error;
  }

  return Result::Ok;
}

Result WastParser::ParseConst(Const* const_) {
  WABT_TRACE(ParseConst);
  auto opcode = Consume().opcode;
  LiteralTerminal literal;

  auto loc = GetLocation();

  switch (Peek()) {
    case TokenType::Nat:
    case TokenType::Int:
    case TokenType::Float:
      literal = Consume().literal;
      break;

    default:
      return ErrorExpected({"a numeric literal"}, "123, -45, 6.7e8");
  }

  string_view sv = literal.text.to_string_view();
  const char* s = sv.begin();
  const char* end = sv.end();

  Result result;
  switch (opcode) {
    case Opcode::I32Const:
      const_->type = Type::I32;
      result =
          ParseInt32(s, end, &const_->u32, ParseIntType::SignedAndUnsigned);
      break;

    case Opcode::I64Const:
      const_->type = Type::I64;
      result =
          ParseInt64(s, end, &const_->u64, ParseIntType::SignedAndUnsigned);
      break;

    case Opcode::F32Const:
      const_->type = Type::F32;
      result = ParseFloat(literal.type, s, end, &const_->f32_bits);
      break;

    case Opcode::F64Const:
      const_->type = Type::F64;
      result = ParseDouble(literal.type, s, end, &const_->f64_bits);
      break;

    default:
      assert(!"ParseConst called with invalid opcode");
      return Result::Error;
  }

  if (Failed(result)) {
    Error(loc, "invalid literal \"%s\"", literal.text.to_string().c_str());
  }

  return Result::Ok;
}

Result WastParser::ParseConstList(ConstVector* consts) {
  WABT_TRACE(ParseConstList);
  while (PeekMatchLpar(TokenType::Const)) {
    Consume();
    Const const_;
    CHECK_RESULT(ParseConst(&const_));
    EXPECT(Rpar);
    consts->push_back(const_);
  }

  return Result::Ok;
}

Result WastParser::ParseBlockInstr(std::unique_ptr<Expr>* out_expr) {
  WABT_TRACE(ParseBlockInstr);
  auto loc = GetLocation();

  switch (Peek()) {
    case TokenType::Block: {
      Consume();
      auto block = make_unique<Block>();
      CHECK_RESULT(ParseLabelOpt(&block->label));
      CHECK_RESULT(ParseBlock(block.get()));
      EXPECT(End);
      CHECK_RESULT(ParseEndLabelOpt(block->label));
      out_expr->reset(new BlockExpr(block.release(), loc));
      break;
    }

    case TokenType::Loop: {
      Consume();
      auto block = make_unique<Block>();
      CHECK_RESULT(ParseLabelOpt(&block->label));
      CHECK_RESULT(ParseBlock(block.get()));
      EXPECT(End);
      CHECK_RESULT(ParseEndLabelOpt(block->label));
      out_expr->reset(new LoopExpr(block.release(), loc));
      break;
    }

    case TokenType::If: {
      Consume();
      auto true_ = make_unique<Block>();
      ExprList false_;
      CHECK_RESULT(ParseLabelOpt(&true_->label));
      CHECK_RESULT(ParseBlock(true_.get()));
      if (Match(TokenType::Else)) {
        CHECK_RESULT(ParseEndLabelOpt(true_->label));
        CHECK_RESULT(ParseTerminatingInstrList(&false_));
      }
      EXPECT(End);
      CHECK_RESULT(ParseEndLabelOpt(true_->label));
      out_expr->reset(new IfExpr(true_.release(), std::move(false_), loc));
      break;
    }

    case TokenType::Try: {
      ErrorUnlessOpcodeEnabled(Consume());
      auto expr = make_unique<TryExpr>(loc);
      expr->block = new Block();
      CatchVector catches;
      CHECK_RESULT(ParseLabelOpt(&expr->block->label));
      CHECK_RESULT(ParseBlock(expr->block));
      CHECK_RESULT(ParseCatchInstrList(&expr->catches));
      CHECK_RESULT(ErrorIfLpar({"a catch expr"}));
      EXPECT(End);
      CHECK_RESULT(ParseEndLabelOpt(expr->block->label));
      *out_expr = std::move(expr);
      break;
    }

    default:
      assert(
          !"ParseBlockInstr should only be called when IsBlockInstr() is true");
      return Result::Error;
  }

  return Result::Ok;
}

Result WastParser::ParseLabelOpt(std::string* out_label) {
  WABT_TRACE(ParseLabelOpt);
  if (PeekMatch(TokenType::Var)) {
    *out_label = Consume().text.to_string();
  } else {
    out_label->clear();
  }
  return Result::Ok;
}

Result WastParser::ParseEndLabelOpt(const std::string& begin_label) {
  WABT_TRACE(ParseEndLabelOpt);
  auto loc = GetLocation();
  std::string end_label;
  CHECK_RESULT(ParseLabelOpt(&end_label));
  if (!end_label.empty()) {
    if (begin_label.empty()) {
      Error(loc, "unexpected label \"%s\"", end_label.c_str());
    } else if (begin_label != end_label) {
      Error(loc, "mismatching label \"%s\" != \"%s\"", begin_label.c_str(),
            end_label.c_str());
    }
  }
  return Result::Ok;
}

Result WastParser::ParseBlock(Block* block) {
  WABT_TRACE(ParseBlock);
  CHECK_RESULT(ParseResultList(&block->sig));
  CHECK_RESULT(ParseInstrList(&block->exprs));
  return Result::Ok;
}

Result WastParser::ParseExprList(ExprList* exprs) {
  WABT_TRACE(ParseExprList);
  ExprList new_exprs;
  while (PeekMatchExpr()) {
    if (Succeeded(ParseExpr(&new_exprs))) {
      exprs->splice(exprs->end(), new_exprs);
    } else {
      CHECK_RESULT(Synchronize(IsExpr));
    }
  }
  return Result::Ok;
}

Result WastParser::ParseExpr(ExprList* exprs) {
  WABT_TRACE(ParseExpr);
  if (!PeekMatch(TokenType::Lpar))
    return Result::Error;

  if (IsPlainInstr(Peek(1))) {
    Consume();
    std::unique_ptr<Expr> expr;
    CHECK_RESULT(ParsePlainInstr(&expr));
    CHECK_RESULT(ParseExprList(exprs));
    CHECK_RESULT(ErrorIfLpar({"an expr"}));
    exprs->push_back(expr.release());
  } else {
    auto loc = GetLocation();

    switch (Peek(1)) {
      case TokenType::Block: {
        Consume();
        Consume();
        auto block = make_unique<Block>();
        CHECK_RESULT(ParseLabelOpt(&block->label));
        CHECK_RESULT(ParseBlock(block.get()));
        exprs->push_back(new BlockExpr(block.release(), loc));
        break;
      }

      case TokenType::Loop: {
        Consume();
        Consume();
        auto block = make_unique<Block>();
        CHECK_RESULT(ParseLabelOpt(&block->label));
        CHECK_RESULT(ParseBlock(block.get()));
        exprs->push_back(new LoopExpr(block.release(), loc));
        break;
      }

      case TokenType::If: {
        Consume();
        Consume();
        auto true_ = make_unique<Block>();
        ExprList false_;

        CHECK_RESULT(ParseLabelOpt(&true_->label));
        CHECK_RESULT(ParseResultList(&true_->sig));

        if (PeekMatchExpr()) {
          ExprList cond;
          CHECK_RESULT(ParseExpr(&cond));
          exprs->splice(exprs->end(), cond);
        }

        if (MatchLpar(TokenType::Then)) {
          CHECK_RESULT(ParseTerminatingInstrList(&true_->exprs));
          EXPECT(Rpar);

          if (MatchLpar(TokenType::Else)) {
            CHECK_RESULT(ParseTerminatingInstrList(&false_));
            EXPECT(Rpar);
          } else if (PeekMatchExpr()) {
            CHECK_RESULT(ParseExpr(&false_));
          }
        } else if (PeekMatchExpr()) {
          CHECK_RESULT(ParseExpr(&true_->exprs));
          if (PeekMatchExpr()) {
            CHECK_RESULT(ParseExpr(&false_));
          }
        } else {
          ConsumeIfLpar();
          return ErrorExpected({"then block"}, "(then ...)");
        }

        exprs->push_back(new IfExpr(true_.release(), std::move(false_), loc));
        break;
      }

      case TokenType::Try: {
        Consume();
        ErrorUnlessOpcodeEnabled(Consume());

        auto block = make_unique<Block>();
        CHECK_RESULT(ParseLabelOpt(&block->label));
        CHECK_RESULT(ParseResultList(&block->sig));
        CHECK_RESULT(ParseInstrList(&block->exprs));
        auto try_ = make_unique<TryExpr>(loc);
        try_->block = block.release();
        CHECK_RESULT(ParseCatchExprList(&try_->catches));
        CHECK_RESULT(ErrorIfLpar({"a catch expr"}));

        exprs->push_back(try_.release());
        break;
      }

      default:
        assert(!"ParseExpr should only be called when IsExpr() is true");
        return Result::Error;
    }
  }

  EXPECT(Rpar);
  return Result::Ok;
}

Result WastParser::ParseCatchInstrList(CatchVector* catches) {
  WABT_TRACE(ParseCatchInstrList);
  while (IsCatch(Peek())) {
    auto catch_ = make_unique<Catch>();
    catch_->loc = GetLocation();

    if (Consume().token_type == TokenType::Catch)
      CHECK_RESULT(ParseVar(&catch_->var));

    CHECK_RESULT(ParseInstrList(&catch_->exprs));
    catches->push_back(catch_.release());
  }

  return Result::Ok;
}

Result WastParser::ParseCatchExprList(CatchVector* catches) {
  WABT_TRACE(ParseCatchExprList);
  while (PeekMatch(TokenType::Lpar) && IsCatch(Peek(1))) {
    Consume();
    auto catch_ = make_unique<Catch>();
    catch_->loc = GetLocation();

    if (Consume().token_type == TokenType::Catch)
      CHECK_RESULT(ParseVar(&catch_->var));

    CHECK_RESULT(ParseTerminatingInstrList(&catch_->exprs));
    EXPECT(Rpar);
    catches->push_back(catch_.release());
  }

  return Result::Ok;
}

Result WastParser::ParseGlobalType(Global* global) {
  WABT_TRACE(ParseGlobalType);
  if (MatchLpar(TokenType::Mut)) {
    global->mutable_ = true;
    CHECK_RESULT(ParseValueType(&global->type));
    CHECK_RESULT(ErrorIfLpar({"i32", "i64", "f32", "f64"}));
    EXPECT(Rpar);
  } else {
    CHECK_RESULT(ParseValueType(&global->type));
  }

  return Result::Ok;
}

Result WastParser::ParseCommandList(CommandPtrVector* commands) {
  WABT_TRACE(ParseCommandList);
  while (PeekMatch(TokenType::Lpar)) {
    CommandPtr command;
    if (Succeeded(ParseCommand(&command))) {
      commands->push_back(std::move(command));
    } else {
      CHECK_RESULT(Synchronize(IsCommand));
    }
  }
  return Result::Ok;
}

Result WastParser::ParseCommand(CommandPtr* out_command) {
  WABT_TRACE(ParseCommand);
  switch (Peek(1)) {
    case TokenType::AssertExhaustion:
      return ParseAssertExhaustionCommand(out_command);

    case TokenType::AssertInvalid:
      return ParseAssertInvalidCommand(out_command);

    case TokenType::AssertMalformed:
      return ParseAssertMalformedCommand(out_command);

    case TokenType::AssertReturn:
      return ParseAssertReturnCommand(out_command);

    case TokenType::AssertReturnArithmeticNan:
      return ParseAssertReturnArithmeticNanCommand(out_command);

    case TokenType::AssertReturnCanonicalNan:
      return ParseAssertReturnCanonicalNanCommand(out_command);

    case TokenType::AssertTrap:
      return ParseAssertTrapCommand(out_command);

    case TokenType::AssertUnlinkable:
      return ParseAssertUnlinkableCommand(out_command);

    case TokenType::Get:
    case TokenType::Invoke:
      return ParseActionCommand(out_command);

    case TokenType::Module:
      return ParseModuleCommand(out_command);

    case TokenType::Register:
      return ParseRegisterCommand(out_command);

    default:
      assert(!"ParseCommand should only be called when IsCommand() is true");
      return Result::Error;
  }
}

Result WastParser::ParseAssertExhaustionCommand(CommandPtr* out_command) {
  WABT_TRACE(ParseAssertExhaustionCommand);
  return ParseAssertActionTextCommand<AssertExhaustionCommand>(
      TokenType::AssertExhaustion, out_command);
}

Result WastParser::ParseAssertInvalidCommand(CommandPtr* out_command) {
  WABT_TRACE(ParseAssertInvalidCommand);
  return ParseAssertScriptModuleCommand<AssertInvalidCommand>(
      TokenType::AssertInvalid, out_command);
}

Result WastParser::ParseAssertMalformedCommand(CommandPtr* out_command) {
  WABT_TRACE(ParseAssertMalformedCommand);
  return ParseAssertScriptModuleCommand<AssertMalformedCommand>(
      TokenType::AssertMalformed, out_command);
}

Result WastParser::ParseAssertReturnCommand(CommandPtr* out_command) {
  WABT_TRACE(ParseAssertReturnCommand);
  EXPECT(Lpar);
  EXPECT(AssertReturn);
  auto action = make_unique<Action>();
  auto consts = make_unique<ConstVector>();
  CHECK_RESULT(ParseAction(action.get()));
  CHECK_RESULT(ParseConstList(consts.get()));
  EXPECT(Rpar);
  out_command->reset(
      new AssertReturnCommand(action.release(), consts.release()));
  return Result::Ok;
}

Result WastParser::ParseAssertReturnArithmeticNanCommand(
    CommandPtr* out_command) {
  WABT_TRACE(ParseAssertReturnArithmeticNanCommand);
  return ParseAssertActionCommand<AssertReturnArithmeticNanCommand>(
      TokenType::AssertReturnArithmeticNan, out_command);
}

Result WastParser::ParseAssertReturnCanonicalNanCommand(
    CommandPtr* out_command) {
  WABT_TRACE(ParseAssertReturnCanonicalNanCommand);
  return ParseAssertActionCommand<AssertReturnCanonicalNanCommand>(
      TokenType::AssertReturnCanonicalNan, out_command);
}

Result WastParser::ParseAssertTrapCommand(CommandPtr* out_command) {
  WABT_TRACE(ParseAssertTrapCommand);
  EXPECT(Lpar);
  EXPECT(AssertTrap);
  if (PeekMatchLpar(TokenType::Module)) {
    std::unique_ptr<ScriptModule> module;
    std::string text;
    CHECK_RESULT(ParseScriptModule(&module));
    CHECK_RESULT(ParseQuotedText(&text));
    out_command->reset(new AssertUninstantiableCommand(module.release(), text));
  } else {
    auto action = make_unique<Action>();
    std::string text;
    CHECK_RESULT(ParseAction(action.get()));
    CHECK_RESULT(ParseQuotedText(&text));
    out_command->reset(new AssertTrapCommand(action.release(), text));
  }
  EXPECT(Rpar);
  return Result::Ok;
}

Result WastParser::ParseAssertUnlinkableCommand(CommandPtr* out_command) {
  WABT_TRACE(ParseAssertUnlinkableCommand);
  return ParseAssertScriptModuleCommand<AssertUnlinkableCommand>(
      TokenType::AssertUnlinkable, out_command);
}

Result WastParser::ParseActionCommand(CommandPtr* out_command) {
  WABT_TRACE(ParseActionCommand);
  auto action = make_unique<Action>();
  CHECK_RESULT(ParseAction(action.get()));
  out_command->reset(new ActionCommand(action.release()));
  return Result::Ok;
}

Result WastParser::ParseModuleCommand(CommandPtr* out_command) {
  WABT_TRACE(ParseModuleCommand);
  std::unique_ptr<ScriptModule> script_module;
  CHECK_RESULT(ParseScriptModule(&script_module));

  std::unique_ptr<Module> module;

  switch (script_module->type) {
    case ScriptModule::Type::Text:
      module.reset(script_module->text);
      script_module->text = nullptr;
      break;

    case ScriptModule::Type::Binary: {
      module.reset(new Module());
      ReadBinaryOptions options;
      BinaryErrorHandlerModule error_handler(&script_module->binary.loc, this);
      const char* filename = "<text>";
      ReadBinaryIr(filename, script_module->binary.data.data(),
                   script_module->binary.data.size(), &options, &error_handler,
                   module.get());
      module->name = script_module->binary.name;
      module->loc = script_module->binary.loc;
      break;
    }

    case ScriptModule::Type::Quoted:
      return ErrorExpected({"a binary module", "a text module"});
  }

  Index command_index = script_->commands.size();

  if (!module->name.empty()) {
    script_->module_bindings.emplace(module->name,
                                     Binding(module->loc, command_index));
  }

  last_module_index_ = command_index;

  out_command->reset(new ModuleCommand(module.release()));
  return Result::Ok;
}

Result WastParser::ParseRegisterCommand(CommandPtr* out_command) {
  WABT_TRACE(ParseRegisterCommand);
  EXPECT(Lpar);
  auto loc = GetLocation();
  EXPECT(Register);
  std::string text;
  Var var;
  CHECK_RESULT(ParseQuotedText(&text));
  ParseVarOpt(&var, Var(last_module_index_, loc));
  EXPECT(Rpar);
  out_command->reset(new RegisterCommand(text, var));
  return Result::Ok;
}

Result WastParser::ParseAction(Action* out_action) {
  WABT_TRACE(ParseAction);
  EXPECT(Lpar);
  auto loc = GetLocation();

  switch (Peek()) {
    case TokenType::Invoke:
      out_action->loc = loc;
      out_action->type = ActionType::Invoke;
      out_action->invoke = new ActionInvoke();

      Consume();
      ParseVarOpt(&out_action->module_var, Var(last_module_index_, loc));
      CHECK_RESULT(ParseQuotedText(&out_action->name));
      CHECK_RESULT(ParseConstList(&out_action->invoke->args));
      break;

    case TokenType::Get:
      out_action->loc = loc;
      out_action->type = ActionType::Get;

      Consume();
      ParseVarOpt(&out_action->module_var, Var(last_module_index_, loc));
      CHECK_RESULT(ParseQuotedText(&out_action->name));
      break;

    default:
      return ErrorExpected({"invoke", "get"});
  }
  EXPECT(Rpar);
  return Result::Ok;
}

Result WastParser::ParseScriptModule(
    std::unique_ptr<ScriptModule>* out_module) {
  WABT_TRACE(ParseScriptModule);
  EXPECT(Lpar);
  auto loc = GetLocation();
  EXPECT(Module);
  std::string name;
  ParseBindVarOpt(&name);

  std::unique_ptr<ScriptModule> script_module;

  switch (Peek()) {
    case TokenType::Bin: {
      Consume();
      std::vector<uint8_t> data;
      CHECK_RESULT(ParseTextList(&data));

      script_module = make_unique<ScriptModule>(ScriptModule::Type::Binary);
      script_module->binary.name = name;
      script_module->binary.loc = loc;
      script_module->binary.data = std::move(data);
      break;
    }

    case TokenType::Quote: {
      Consume();
      std::vector<uint8_t> data;
      CHECK_RESULT(ParseTextList(&data));

      script_module = make_unique<ScriptModule>(ScriptModule::Type::Quoted);
      script_module->quoted.name = name;
      script_module->quoted.loc = loc;
      script_module->quoted.data = std::move(data);
      break;
    }

    default: {
      script_module = make_unique<ScriptModule>(ScriptModule::Type::Text);
      auto module = make_unique<Module>();
      module->loc = loc;
      module->name = name;
      CHECK_RESULT(ParseModuleFieldList(module.get()));
      script_module->text = module.release();
      break;
    }
  }

  *out_module = std::move(script_module);

  EXPECT(Rpar);
  return Result::Ok;
}

template <typename T>
Result WastParser::ParseAssertActionCommand(TokenType token_type,
                                            CommandPtr* out_command) {
  WABT_TRACE(ParseAssertActionCommand);
  EXPECT(Lpar);
  CHECK_RESULT(Expect(token_type));
  auto action = make_unique<Action>();
  CHECK_RESULT(ParseAction(action.get()));
  EXPECT(Rpar);
  out_command->reset(new T(action.release()));
  return Result::Ok;
}

template <typename T>
Result WastParser::ParseAssertActionTextCommand(TokenType token_type,
                                                CommandPtr* out_command) {
  WABT_TRACE(ParseAssertActionTextCommand);
  EXPECT(Lpar);
  CHECK_RESULT(Expect(token_type));
  auto action = make_unique<Action>();
  std::string text;
  CHECK_RESULT(ParseAction(action.get()));
  CHECK_RESULT(ParseQuotedText(&text));
  EXPECT(Rpar);
  out_command->reset(new T(action.release(), text));
  return Result::Ok;
}

template <typename T>
Result WastParser::ParseAssertScriptModuleCommand(TokenType token_type,
                                                  CommandPtr* out_command) {
  WABT_TRACE(ParseAssertScriptModuleCommand);
  EXPECT(Lpar);
  CHECK_RESULT(Expect(token_type));
  std::unique_ptr<ScriptModule> module;
  std::string text;
  CHECK_RESULT(ParseScriptModule(&module));
  CHECK_RESULT(ParseQuotedText(&text));
  EXPECT(Rpar);
  out_command->reset(new T(module.release(), text));
  return Result::Ok;
}

void WastParser::CheckImportOrdering(Module* module) {
  if (module->funcs.size() != module->num_func_imports ||
      module->tables.size() != module->num_table_imports ||
      module->memories.size() != module->num_memory_imports ||
      module->globals.size() != module->num_global_imports ||
      module->excepts.size() != module->num_except_imports) {
    Error(GetLocation(),
          "imports must occur before all non-import definitions");
  }
}

Result ParseWast(WastLexer* lexer,
                 Script** out_script,
                 ErrorHandler* error_handler,
                 WastParseOptions* options) {
  auto script = make_unique<Script>();
  WastParser parser(lexer, error_handler, options);
  Result result = parser.ParseScript(script.get());

  if (out_script)
    *out_script = script.release();

  return result;
}

}  // namespace wabt

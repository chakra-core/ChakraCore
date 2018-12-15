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

#include "gtest/gtest.h"

#include <memory>

#include "src/wast-lexer.h"
#include "src/wast-parser.h"

using namespace wabt;

namespace {

std::string repeat(std::string s, size_t count) {
  std::string result;
  for (size_t i = 0; i < count; ++i) {
    result += s;
  }
  return result;
}

Errors ParseInvalidModule(std::string text) {
  auto lexer = WastLexer::CreateBufferLexer("test", text.c_str(), text.size());
  Errors errors;
  std::unique_ptr<Module> module;
  Result result = ParseWatModule(lexer.get(), &module, &errors);
  EXPECT_EQ(Result::Error, result);

  return errors;
}

}  // end of anonymous namespace

TEST(WastParser, LongToken) {
  std::string text;
  text = "(module (data \"";
  text += repeat("a", 0x5000);
  text += "\" \"";
  text += repeat("a", 0x10000);
  text += "\"))";

  Errors errors = ParseInvalidModule(text);
  ASSERT_EQ(1u, errors.size());

  ASSERT_EQ(ErrorLevel::Error, errors[0].error_level);
  ASSERT_EQ(1, errors[0].loc.line);
  ASSERT_EQ(15, errors[0].loc.first_column);
  ASSERT_STREQ(
      R"(unexpected token ""aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa...", expected an offset expr (e.g. (i32.const 123)).)",
      errors[0].message.c_str());
}

TEST(WastParser, LongTokenSpace) {
  std::string text;
  text = "notparen";
  text += repeat(" ", 0x10000);
  text += "notmodule";

  Errors errors = ParseInvalidModule(text);
  ASSERT_EQ(2u, errors.size());

  ASSERT_EQ(ErrorLevel::Error, errors[0].error_level);
  ASSERT_EQ(1, errors[0].loc.line);
  ASSERT_EQ(1, errors[0].loc.first_column);
  ASSERT_STREQ(
      R"(unexpected token "notparen", expected a module field or a module.)",
      errors[0].message.c_str());

  ASSERT_EQ(1, errors[1].loc.line);
  ASSERT_EQ(65545, errors[1].loc.first_column);
  ASSERT_STREQ(R"(unexpected token notmodule, expected EOF.)",
               errors[1].message.c_str());
}

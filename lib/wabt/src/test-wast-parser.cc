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

#include "src/error-handler.h"
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

std::string ParseInvalidModule(std::string text) {
  auto lexer = WastLexer::CreateBufferLexer("test", text.c_str(), text.size());
  const size_t source_line_max_length = 80;
  ErrorHandlerBuffer error_handler(Location::Type::Text,
                                   source_line_max_length);
  std::unique_ptr<Module> module;
  Result result = ParseWatModule(lexer.get(), &module, &error_handler);
  EXPECT_EQ(Result::Error, result);

  return error_handler.buffer();
}

}  // end of anonymous namespace

TEST(WastParser, LongToken) {
  std::string text;
  text = "(module (data \"";
  text += repeat("a", 0x5000);
  text += "\" \"";
  text += repeat("a", 0x10000);
  text += "\"))";

  std::string output = ParseInvalidModule(text);

  const char expected[] =
      R"(test:1:15: error: unexpected token ""aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa...", expected an offset expr (e.g. (i32.const 123)).
(module (data "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa...
              ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
)";

  ASSERT_STREQ(expected, output.c_str());
}

TEST(WastParser, LongTokenSpace) {
  std::string text;
  text = "notparen";
  text += repeat(" ", 0x10000);
  text += "notmodule";

  std::string output = ParseInvalidModule(text);

  const char expected[] =
      R"(test:1:1: error: unexpected token "notparen", expected a module field or a module.
notparen                                                                     ...
^^^^^^^^
test:1:65545: error: unexpected token notmodule, expected EOF.
...                                                                    notmodule
                                                                       ^^^^^^^^^
)";

  ASSERT_STREQ(expected, output.c_str());
}

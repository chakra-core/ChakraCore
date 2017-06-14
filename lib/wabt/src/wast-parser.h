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

#include "wast-lexer.h"

namespace wabt {

struct Script;
class SourceErrorHandler;

struct WastParseOptions {
  bool allow_exceptions = false;
  bool debug_parsing = false;
};

Result parse_wast(WastLexer* lexer, Script** out_script, SourceErrorHandler*,
                  WastParseOptions* options = nullptr);

}  // namespace wabt

#endif /* WABT_WAST_PARSER_H_ */

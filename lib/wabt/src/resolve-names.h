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

#ifndef WABT_RESOLVE_NAMES_H_
#define WABT_RESOLVE_NAMES_H_

#include "common.h"

namespace wabt {

class WastLexer;
struct Module;
struct Script;
class SourceErrorHandler;

Result resolve_names_module(WastLexer*, Module*, SourceErrorHandler*);
Result resolve_names_script(WastLexer*, Script*, SourceErrorHandler*);

}  // namespace wabt

#endif /* WABT_RESOLVE_NAMES_H_ */

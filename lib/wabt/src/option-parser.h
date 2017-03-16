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

#ifndef WABT_OPTION_PARSER_H_
#define WABT_OPTION_PARSER_H_

#include "common.h"

namespace wabt {

enum class HasArgument {
  No = 0,
  Yes = 1,
};

struct Option;
struct OptionParser;
typedef void (*OptionCallback)(struct OptionParser*,
                               struct Option*,
                               const char* argument);
typedef void (*ArgumentCallback)(struct OptionParser*, const char* argument);
typedef void (*OptionErrorCallback)(struct OptionParser*, const char* message);

struct Option {
  int id;
  char short_name;
  const char* long_name;
  const char* metavar;
  HasArgument has_argument;
  const char* help;
};

struct OptionParser {
  const char* description;
  Option* options;
  int num_options;
  OptionCallback on_option;
  ArgumentCallback on_argument;
  OptionErrorCallback on_error;
  void* user_data;

  /* cached after call to parse_options */
  char* argv0;
};

void parse_options(OptionParser* parser, int argc, char** argv);
void print_help(OptionParser* parser, const char* program_name);

}  // namespace wabt

#endif /* WABT_OPTION_PARSER_H_ */

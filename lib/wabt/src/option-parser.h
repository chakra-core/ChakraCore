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

#include <functional>
#include <string>
#include <vector>

#include "src/common.h"

namespace wabt {

class OptionParser {
 public:
  enum class HasArgument { No, Yes };
  enum class ArgumentCount { One, OneOrMore };

  struct Option;
  typedef std::function<void(const char*)> Callback;
  typedef std::function<void()> NullCallback;

  struct Option {
    Option(char short_name,
           const std::string& long_name,
           const std::string& metavar,
           HasArgument has_argument,
           const std::string& help,
           const Callback&);

    char short_name;
    std::string long_name;
    std::string metavar;
    bool has_argument;
    std::string help;
    Callback callback;
  };

  struct Argument {
    Argument(const std::string& name, ArgumentCount, const Callback&);

    std::string name;
    ArgumentCount count;
    Callback callback;
    int handled_count = 0;
  };

  explicit OptionParser(const char* program_name, const char* description);

  void AddOption(const Option&);
  void AddArgument(const std::string& name, ArgumentCount, const Callback&);
  void SetErrorCallback(const Callback&);
  void Parse(int argc, char* argv[]);
  void PrintHelp();

  // Helper functions.
  void AddOption(char short_name,
                 const char* long_name,
                 const char* help,
                 const NullCallback&);
  void AddOption(const char* long_name, const char* help, const NullCallback&);
  void AddOption(char short_name,
                 const char* long_name,
                 const char* metavar,
                 const char* help,
                 const Callback&);
  void AddHelpOption();

 private:
  static int Match(const char* s, const std::string& full, bool has_argument);
  void WABT_PRINTF_FORMAT(2, 3) Errorf(const char* format, ...);
  void HandleArgument(size_t* arg_index, const char* arg_value);

  // Print the error and exit(1).
  void DefaultError(const std::string&);

  std::string program_name_;
  std::string description_;
  std::vector<Option> options_;
  std::vector<Argument> arguments_;
  Callback on_error_;
};

}  // namespace wabt

#endif /* WABT_OPTION_PARSER_H_ */

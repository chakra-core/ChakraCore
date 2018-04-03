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

#include "src/option-parser.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "config.h"

#if HAVE_ALLOCA
#include <alloca.h>
#endif

namespace wabt {

OptionParser::Option::Option(char short_name,
                             const std::string& long_name,
                             const std::string& metavar,
                             HasArgument has_argument,
                             const std::string& help,
                             const Callback& callback)
    : short_name(short_name),
      long_name(long_name),
      metavar(metavar),
      has_argument(has_argument == HasArgument::Yes),
      help(help),
      callback(callback) {}

OptionParser::Argument::Argument(const std::string& name,
                                 ArgumentCount count,
                                 const Callback& callback)
    : name(name), count(count), callback(callback) {}

OptionParser::OptionParser(const char* program_name, const char* description)
    : program_name_(program_name),
      description_(description),
      on_error_([this](const std::string& message) { DefaultError(message); }) {
}

void OptionParser::AddOption(const Option& option) {
  options_.emplace_back(option);
}

void OptionParser::AddArgument(const std::string& name,
                               ArgumentCount count,
                               const Callback& callback) {
  arguments_.emplace_back(name, count, callback);
}

void OptionParser::AddOption(char short_name,
                             const char* long_name,
                             const char* help,
                             const NullCallback& callback) {
  Option option(short_name, long_name, std::string(), HasArgument::No, help,
                [callback](const char*) { callback(); });
  AddOption(option);
}

void OptionParser::AddOption(const char* long_name,
                             const char* help,
                             const NullCallback& callback) {
  Option option('\0', long_name, std::string(), HasArgument::No, help,
                [callback](const char*) { callback(); });
  AddOption(option);
}

void OptionParser::AddOption(char short_name,
                             const char* long_name,
                             const char* metavar,
                             const char* help,
                             const Callback& callback) {
  Option option(short_name, long_name, metavar, HasArgument::Yes, help,
                callback);
  AddOption(option);
}

void OptionParser::AddHelpOption() {
  AddOption('h', "help", "Print this help message", [this]() {
    PrintHelp();
    exit(0);
  });
}

// static
int OptionParser::Match(const char* s,
                        const std::string& full,
                        bool has_argument) {
  int i;
  for (i = 0;; i++) {
    if (full[i] == '\0') {
      // Perfect match. Return +1, so it will be preferred over a longer option
      // with the same prefix.
      if (s[i] == '\0') {
        return i + 1;
      }

      // We want to fail if s is longer than full, e.g. --foobar vs. --foo.
      // However, if s ends with an '=', it's OK.
      if (!(has_argument && s[i] == '=')) {
        return -1;
      }
      break;
    }
    if (s[i] == '\0') {
      break;
    }
    if (s[i] != full[i]) {
      return -1;
    }
  }
  return i;
}

void OptionParser::Errorf(const char* format, ...) {
  WABT_SNPRINTF_ALLOCA(buffer, length, format);
  on_error_(buffer);
}

void OptionParser::DefaultError(const std::string& message) {
  WABT_FATAL("%s\n", message.c_str());
}

void OptionParser::HandleArgument(size_t* arg_index, const char* arg_value) {
  if (*arg_index >= arguments_.size()) {
    Errorf("unexpected argument '%s'", arg_value);
    return;
  }
  Argument& argument = arguments_[*arg_index];
  argument.callback(arg_value);
  argument.handled_count++;

  if (argument.count == ArgumentCount::One) {
    (*arg_index)++;
  }
}

void OptionParser::Parse(int argc, char* argv[]) {
  size_t arg_index = 0;

  for (int i = 1; i < argc; ++i) {
    const char* arg = argv[i];
    if (arg[0] == '-') {
      if (arg[1] == '-') {
        // Long option.
        int best_index = -1;
        int best_length = 0;
        int best_count = 0;
        for (size_t j = 0; j < options_.size(); ++j) {
          const Option& option = options_[j];
          if (!option.long_name.empty()) {
            int match_length =
                Match(&arg[2], option.long_name, option.has_argument);
            if (match_length > best_length) {
              best_index = j;
              best_length = match_length;
              best_count = 1;
            } else if (match_length == best_length && best_length > 0) {
              best_count++;
            }
          }
        }

        if (best_count > 1) {
          Errorf("ambiguous option '%s'", arg);
          continue;
        } else if (best_count == 0) {
          Errorf("unknown option '%s'", arg);
          continue;
        }

        const Option& best_option = options_[best_index];
        const char* option_argument = nullptr;
        if (best_option.has_argument) {
          if (arg[best_length] == '=') {
            option_argument = &arg[best_length + 1];
          } else {
            if (i + 1 == argc || argv[i + 1][0] == '-') {
              Errorf("option '--%s' requires argument",
                     best_option.long_name.c_str());
              continue;
            }
            ++i;
            option_argument = argv[i];
          }
        }
        best_option.callback(option_argument);
      } else {
        // Short option.
        if (arg[1] == '\0') {
          // Just "-".
          HandleArgument(&arg_index, arg);
          continue;
        }

        // Allow short names to be combined, e.g. "-d -v" => "-dv".
        for (int k = 1; arg[k]; ++k) {
          bool matched = false;
          for (const Option& option : options_) {
            if (option.short_name && arg[k] == option.short_name) {
              const char* option_argument = nullptr;
              if (option.has_argument) {
                // A short option with a required argument cannot be followed
                // by other short options_.
                if (arg[k + 1] != '\0') {
                  Errorf("option '-%c' requires argument", option.short_name);
                  break;
                }

                if (i + 1 == argc || argv[i + 1][0] == '-') {
                  Errorf("option '-%c' requires argument", option.short_name);
                  break;
                }
                ++i;
                option_argument = argv[i];
              }
              option.callback(option_argument);
              matched = true;
              break;
            }
          }

          if (!matched) {
            Errorf("unknown option '-%c'", arg[k]);
            continue;
          }
        }
      }
    } else {
      // Non-option argument.
      HandleArgument(&arg_index, arg);
    }
  }

  // For now, all arguments must be provided. Check that the last Argument was
  // handled at least once.
  if (!arguments_.empty() && arguments_.back().handled_count == 0) {
    PrintHelp();
    for (size_t i = arg_index; i < arguments_.size(); ++i) {
      Errorf("expected %s argument.\n", arguments_[i].name.c_str());
    }
  }
}

void OptionParser::PrintHelp() {
  printf("usage: %s [options]", program_name_.c_str());

  for (size_t i = 0; i < arguments_.size(); ++i) {
    Argument& argument = arguments_[i];
    switch (argument.count) {
      case ArgumentCount::One:
        printf(" %s", argument.name.c_str());
        break;

      case ArgumentCount::OneOrMore:
        printf(" %s+", argument.name.c_str());
        break;
    }
  }

  printf("\n\n");
  printf("%s\n", description_.c_str());
  printf("options:\n");

  const size_t kExtraSpace = 8;
  size_t longest_name_length = 0;
  for (const Option& option : options_) {
    size_t length;
    if (!option.long_name.empty()) {
      length = option.long_name.size();
      if (!option.metavar.empty()) {
        // +1 for '='.
        length += option.metavar.size() + 1;
      }
    } else {
      continue;
    }

    if (length > longest_name_length) {
      longest_name_length = length;
    }
  }

  for (const Option& option : options_) {
    if (!option.short_name && option.long_name.empty()) {
      continue;
    }

    std::string line;
    if (option.short_name) {
      line += std::string("  -") + option.short_name + ", ";
    } else {
      line += "      ";
    }

    std::string flag;
    if (!option.long_name.empty()) {
      flag = "--";
      if (!option.metavar.empty()) {
        flag += option.long_name + '=' + option.metavar;
      } else {
        flag += option.long_name;
      }
    }

    // +2 for "--" of the long flag name.
    size_t remaining = longest_name_length + kExtraSpace + 2 - flag.size();
    line += flag + std::string(remaining, ' ');

    if (!option.help.empty()) {
      line += option.help;
    }
    printf("%s\n", line.c_str());
  }
}

}  // namespace wabt

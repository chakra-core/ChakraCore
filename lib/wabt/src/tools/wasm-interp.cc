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

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "src/binary-reader-interp.h"
#include "src/binary-reader.h"
#include "src/cast.h"
#include "src/error-handler.h"
#include "src/feature.h"
#include "src/interp.h"
#include "src/literal.h"
#include "src/option-parser.h"
#include "src/resolve-names.h"
#include "src/stream.h"
#include "src/validator.h"
#include "src/wast-lexer.h"
#include "src/wast-parser.h"

using namespace wabt;
using namespace wabt::interp;

static int s_verbose;
static const char* s_infile;
static Thread::Options s_thread_options;
static Stream* s_trace_stream;
static bool s_run_all_exports;
static bool s_host_print;
static Features s_features;

static std::unique_ptr<FileStream> s_log_stream;
static std::unique_ptr<FileStream> s_stdout_stream;

enum class RunVerbosity {
  Quiet = 0,
  Verbose = 1,
};

static const char s_description[] =
    R"(  read a file in the wasm binary format, and run in it a stack-based
  interpreter.

examples:
  # parse binary file test.wasm, and type-check it
  $ wasm-interp test.wasm

  # parse test.wasm and run all its exported functions
  $ wasm-interp test.wasm --run-all-exports

  # parse test.wasm, run the exported functions and trace the output
  $ wasm-interp test.wasm --run-all-exports --trace

  # parse test.wasm and run all its exported functions, setting the
  # value stack size to 100 elements
  $ wasm-interp test.wasm -V 100 --run-all-exports
)";

static void ParseOptions(int argc, char** argv) {
  OptionParser parser("wasm-interp", s_description);

  parser.AddOption('v', "verbose", "Use multiple times for more info", []() {
    s_verbose++;
    s_log_stream = FileStream::CreateStdout();
  });
  parser.AddHelpOption();
  s_features.AddOptions(&parser);
  parser.AddOption('V', "value-stack-size", "SIZE",
                   "Size in elements of the value stack",
                   [](const std::string& argument) {
                     // TODO(binji): validate.
                     s_thread_options.value_stack_size = atoi(argument.c_str());
                   });
  parser.AddOption('C', "call-stack-size", "SIZE",
                   "Size in elements of the call stack",
                   [](const std::string& argument) {
                     // TODO(binji): validate.
                     s_thread_options.call_stack_size = atoi(argument.c_str());
                   });
  parser.AddOption('t', "trace", "Trace execution",
                   []() { s_trace_stream = s_stdout_stream.get(); });
  parser.AddOption(
      "run-all-exports",
      "Run all the exported functions, in order. Useful for testing",
      []() { s_run_all_exports = true; });
  parser.AddOption("host-print",
                   "Include an importable function named \"host.print\" for "
                   "printing to stdout",
                   []() { s_host_print = true; });

  parser.AddArgument("filename", OptionParser::ArgumentCount::One,
                     [](const char* argument) { s_infile = argument; });
  parser.Parse(argc, argv);
}

static void RunAllExports(interp::Module* module,
                          Executor* executor,
                          RunVerbosity verbose) {
  TypedValues args;
  TypedValues results;
  for (const interp::Export& export_ : module->exports) {
    ExecResult exec_result = executor->RunExport(&export_, args);
    if (verbose == RunVerbosity::Verbose) {
      WriteCall(s_stdout_stream.get(), string_view(), export_.name, args,
                exec_result.values, exec_result.result);
    }
  }
}

static wabt::Result ReadModule(const char* module_filename,
                               Environment* env,
                               ErrorHandler* error_handler,
                               DefinedModule** out_module) {
  wabt::Result result;
  std::vector<uint8_t> file_data;

  *out_module = nullptr;

  result = ReadFile(module_filename, &file_data);
  if (Succeeded(result)) {
    const bool kReadDebugNames = true;
    const bool kStopOnFirstError = true;
    ReadBinaryOptions options(s_features, s_log_stream.get(), kReadDebugNames,
                              kStopOnFirstError);
    result = ReadBinaryInterp(env, file_data.data(), file_data.size(),
                              &options, error_handler, out_module);

    if (Succeeded(result)) {
      if (s_verbose) {
        env->DisassembleModule(s_stdout_stream.get(), *out_module);
      }
    }
  }
  return result;
}

#define PRIimport "\"" PRIstringview "." PRIstringview "\""
#define PRINTF_IMPORT_ARG(x)                   \
  WABT_PRINTF_STRING_VIEW_ARG((x).module_name) \
  , WABT_PRINTF_STRING_VIEW_ARG((x).field_name)

class WasmInterpHostImportDelegate : public HostImportDelegate {
 public:
  wabt::Result ImportFunc(interp::FuncImport* import,
                          interp::Func* func,
                          interp::FuncSignature* func_sig,
                          const ErrorCallback& callback) override {
    if (import->field_name == "print") {
      cast<HostFunc>(func)->callback = PrintCallback;
      return wabt::Result::Ok;
    } else {
      PrintError(callback, "unknown host function import " PRIimport,
                 PRINTF_IMPORT_ARG(*import));
      return wabt::Result::Error;
    }
  }

  wabt::Result ImportTable(interp::TableImport* import,
                           interp::Table* table,
                           const ErrorCallback& callback) override {
    return wabt::Result::Error;
  }

  wabt::Result ImportMemory(interp::MemoryImport* import,
                            interp::Memory* memory,
                            const ErrorCallback& callback) override {
    return wabt::Result::Error;
  }

  wabt::Result ImportGlobal(interp::GlobalImport* import,
                            interp::Global* global,
                            const ErrorCallback& callback) override {
    return wabt::Result::Error;
  }

 private:
  static interp::Result PrintCallback(const HostFunc* func,
                                      const interp::FuncSignature* sig,
                                      Index num_args,
                                      TypedValue* args,
                                      Index num_results,
                                      TypedValue* out_results,
                                      void* user_data) {
    memset(out_results, 0, sizeof(TypedValue) * num_results);
    for (Index i = 0; i < num_results; ++i)
      out_results[i].type = sig->result_types[i];

    TypedValues vec_args(args, args + num_args);
    TypedValues vec_results(out_results, out_results + num_results);

    printf("called host ");
    WriteCall(s_stdout_stream.get(), func->module_name, func->field_name,
              vec_args, vec_results, interp::Result::Ok);
    return interp::Result::Ok;
  }

  void PrintError(const ErrorCallback& callback, const char* format, ...) {
    WABT_SNPRINTF_ALLOCA(buffer, length, format);
    callback(buffer);
  }
};

static void InitEnvironment(Environment* env) {
  if (s_host_print) {
    HostModule* host_module = env->AppendHostModule("host");
    host_module->import_delegate.reset(new WasmInterpHostImportDelegate());
  }
}

static wabt::Result ReadAndRunModule(const char* module_filename) {
  wabt::Result result;
  Environment env;
  InitEnvironment(&env);

  ErrorHandlerFile error_handler(Location::Type::Binary);
  DefinedModule* module = nullptr;
  result = ReadModule(module_filename, &env, &error_handler, &module);
  if (Succeeded(result)) {
    Executor executor(&env, s_trace_stream, s_thread_options);
    ExecResult exec_result = executor.RunStartFunction(module);
    if (exec_result.result == interp::Result::Ok) {
      if (s_run_all_exports) {
        RunAllExports(module, &executor, RunVerbosity::Verbose);
      }
    } else {
      WriteResult(s_stdout_stream.get(), "error running start function",
                  exec_result.result);
    }
  }
  return result;
}

int ProgramMain(int argc, char** argv) {
  InitStdio();
  s_stdout_stream = FileStream::CreateStdout();

  ParseOptions(argc, argv);

  wabt::Result result = ReadAndRunModule(s_infile);
  return result != wabt::Result::Ok;
}

int main(int argc, char** argv) {
  WABT_TRY
  return ProgramMain(argc, argv);
  WABT_CATCH_BAD_ALLOC_AND_EXIT
}

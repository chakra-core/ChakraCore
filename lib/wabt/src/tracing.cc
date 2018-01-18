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

#define WABT_TRACING 1
#include "src/tracing.h"

namespace {

size_t indent = 0;
const char* indent_text = "  ";

void Fill() {
  for (size_t i = 0; i < indent; ++i)
    fputs(indent_text, stderr);
}

void Indent() {
  Fill();
  ++indent;
}

void Dedent() {
  if (indent) {
    --indent;
  }
  Fill();
}

}  // end of anonymous namespace

namespace wabt {

// static

TraceScope::TraceScope(const char* method) : method_(method) {
  PrintEnter(method);
  PrintNewline();
}

TraceScope::~TraceScope() {
  Dedent();
  fputs("<- ", stderr);
  fputs(method_, stderr);
  fputc('\n', stderr);
}

void TraceScope::PrintEnter(const char* method) {
  Indent();
  fputs("-> ", stderr);
  fputs(method, stderr);
  fputs("(", stderr);
}

void TraceScope::PrintNewline() {
  fputs(")\n", stderr);
}

}  // end of namespace wabt

/*
 * Copyright 2018 WebAssembly Community Group participants
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

#include "src/binary-reader.h"
#include "src/binary-reader-nop.h"
#include "src/leb128.h"
#include "src/opcode.h"

using namespace wabt;

namespace {

struct BinaryReaderError : BinaryReaderNop {
  bool OnError(const Error& error) override {
    first_error = error;
    return true;  // Error handled.
  }

  Error first_error;
};

}  // End of anonymous namespace

TEST(BinaryReader, DisabledOpcodes) {
  // Use the default features.
  ReadBinaryOptions options;

  // Loop through all opcodes.
  for (uint32_t i = 0; i < static_cast<uint32_t>(Opcode::Invalid); ++i) {
    Opcode opcode(static_cast<Opcode::Enum>(i));
    if (opcode.IsEnabled(options.features)) {
      continue;
    }

    // Use a shorter name to make the clang-formatted table below look nicer.
    std::vector<uint8_t> b = opcode.GetBytes();
    assert(b.size() <= 3);
    b.resize(3);

    uint8_t data[] = {
        0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,  // magic + version
        0x01, 0x04, 0x01, 0x60, 0x00, 0x00,  // type section: 1 type, (func)
        0x03, 0x02, 0x01, 0x00,              // func section: 1 func, type 0
        0x0a, 0x07, 0x01, 0x05, 0x00,        // code section: 1 func, 0 locals
        b[0], b[1], b[2],  // The instruction, padded with zeroes
        0x0b,              // end
    };
    const size_t size = sizeof(data);

    BinaryReaderError reader;
    Result result = ReadBinary(data, size, &reader, options);
    EXPECT_EQ(Result::Error, result);

    // This relies on the binary reader checking whether the opcode is allowed
    // before reading any further data needed by the instruction.
    const std::string& message = reader.first_error.message;
    EXPECT_EQ(0u, message.find("unexpected opcode"))
        << "Got error message: " << message;
  }
}

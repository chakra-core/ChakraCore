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

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "src/binary-reader.h"
#include "src/cast.h"
#include "src/interp/binary-reader-interp.h"
#include "src/interp/interp.h"
#include "src/make-unique.h"

using namespace wabt;

namespace {

interp::Result TrapCallback(const interp::HostFunc* func,
                            const interp::FuncSignature* sig,
                            const interp::TypedValues& args,
                            interp::TypedValues& results) {
  return interp::Result::TrapHostTrapped;
}

class HostTrapTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    interp::HostModule* host_module = env_.AppendHostModule("host");
    host_module->AppendFuncExport("a", {{}, {}}, TrapCallback);
  }

  virtual void TearDown() {}

  interp::ExecResult LoadModuleAndRunStartFunction(
      const std::vector<uint8_t>& data) {
    Errors errors;
    interp::DefinedModule* module = nullptr;
    ReadBinaryOptions options;
    Result result = ReadBinaryInterp(&env_, data.data(), data.size(), options,
                                     &errors, &module);
    EXPECT_EQ(Result::Ok, result);

    if (result == Result::Ok) {
      interp::Executor executor(&env_);
      return executor.RunStartFunction(module);
    } else {
      return {};
    }
  }

  interp::Environment env_;
};

}  // end of anonymous namespace

TEST_F(HostTrapTest, Call) {
  // (import "host" "a" (func $0))
  // (func $1 call $0)
  // (start $1)
  std::vector<uint8_t> data = {
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01,
      0x60, 0x00, 0x00, 0x02, 0x0a, 0x01, 0x04, 0x68, 0x6f, 0x73, 0x74,
      0x01, 0x61, 0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x08, 0x01, 0x01,
      0x0a, 0x06, 0x01, 0x04, 0x00, 0x10, 0x00, 0x0b,
  };
  ASSERT_EQ(interp::Result::TrapHostTrapped,
            LoadModuleAndRunStartFunction(data).result);
}

TEST_F(HostTrapTest, CallIndirect) {
  // (import "host" "a" (func $0))
  // (table anyfunc (elem $0))
  // (func $1 i32.const 0 call_indirect)
  // (start $1)
  std::vector<uint8_t> data = {
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
      0x00, 0x00, 0x02, 0x0a, 0x01, 0x04, 0x68, 0x6f, 0x73, 0x74, 0x01, 0x61,
      0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x04, 0x05, 0x01, 0x70, 0x01, 0x01,
      0x01, 0x08, 0x01, 0x01, 0x09, 0x07, 0x01, 0x00, 0x41, 0x00, 0x0b, 0x01,
      0x00, 0x0a, 0x09, 0x01, 0x07, 0x00, 0x41, 0x00, 0x11, 0x00, 0x00, 0x0b,
  };
  ASSERT_EQ(interp::Result::TrapHostTrapped,
            LoadModuleAndRunStartFunction(data).result);
}

namespace {

class HostMemoryTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    interp::HostModule* host_module = env_.AppendHostModule("host");
    executor_ = MakeUnique<interp::Executor>(&env_);
    std::pair<interp::Memory*, Index> pair =
        host_module->AppendMemoryExport("mem", Limits(1));

    using namespace std::placeholders;

    host_module->AppendFuncExport(
        "fill_buf", {{Type::I32, Type::I32}, {Type::I32}},
        std::bind(&HostMemoryTest::FillBufCallback, this, _1, _2, _3, _4));
    host_module->AppendFuncExport(
        "buf_done", {{Type::I32, Type::I32}, {}},
        std::bind(&HostMemoryTest::BufDoneCallback, this, _1, _2, _3, _4));
    memory_ = pair.first;
  }

  virtual void TearDown() {
    executor_.reset();
  }

  Result LoadModule(const std::vector<uint8_t>& data) {
    Errors errors;
    ReadBinaryOptions options;
    return ReadBinaryInterp(&env_, data.data(), data.size(), options, &errors,
                            &module_);
  }

  std::string string_data;

  interp::Result FillBufCallback(const interp::HostFunc* func,
                                 const interp::FuncSignature* sig,
                                 const interp::TypedValues& args,
                                 interp::TypedValues& results) {
    // (param $ptr i32) (param $max_size i32) (result $size i32)
    EXPECT_EQ(2u, args.size());
    EXPECT_EQ(Type::I32, args[0].type);
    EXPECT_EQ(Type::I32, args[1].type);
    EXPECT_EQ(1u, results.size());
    EXPECT_EQ(Type::I32, results[0].type);

    uint32_t ptr = args[0].get_i32();
    uint32_t max_size = args[1].get_i32();
    uint32_t size = std::min(max_size, uint32_t(string_data.size()));

    EXPECT_LT(ptr + size, memory_->data.size());

    std::copy(string_data.begin(), string_data.begin() + size,
              memory_->data.begin() + ptr);

    results[0].set_i32(size);
    return interp::Result::Ok;
  }

  interp::Result BufDoneCallback(const interp::HostFunc* func,
                                 const interp::FuncSignature* sig,
                                 const interp::TypedValues& args,
                                 interp::TypedValues& results) {
    // (param $ptr i32) (param $size i32)
    EXPECT_EQ(2u, args.size());
    EXPECT_EQ(Type::I32, args[0].type);
    EXPECT_EQ(Type::I32, args[1].type);
    EXPECT_EQ(0u, results.size());

    uint32_t ptr = args[0].get_i32();
    uint32_t size = args[1].get_i32();

    EXPECT_LT(ptr + size, memory_->data.size());

    string_data.resize(size);
    std::copy(memory_->data.begin() + ptr, memory_->data.begin() + ptr + size,
              string_data.begin());

    return interp::Result::Ok;
  }

  interp::Environment env_;
  interp::Memory* memory_;
  interp::DefinedModule* module_;
  std::unique_ptr<interp::Executor> executor_;
};

}  // end of anonymous namespace

TEST_F(HostMemoryTest, Rot13) {
  // (import "host" "mem" (memory $mem 1))
  // (import "host" "fill_buf" (func $fill_buf (param i32 i32) (result i32)))
  // (import "host" "buf_done" (func $buf_done (param i32 i32)))
  //
  // (func $rot13c (param $c i32) (result i32)
  //   (local $uc i32)
  //
  //   ;; No change if < 'A'.
  //   (if (i32.lt_u (get_local $c) (i32.const 65))
  //     (return (get_local $c)))
  //
  //   ;; Clear 5th bit of c, to force uppercase. 0xdf = 0b11011111
  //   (set_local $uc (i32.and (get_local $c) (i32.const 0xdf)))
  //
  //   ;; In range ['A', 'M'] return |c| + 13.
  //   (if (i32.le_u (get_local $uc) (i32.const 77))
  //     (return (i32.add (get_local $c) (i32.const 13))))
  //
  //   ;; In range ['N', 'Z'] return |c| - 13.
  //   (if (i32.le_u (get_local $uc) (i32.const 90))
  //     (return (i32.sub (get_local $c) (i32.const 13))))
  //
  //   ;; No change for everything else.
  //   (return (get_local $c))
  // )
  //
  // (func (export "rot13")
  //   (local $size i32)
  //   (local $i i32)
  //
  //   ;; Ask host to fill memory [0, 1024) with data.
  //   (call $fill_buf (i32.const 0) (i32.const 1024))
  //
  //   ;; The host returns the size filled.
  //   (set_local $size)
  //
  //   ;; Loop over all bytes and rot13 them.
  //   (block $exit
  //     (loop $top
  //       ;; if (i >= size) break
  //       (if (i32.ge_u (get_local $i) (get_local $size)) (br $exit))
  //
  //       ;; mem[i] = rot13c(mem[i])
  //       (i32.store8
  //         (get_local $i)
  //         (call $rot13c
  //           (i32.load8_u (get_local $i))))
  //
  //       ;; i++
  //       (set_local $i (i32.add (get_local $i) (i32.const 1)))
  //       (br $top)
  //     )
  //   )
  //
  //   (call $buf_done (i32.const 0) (get_local $size))
  // )
  std::vector<uint8_t> data = {
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x14, 0x04, 0x60,
      0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x60, 0x02, 0x7f, 0x7f, 0x00, 0x60, 0x01,
      0x7f, 0x01, 0x7f, 0x60, 0x00, 0x00, 0x02, 0x2d, 0x03, 0x04, 0x68, 0x6f,
      0x73, 0x74, 0x03, 0x6d, 0x65, 0x6d, 0x02, 0x00, 0x01, 0x04, 0x68, 0x6f,
      0x73, 0x74, 0x08, 0x66, 0x69, 0x6c, 0x6c, 0x5f, 0x62, 0x75, 0x66, 0x00,
      0x00, 0x04, 0x68, 0x6f, 0x73, 0x74, 0x08, 0x62, 0x75, 0x66, 0x5f, 0x64,
      0x6f, 0x6e, 0x65, 0x00, 0x01, 0x03, 0x03, 0x02, 0x02, 0x03, 0x07, 0x09,
      0x01, 0x05, 0x72, 0x6f, 0x74, 0x31, 0x33, 0x00, 0x03, 0x0a, 0x74, 0x02,
      0x39, 0x01, 0x01, 0x7f, 0x20, 0x00, 0x41, 0xc1, 0x00, 0x49, 0x04, 0x40,
      0x20, 0x00, 0x0f, 0x0b, 0x20, 0x00, 0x41, 0xdf, 0x01, 0x71, 0x21, 0x01,
      0x20, 0x01, 0x41, 0xcd, 0x00, 0x4d, 0x04, 0x40, 0x20, 0x00, 0x41, 0x0d,
      0x6a, 0x0f, 0x0b, 0x20, 0x01, 0x41, 0xda, 0x00, 0x4d, 0x04, 0x40, 0x20,
      0x00, 0x41, 0x0d, 0x6b, 0x0f, 0x0b, 0x20, 0x00, 0x0f, 0x0b, 0x38, 0x01,
      0x02, 0x7f, 0x41, 0x00, 0x41, 0x80, 0x08, 0x10, 0x00, 0x21, 0x00, 0x02,
      0x40, 0x03, 0x40, 0x20, 0x01, 0x20, 0x00, 0x4f, 0x04, 0x40, 0x0c, 0x02,
      0x0b, 0x20, 0x01, 0x20, 0x01, 0x2d, 0x00, 0x00, 0x10, 0x02, 0x3a, 0x00,
      0x00, 0x20, 0x01, 0x41, 0x01, 0x6a, 0x21, 0x01, 0x0c, 0x00, 0x0b, 0x0b,
      0x41, 0x00, 0x20, 0x00, 0x10, 0x01, 0x0b,
  };

  ASSERT_EQ(Result::Ok, LoadModule(data));

  string_data = "Hello, WebAssembly!";

  ASSERT_EQ(interp::Result::Ok,
            executor_->RunExportByName(module_, "rot13", {}).result);

  ASSERT_EQ("Uryyb, JroNffrzoyl!", string_data);

  ASSERT_EQ(interp::Result::Ok,
            executor_->RunExportByName(module_, "rot13", {}).result);

  ASSERT_EQ("Hello, WebAssembly!", string_data);
}

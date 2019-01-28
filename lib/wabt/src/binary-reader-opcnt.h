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

#ifndef WABT_BINARY_READER_OPCNT_H_
#define WABT_BINARY_READER_OPCNT_H_

#include <map>
#include <vector>

#include "src/common.h"
#include "src/opcode.h"

namespace wabt {

struct Module;
struct ReadBinaryOptions;
class Stream;

class OpcodeInfo {
 public:
  enum class Kind {
    Bare,
    Uint32,
    Uint64,
    Index,
    Float32,
    Float64,
    Uint32Uint32,
    BlockSig,
    BrTable,
  };

  explicit OpcodeInfo(Opcode, Kind);
  template <typename T>
  OpcodeInfo(Opcode, Kind, T* data, size_t count = 1);
  template <typename T>
  OpcodeInfo(Opcode, Kind, T* data, size_t count, T extra);

  Opcode opcode() const { return opcode_; }

  void Write(Stream&);

 private:
  template <typename T>
  std::pair<const T*, size_t> GetDataArray() const;
  template <typename T>
  const T* GetData(size_t expected_size = 1) const;

  template <typename T, typename F>
  void WriteArray(Stream& stream, F&& write_func);

  Opcode opcode_;
  Kind kind_;
  std::vector<uint8_t> data_;

  friend bool operator==(const OpcodeInfo&, const OpcodeInfo&);
  friend bool operator!=(const OpcodeInfo&, const OpcodeInfo&);
  friend bool operator<(const OpcodeInfo&, const OpcodeInfo&);
  friend bool operator<=(const OpcodeInfo&, const OpcodeInfo&);
  friend bool operator>(const OpcodeInfo&, const OpcodeInfo&);
  friend bool operator>=(const OpcodeInfo&, const OpcodeInfo&);
};

bool operator==(const OpcodeInfo&, const OpcodeInfo&);
bool operator!=(const OpcodeInfo&, const OpcodeInfo&);
bool operator<(const OpcodeInfo&, const OpcodeInfo&);
bool operator<=(const OpcodeInfo&, const OpcodeInfo&);
bool operator>(const OpcodeInfo&, const OpcodeInfo&);
bool operator>=(const OpcodeInfo&, const OpcodeInfo&);

typedef std::map<OpcodeInfo, size_t> OpcodeInfoCounts;

Result ReadBinaryOpcnt(const void* data,
                       size_t size,
                       const ReadBinaryOptions& options,
                       OpcodeInfoCounts* opcode_counts);

}  // namespace wabt

#endif /* WABT_BINARY_READER_OPCNT_H_ */

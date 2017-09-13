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

#include "src/binary-reader-opcnt.h"

#include <cassert>
#include <cinttypes>
#include <cstdarg>
#include <cstdint>
#include <cstdio>

#include "src/binary-reader-nop.h"
#include "src/common.h"
#include "src/literal.h"
#include "src/stream.h"

namespace wabt {

OpcodeInfo::OpcodeInfo(Opcode opcode, Kind kind)
    : opcode_(opcode), kind_(kind) {}

template <typename T>
OpcodeInfo::OpcodeInfo(Opcode opcode, Kind kind, T* data, size_t count)
    : OpcodeInfo(opcode, kind) {
  if (count > 0) {
    data_.resize(sizeof(T) * count);
    memcpy(data_.data(), data, data_.size());
  }
}

template <typename T>
OpcodeInfo::OpcodeInfo(Opcode opcode, Kind kind, T* data, size_t count, T extra)
    : OpcodeInfo(opcode, kind, data, count) {
  data_.resize(data_.size() + sizeof(T));
  memcpy(data_.data() + data_.size() - sizeof(T), &extra, sizeof(T));
}

template <typename T>
std::pair<const T*, size_t> OpcodeInfo::GetDataArray() const {
  if (data_.empty()) {
    return std::pair<const T*, size_t>(nullptr, 0);
  }

  assert(data_.size() % sizeof(T) == 0);
  return std::make_pair(reinterpret_cast<const T*>(data_.data()),
                        data_.size() / sizeof(T));
}

template <typename T>
const T* OpcodeInfo::GetData(size_t expected_size) const {
  auto pair = GetDataArray<T>();
  assert(pair.second == expected_size);
  return pair.first;
}

template <typename T, typename F>
void OpcodeInfo::WriteArray(Stream& stream, F&& write_func) {
  auto pair = GetDataArray<T>();
  for (size_t i = 0; i < pair.second; ++i) {
    // Write an initial space (to separate from the opcode name) first, then
    // comma-separate.
    stream.Writef("%s", i == 0 ? " " : ", ");
    write_func(pair.first[i]);
  }
}

void OpcodeInfo::Write(Stream& stream) {
  stream.Writef("%s", opcode_.GetName());

  switch (kind_) {
    case Kind::Bare:
      break;

    case Kind::Uint32:
      stream.Writef(" %u (0x%x)", *GetData<uint32_t>(), *GetData<uint32_t>());
      break;

    case Kind::Uint64:
      stream.Writef(" %" PRIu64 " (0x%" PRIx64 ")", *GetData<uint64_t>(),
                    *GetData<uint64_t>());
      break;

    case Kind::Index:
      stream.Writef(" %" PRIindex, *GetData<Index>());
      break;

    case Kind::Float32: {
      stream.Writef(" %g", *GetData<float>());
      char buffer[WABT_MAX_FLOAT_HEX + 1];
      WriteFloatHex(buffer, sizeof(buffer), *GetData<uint32_t>());
      stream.Writef(" (%s)", buffer);
      break;
    }

    case Kind::Float64: {
      stream.Writef(" %g", *GetData<double>());
      char buffer[WABT_MAX_DOUBLE_HEX + 1];
      WriteDoubleHex(buffer, sizeof(buffer), *GetData<uint64_t>());
      stream.Writef(" (%s)", buffer);
      break;
    }

    case Kind::Uint32Uint32:
      WriteArray<uint32_t>(
          stream, [&stream](uint32_t value) { stream.Writef("%u", value); });
      break;

    case Kind::BlockSig:
      WriteArray<Type>(stream, [&stream](Type type) {
        stream.Writef("%s", GetTypeName(type));
      });
      break;

    case Kind::BrTable: {
      WriteArray<Index>(stream, [&stream](Index index) {
        stream.Writef("%" PRIindex, index);
      });
      break;
    }
  }
}

bool operator==(const OpcodeInfo& lhs, const OpcodeInfo& rhs) {
  return lhs.opcode_ == rhs.opcode_ && lhs.kind_ == rhs.kind_ &&
         lhs.data_ == rhs.data_;
}

bool operator!=(const OpcodeInfo& lhs, const OpcodeInfo& rhs) {
  return !(lhs == rhs);
}

bool operator<(const OpcodeInfo& lhs, const OpcodeInfo& rhs) {
  if (lhs.opcode_ < rhs.opcode_) return true;
  if (lhs.opcode_ > rhs.opcode_) return false;
  if (lhs.kind_ < rhs.kind_) return true;
  if (lhs.kind_ > rhs.kind_) return false;
  if (lhs.data_ < rhs.data_) return true;
  if (lhs.data_ > rhs.data_) return false;
  return false;
}

bool operator<=(const OpcodeInfo& lhs, const OpcodeInfo& rhs) {
  return lhs < rhs || lhs == rhs;
}

bool operator>(const OpcodeInfo& lhs, const OpcodeInfo& rhs) {
  return !(lhs <= rhs);
}

bool operator>=(const OpcodeInfo& lhs, const OpcodeInfo& rhs) {
  return !(lhs < rhs);
}

namespace {

class BinaryReaderOpcnt : public BinaryReaderNop {
 public:
  explicit BinaryReaderOpcnt(OpcodeInfoCounts* counts);

  Result OnOpcode(Opcode opcode) override;
  Result OnOpcodeBare() override;
  Result OnOpcodeUint32(uint32_t value) override;
  Result OnOpcodeIndex(Index value) override;
  Result OnOpcodeUint32Uint32(uint32_t value, uint32_t value2) override;
  Result OnOpcodeUint64(uint64_t value) override;
  Result OnOpcodeF32(uint32_t value) override;
  Result OnOpcodeF64(uint64_t value) override;
  Result OnOpcodeBlockSig(Index num_types, Type* sig_types) override;
  Result OnBrTableExpr(Index num_targets,
                       Index* target_depths,
                       Index default_target_depth) override;
  Result OnEndExpr() override;
  Result OnEndFunc() override;

 private:
  template <typename... Args>
  Result Emplace(Args&&... args);

  OpcodeInfoCounts* opcode_counts_;
  Opcode current_opcode_;
};

template <typename... Args>
Result BinaryReaderOpcnt::Emplace(Args&&... args) {
  auto pair = opcode_counts_->emplace(
      std::piecewise_construct, std::make_tuple(std::forward<Args>(args)...),
      std::make_tuple(0));

  auto& count = pair.first->second;
  count++;
  return Result::Ok;
}

BinaryReaderOpcnt::BinaryReaderOpcnt(OpcodeInfoCounts* counts)
    : opcode_counts_(counts) {}

Result BinaryReaderOpcnt::OnOpcode(Opcode opcode) {
  current_opcode_ = opcode;
  return Result::Ok;
}

Result BinaryReaderOpcnt::OnOpcodeBare() {
  return Emplace(current_opcode_, OpcodeInfo::Kind::Bare);
}

Result BinaryReaderOpcnt::OnOpcodeUint32(uint32_t value) {
  return Emplace(current_opcode_, OpcodeInfo::Kind::Uint32, &value);
}

Result BinaryReaderOpcnt::OnOpcodeIndex(Index value) {
  return Emplace(current_opcode_, OpcodeInfo::Kind::Index, &value);
}

Result BinaryReaderOpcnt::OnOpcodeUint32Uint32(uint32_t value0,
                                               uint32_t value1) {
  uint32_t array[2] = {value0, value1};
  return Emplace(current_opcode_, OpcodeInfo::Kind::Uint32Uint32, array, 2);
}

Result BinaryReaderOpcnt::OnOpcodeUint64(uint64_t value) {
  return Emplace(current_opcode_, OpcodeInfo::Kind::Uint64, &value);
}

Result BinaryReaderOpcnt::OnOpcodeF32(uint32_t value) {
  return Emplace(current_opcode_, OpcodeInfo::Kind::Float32, &value);
}

Result BinaryReaderOpcnt::OnOpcodeF64(uint64_t value) {
  return Emplace(current_opcode_, OpcodeInfo::Kind::Float64, &value);
}

Result BinaryReaderOpcnt::OnOpcodeBlockSig(Index num_types, Type* sig_types) {
  return Emplace(current_opcode_, OpcodeInfo::Kind::BlockSig, sig_types,
                 num_types);
}

Result BinaryReaderOpcnt::OnBrTableExpr(Index num_targets,
                                        Index* target_depths,
                                        Index default_target_depth) {
  return Emplace(current_opcode_, OpcodeInfo::Kind::BrTable, target_depths,
                 num_targets, default_target_depth);
}

Result BinaryReaderOpcnt::OnEndExpr() {
  return Emplace(Opcode::End, OpcodeInfo::Kind::Bare);
}

Result BinaryReaderOpcnt::OnEndFunc() {
  return Emplace(Opcode::End, OpcodeInfo::Kind::Bare);
}

}  // end anonymous namespace

Result ReadBinaryOpcnt(const void* data,
                       size_t size,
                       const struct ReadBinaryOptions* options,
                       OpcodeInfoCounts* counts) {
  BinaryReaderOpcnt reader(counts);
  return ReadBinary(data, size, &reader, options);
}

}  // namespace wabt

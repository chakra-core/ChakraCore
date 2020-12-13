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

namespace wabt {
namespace interp {

// A table entry has the following packed layout:
//
//   struct {
//     IstreamOffset offset;
//     uint32_t drop_count;
//     uint32_t keep_count;
//   };
#define WABT_TABLE_ENTRY_SIZE \
  (sizeof(IstreamOffset) + sizeof(uint32_t) + sizeof(uint32_t))
#define WABT_TABLE_ENTRY_OFFSET_OFFSET 0
#define WABT_TABLE_ENTRY_DROP_OFFSET sizeof(IstreamOffset)
#define WABT_TABLE_ENTRY_KEEP_OFFSET (sizeof(IstreamOffset) + sizeof(uint32_t))

template <typename T>
inline T ReadUxAt(const uint8_t* pc) {
  T result;
  memcpy(&result, pc, sizeof(T));
  return result;
}

template <typename T>
inline T ReadUx(const uint8_t** pc) {
  T result = ReadUxAt<T>(*pc);
  *pc += sizeof(T);
  return result;
}

inline uint8_t ReadU8At(const uint8_t* pc) {
  return ReadUxAt<uint8_t>(pc);
}

inline uint8_t ReadU8(const uint8_t** pc) {
  return ReadUx<uint8_t>(pc);
}

inline uint32_t ReadU32At(const uint8_t* pc) {
  return ReadUxAt<uint32_t>(pc);
}

inline uint32_t ReadU32(const uint8_t** pc) {
  return ReadUx<uint32_t>(pc);
}

inline uint64_t ReadU64At(const uint8_t* pc) {
  return ReadUxAt<uint64_t>(pc);
}

inline uint64_t ReadU64(const uint8_t** pc) {
  return ReadUx<uint64_t>(pc);
}

inline v128 ReadV128At(const uint8_t* pc) {
  return ReadUxAt<v128>(pc);
}

inline v128 ReadV128(const uint8_t** pc) {
  return ReadUx<v128>(pc);
}

inline Opcode ReadOpcode(const uint8_t** pc) {
  uint32_t value = ReadU32(pc);
  return Opcode(static_cast<Opcode::Enum>(value));
}

inline void ReadTableEntryAt(const uint8_t* pc,
                             IstreamOffset* out_offset,
                             uint32_t* out_drop,
                             uint32_t* out_keep) {
  *out_offset = ReadU32At(pc + WABT_TABLE_ENTRY_OFFSET_OFFSET);
  *out_drop = ReadU32At(pc + WABT_TABLE_ENTRY_DROP_OFFSET);
  *out_keep = ReadU32At(pc + WABT_TABLE_ENTRY_KEEP_OFFSET);
}

}  // namespace interp
}  // namespace wabt

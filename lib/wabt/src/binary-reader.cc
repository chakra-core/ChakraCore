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

#include "src/binary-reader.h"

#include <cassert>
#include <cinttypes>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "config.h"

#include "src/binary-reader-logging.h"
#include "src/binary.h"
#include "src/leb128.h"
#include "src/stream.h"
#include "src/utf8.h"

#if HAVE_ALLOCA
#include <alloca.h>
#endif

#define ERROR_UNLESS(expr, ...) \
  do {                          \
    if (!(expr)) {              \
      PrintError(__VA_ARGS__);  \
      return Result::Error;     \
    }                           \
  } while (0)

#define ERROR_UNLESS_OPCODE_ENABLED(opcode)      \
  do {                                           \
    if (!opcode.IsEnabled(options_->features)) { \
      return ReportUnexpectedOpcode(opcode);     \
    }                                            \
  } while (0)

#define CALLBACK0(member) \
  ERROR_UNLESS(Succeeded(delegate_->member()), #member " callback failed")

#define CALLBACK(member, ...)                             \
  ERROR_UNLESS(Succeeded(delegate_->member(__VA_ARGS__)), \
               #member " callback failed")

namespace wabt {

namespace {

class BinaryReader {
 public:
  BinaryReader(const void* data,
               size_t size,
               BinaryReaderDelegate* delegate,
               const ReadBinaryOptions* options);

  Result ReadModule();

 private:
  template <typename T, T BinaryReader::*member>
  struct ValueRestoreGuard {
    explicit ValueRestoreGuard(BinaryReader* this_)
        : this_(this_), previous_value_(this_->*member) {}
    ~ValueRestoreGuard() { this_->*member = previous_value_; }

    BinaryReader* this_;
    T previous_value_;
  };

  void WABT_PRINTF_FORMAT(2, 3) PrintError(const char* format, ...);
  Result ReadOpcode(Opcode* out_value, const char* desc) WABT_WARN_UNUSED;
  template <typename T>
  Result ReadT(T* out_value,
               const char* type_name,
               const char* desc) WABT_WARN_UNUSED;
  Result ReadU8(uint8_t* out_value, const char* desc) WABT_WARN_UNUSED;
  Result ReadU32(uint32_t* out_value, const char* desc) WABT_WARN_UNUSED;
  Result ReadF32(uint32_t* out_value, const char* desc) WABT_WARN_UNUSED;
  Result ReadF64(uint64_t* out_value, const char* desc) WABT_WARN_UNUSED;
  Result ReadV128(v128* out_value, const char* desc) WABT_WARN_UNUSED;
  Result ReadU32Leb128(uint32_t* out_value, const char* desc) WABT_WARN_UNUSED;
  Result ReadS32Leb128(uint32_t* out_value, const char* desc) WABT_WARN_UNUSED;
  Result ReadS64Leb128(uint64_t* out_value, const char* desc) WABT_WARN_UNUSED;
  Result ReadType(Type* out_value, const char* desc) WABT_WARN_UNUSED;
  Result ReadStr(string_view* out_str, const char* desc) WABT_WARN_UNUSED;
  Result ReadBytes(const void** out_data,
                   Address* out_data_size,
                   const char* desc) WABT_WARN_UNUSED;
  Result ReadIndex(Index* index, const char* desc) WABT_WARN_UNUSED;
  Result ReadOffset(Offset* offset, const char* desc) WABT_WARN_UNUSED;
  Result ReadCount(Index* index, const char* desc) WABT_WARN_UNUSED;

  bool IsConcreteType(Type);
  bool IsBlockType(Type);

  Index NumTotalFuncs();
  Index NumTotalTables();
  Index NumTotalMemories();
  Index NumTotalGlobals();

  Result ReadI32InitExpr(Index index) WABT_WARN_UNUSED;
  Result ReadInitExpr(Index index, bool require_i32 = false) WABT_WARN_UNUSED;
  Result ReadTable(Type* out_elem_type,
                   Limits* out_elem_limits) WABT_WARN_UNUSED;
  Result ReadMemory(Limits* out_page_limits) WABT_WARN_UNUSED;
  Result ReadGlobalHeader(Type* out_type, bool* out_mutable) WABT_WARN_UNUSED;
  Result ReadExceptionType(TypeVector& sig) WABT_WARN_UNUSED;
  Result ReadFunctionBody(Offset end_offset) WABT_WARN_UNUSED;
  Result ReadNameSection(Offset section_size) WABT_WARN_UNUSED;
  Result ReadRelocSection(Offset section_size) WABT_WARN_UNUSED;
  Result ReadLinkingSection(Offset section_size) WABT_WARN_UNUSED;
  Result ReadCustomSection(Offset section_size) WABT_WARN_UNUSED;
  Result ReadTypeSection(Offset section_size) WABT_WARN_UNUSED;
  Result ReadImportSection(Offset section_size) WABT_WARN_UNUSED;
  Result ReadFunctionSection(Offset section_size) WABT_WARN_UNUSED;
  Result ReadTableSection(Offset section_size) WABT_WARN_UNUSED;
  Result ReadMemorySection(Offset section_size) WABT_WARN_UNUSED;
  Result ReadGlobalSection(Offset section_size) WABT_WARN_UNUSED;
  Result ReadExportSection(Offset section_size) WABT_WARN_UNUSED;
  Result ReadStartSection(Offset section_size) WABT_WARN_UNUSED;
  Result ReadElemSection(Offset section_size) WABT_WARN_UNUSED;
  Result ReadCodeSection(Offset section_size) WABT_WARN_UNUSED;
  Result ReadDataSection(Offset section_size) WABT_WARN_UNUSED;
  Result ReadExceptionSection(Offset section_size) WABT_WARN_UNUSED;
  Result ReadSections() WABT_WARN_UNUSED;
  Result ReportUnexpectedOpcode(Opcode opcode, const char* message = nullptr);

  size_t read_end_ = 0;  // Either the section end or data_size.
  BinaryReaderDelegate::State state_;
  BinaryReaderLogging logging_delegate_;
  BinaryReaderDelegate* delegate_ = nullptr;
  TypeVector param_types_;
  TypeVector result_types_;
  std::vector<Index> target_depths_;
  const ReadBinaryOptions* options_ = nullptr;
  BinarySection last_known_section_ = BinarySection::Invalid;
  bool did_read_names_section_ = false;
  bool reading_custom_section_ = false;
  Index num_signatures_ = 0;
  Index num_imports_ = 0;
  Index num_func_imports_ = 0;
  Index num_table_imports_ = 0;
  Index num_memory_imports_ = 0;
  Index num_global_imports_ = 0;
  Index num_exception_imports_ = 0;
  Index num_function_signatures_ = 0;
  Index num_tables_ = 0;
  Index num_memories_ = 0;
  Index num_globals_ = 0;
  Index num_exports_ = 0;
  Index num_function_bodies_ = 0;
  Index num_exceptions_ = 0;

  using ReadEndRestoreGuard =
      ValueRestoreGuard<size_t, &BinaryReader::read_end_>;
};

BinaryReader::BinaryReader(const void* data,
                           size_t size,
                           BinaryReaderDelegate* delegate,
                           const ReadBinaryOptions* options)
    : read_end_(size),
      state_(static_cast<const uint8_t*>(data), size),
      logging_delegate_(options->log_stream, delegate),
      delegate_(options->log_stream ? &logging_delegate_ : delegate),
      options_(options),
      last_known_section_(BinarySection::Invalid) {
  delegate->OnSetState(&state_);
}

void WABT_PRINTF_FORMAT(2, 3) BinaryReader::PrintError(const char* format,
                                                       ...) {
  ErrorLevel error_level =
      reading_custom_section_ && !options_->fail_on_custom_section_error
          ? ErrorLevel::Warning
          : ErrorLevel::Error;

  WABT_SNPRINTF_ALLOCA(buffer, length, format);
  bool handled = delegate_->OnError(error_level, buffer);

  if (!handled) {
    // Not great to just print, but we don't want to eat the error either.
    fprintf(stderr, "%07" PRIzx ": %s: %s\n", state_.offset,
            GetErrorLevelName(error_level), buffer);
  }
}

Result BinaryReader::ReportUnexpectedOpcode(Opcode opcode,
                                            const char* message) {
  const char* maybe_space = " ";
  if (!message) {
    message = maybe_space = "";
  }
  if (opcode.HasPrefix()) {
    PrintError("unexpected opcode%s%s: %d %d (0x%x 0x%x)", maybe_space, message,
               opcode.GetPrefix(), opcode.GetCode(), opcode.GetPrefix(),
               opcode.GetCode());
  } else {
    PrintError("unexpected opcode%s%s: %d (0x%x)", maybe_space, message,
               opcode.GetCode(), opcode.GetCode());
  }
  return Result::Error;
}

Result BinaryReader::ReadOpcode(Opcode* out_value, const char* desc) {
  uint8_t value = 0;
  CHECK_RESULT(ReadU8(&value, desc));

  if (Opcode::IsPrefixByte(value)) {
    uint32_t code;
    CHECK_RESULT(ReadU32Leb128(&code, desc));
    *out_value = Opcode::FromCode(value, code);
  } else {
    *out_value = Opcode::FromCode(value);
  }
  return Result::Ok;
}

template <typename T>
Result BinaryReader::ReadT(T* out_value,
                           const char* type_name,
                           const char* desc) {
  if (state_.offset + sizeof(T) > read_end_) {
    PrintError("unable to read %s: %s", type_name, desc);
    return Result::Error;
  }
  memcpy(out_value, state_.data + state_.offset, sizeof(T));
  state_.offset += sizeof(T);
  return Result::Ok;
}

Result BinaryReader::ReadU8(uint8_t* out_value, const char* desc) {
  return ReadT(out_value, "uint8_t", desc);
}

Result BinaryReader::ReadU32(uint32_t* out_value, const char* desc) {
  return ReadT(out_value, "uint32_t", desc);
}

Result BinaryReader::ReadF32(uint32_t* out_value, const char* desc) {
  return ReadT(out_value, "float", desc);
}

Result BinaryReader::ReadF64(uint64_t* out_value, const char* desc) {
  return ReadT(out_value, "double", desc);
}

Result BinaryReader::ReadV128(v128* out_value, const char* desc) {
  return ReadT(out_value, "v128", desc);
}

Result BinaryReader::ReadU32Leb128(uint32_t* out_value, const char* desc) {
  const uint8_t* p = state_.data + state_.offset;
  const uint8_t* end = state_.data + read_end_;
  size_t bytes_read = wabt::ReadU32Leb128(p, end, out_value);
  ERROR_UNLESS(bytes_read > 0, "unable to read u32 leb128: %s", desc);
  state_.offset += bytes_read;
  return Result::Ok;
}

Result BinaryReader::ReadS32Leb128(uint32_t* out_value, const char* desc) {
  const uint8_t* p = state_.data + state_.offset;
  const uint8_t* end = state_.data + read_end_;
  size_t bytes_read = wabt::ReadS32Leb128(p, end, out_value);
  ERROR_UNLESS(bytes_read > 0, "unable to read i32 leb128: %s", desc);
  state_.offset += bytes_read;
  return Result::Ok;
}

Result BinaryReader::ReadS64Leb128(uint64_t* out_value, const char* desc) {
  const uint8_t* p = state_.data + state_.offset;
  const uint8_t* end = state_.data + read_end_;
  size_t bytes_read = wabt::ReadS64Leb128(p, end, out_value);
  ERROR_UNLESS(bytes_read > 0, "unable to read i64 leb128: %s", desc);
  state_.offset += bytes_read;
  return Result::Ok;
}

Result BinaryReader::ReadType(Type* out_value, const char* desc) {
  uint32_t type = 0;
  CHECK_RESULT(ReadS32Leb128(&type, desc));
  *out_value = static_cast<Type>(type);
  return Result::Ok;
}

Result BinaryReader::ReadStr(string_view* out_str, const char* desc) {
  uint32_t str_len = 0;
  CHECK_RESULT(ReadU32Leb128(&str_len, "string length"));

  ERROR_UNLESS(state_.offset + str_len <= read_end_,
               "unable to read string: %s", desc);

  *out_str = string_view(
      reinterpret_cast<const char*>(state_.data) + state_.offset, str_len);
  state_.offset += str_len;

  ERROR_UNLESS(IsValidUtf8(out_str->data(), out_str->length()),
               "invalid utf-8 encoding: %s", desc);
  return Result::Ok;
}

Result BinaryReader::ReadBytes(const void** out_data,
                               Address* out_data_size,
                               const char* desc) {
  uint32_t data_size = 0;
  CHECK_RESULT(ReadU32Leb128(&data_size, "data size"));

  ERROR_UNLESS(state_.offset + data_size <= read_end_,
               "unable to read data: %s", desc);

  *out_data = static_cast<const uint8_t*>(state_.data) + state_.offset;
  *out_data_size = data_size;
  state_.offset += data_size;
  return Result::Ok;
}

Result BinaryReader::ReadIndex(Index* index, const char* desc) {
  uint32_t value;
  CHECK_RESULT(ReadU32Leb128(&value, desc));
  *index = value;
  return Result::Ok;
}

Result BinaryReader::ReadOffset(Offset* offset, const char* desc) {
  uint32_t value;
  CHECK_RESULT(ReadU32Leb128(&value, desc));
  *offset = value;
  return Result::Ok;
}

Result BinaryReader::ReadCount(Index* count, const char* desc) {
  CHECK_RESULT(ReadIndex(count, desc));

  // This check assumes that each item follows in this section, and takes at
  // least 1 byte. It's possible that this check passes but reading fails
  // later. It is still useful to check here, though, because it early-outs
  // when an erroneous large count is used, before allocating memory for it.
  size_t section_remaining = read_end_ - state_.offset;
  if (*count > section_remaining) {
    PrintError("invalid %s %" PRIindex ", only %" PRIzd
               " bytes left in section",
               desc, *count, section_remaining);
    return Result::Error;
  }
  return Result::Ok;
}

static bool is_valid_external_kind(uint8_t kind) {
  return kind < kExternalKindCount;
}

bool BinaryReader::IsConcreteType(Type type) {
  switch (type) {
    case Type::I32:
    case Type::I64:
    case Type::F32:
    case Type::F64:
      return true;

    case Type::V128:
      return options_->features.simd_enabled();

    default:
      return false;
  }
}

bool BinaryReader::IsBlockType(Type type) {
  if (IsConcreteType(type) || type == Type::Void) {
    return true;
  }

  if (!(options_->features.multi_value_enabled() && IsTypeIndex(type))) {
    return false;
  }

  return GetTypeIndex(type) < num_signatures_;
}

Index BinaryReader::NumTotalFuncs() {
  return num_func_imports_ + num_function_signatures_;
}

Index BinaryReader::NumTotalTables() {
  return num_table_imports_ + num_tables_;
}

Index BinaryReader::NumTotalMemories() {
  return num_memory_imports_ + num_memories_;
}

Index BinaryReader::NumTotalGlobals() {
  return num_global_imports_ + num_globals_;
}

Result BinaryReader::ReadI32InitExpr(Index index) {
  return ReadInitExpr(index, true);
}

Result BinaryReader::ReadInitExpr(Index index, bool require_i32) {
  Opcode opcode;
  CHECK_RESULT(ReadOpcode(&opcode, "opcode"));

  switch (opcode) {
    case Opcode::I32Const: {
      uint32_t value = 0;
      CHECK_RESULT(ReadS32Leb128(&value, "init_expr i32.const value"));
      CALLBACK(OnInitExprI32ConstExpr, index, value);
      break;
    }

    case Opcode::I64Const: {
      uint64_t value = 0;
      CHECK_RESULT(ReadS64Leb128(&value, "init_expr i64.const value"));
      CALLBACK(OnInitExprI64ConstExpr, index, value);
      break;
    }

    case Opcode::F32Const: {
      uint32_t value_bits = 0;
      CHECK_RESULT(ReadF32(&value_bits, "init_expr f32.const value"));
      CALLBACK(OnInitExprF32ConstExpr, index, value_bits);
      break;
    }

    case Opcode::F64Const: {
      uint64_t value_bits = 0;
      CHECK_RESULT(ReadF64(&value_bits, "init_expr f64.const value"));
      CALLBACK(OnInitExprF64ConstExpr, index, value_bits);
      break;
    }

    case Opcode::V128Const: {
      ERROR_UNLESS_OPCODE_ENABLED(opcode);
      v128 value_bits;
      ZeroMemory(value_bits);
      CHECK_RESULT(ReadV128(&value_bits, "init_expr v128.const value"));
      CALLBACK(OnInitExprV128ConstExpr, index, value_bits);
      break;
    }

    case Opcode::GetGlobal: {
      Index global_index;
      CHECK_RESULT(ReadIndex(&global_index, "init_expr get_global index"));
      CALLBACK(OnInitExprGetGlobalExpr, index, global_index);
      break;
    }

    case Opcode::End:
      return Result::Ok;

    default:
      return ReportUnexpectedOpcode(opcode, "in initializer expression");
  }

  if (require_i32 && opcode != Opcode::I32Const &&
      opcode != Opcode::GetGlobal) {
    PrintError("expected i32 init_expr");
    return Result::Error;
  }

  CHECK_RESULT(ReadOpcode(&opcode, "opcode"));
  ERROR_UNLESS(opcode == Opcode::End,
               "expected END opcode after initializer expression");
  return Result::Ok;
}

Result BinaryReader::ReadTable(Type* out_elem_type, Limits* out_elem_limits) {
  CHECK_RESULT(ReadType(out_elem_type, "table elem type"));
  ERROR_UNLESS(*out_elem_type == Type::Anyfunc,
               "table elem type must by anyfunc");

  uint32_t flags;
  uint32_t initial;
  uint32_t max = 0;
  CHECK_RESULT(ReadU32Leb128(&flags, "table flags"));
  CHECK_RESULT(ReadU32Leb128(&initial, "table initial elem count"));
  bool has_max = flags & WABT_BINARY_LIMITS_HAS_MAX_FLAG;
  bool is_shared = flags & WABT_BINARY_LIMITS_IS_SHARED_FLAG;
  ERROR_UNLESS(!is_shared, "tables may not be shared");
  if (has_max) {
    CHECK_RESULT(ReadU32Leb128(&max, "table max elem count"));
    ERROR_UNLESS(initial <= max,
                 "table initial elem count must be <= max elem count");
  }

  out_elem_limits->has_max = has_max;
  out_elem_limits->initial = initial;
  out_elem_limits->max = max;
  return Result::Ok;
}

Result BinaryReader::ReadMemory(Limits* out_page_limits) {
  uint32_t flags;
  uint32_t initial;
  uint32_t max = 0;
  CHECK_RESULT(ReadU32Leb128(&flags, "memory flags"));
  CHECK_RESULT(ReadU32Leb128(&initial, "memory initial page count"));
  ERROR_UNLESS(initial <= WABT_MAX_PAGES, "invalid memory initial size");
  bool has_max = flags & WABT_BINARY_LIMITS_HAS_MAX_FLAG;
  bool is_shared = flags & WABT_BINARY_LIMITS_IS_SHARED_FLAG;
  ERROR_UNLESS(!is_shared || has_max, "shared memory must have a max size");
  if (has_max) {
    CHECK_RESULT(ReadU32Leb128(&max, "memory max page count"));
    ERROR_UNLESS(max <= WABT_MAX_PAGES, "invalid memory max size");
    ERROR_UNLESS(initial <= max, "memory initial size must be <= max size");
  }

  out_page_limits->has_max = has_max;
  out_page_limits->is_shared = is_shared;
  out_page_limits->initial = initial;
  out_page_limits->max = max;
  return Result::Ok;
}

Result BinaryReader::ReadGlobalHeader(Type* out_type, bool* out_mutable) {
  Type global_type = Type::Void;
  uint8_t mutable_ = 0;
  CHECK_RESULT(ReadType(&global_type, "global type"));
  ERROR_UNLESS(IsConcreteType(global_type), "invalid global type: %#x",
               static_cast<int>(global_type));

  CHECK_RESULT(ReadU8(&mutable_, "global mutability"));
  ERROR_UNLESS(mutable_ <= 1, "global mutability must be 0 or 1");

  *out_type = global_type;
  *out_mutable = mutable_;
  return Result::Ok;
}

Result BinaryReader::ReadFunctionBody(Offset end_offset) {
  bool seen_end_opcode = false;
  while (state_.offset < end_offset) {
    Opcode opcode;
    CHECK_RESULT(ReadOpcode(&opcode, "opcode"));
    CALLBACK(OnOpcode, opcode);
    switch (opcode) {
      case Opcode::Unreachable:
        CALLBACK0(OnUnreachableExpr);
        CALLBACK0(OnOpcodeBare);
        break;

      case Opcode::Block: {
        Type sig_type;
        CHECK_RESULT(ReadType(&sig_type, "block signature type"));
        ERROR_UNLESS(IsBlockType(sig_type),
                     "expected valid block signature type");
        CALLBACK(OnBlockExpr, sig_type);
        CALLBACK(OnOpcodeBlockSig, sig_type);
        break;
      }

      case Opcode::Loop: {
        Type sig_type;
        CHECK_RESULT(ReadType(&sig_type, "loop signature type"));
        ERROR_UNLESS(IsBlockType(sig_type),
                     "expected valid block signature type");
        CALLBACK(OnLoopExpr, sig_type);
        CALLBACK(OnOpcodeBlockSig, sig_type);
        break;
      }

      case Opcode::If: {
        Type sig_type;
        CHECK_RESULT(ReadType(&sig_type, "if signature type"));
        ERROR_UNLESS(IsBlockType(sig_type),
                     "expected valid block signature type");
        CALLBACK(OnIfExpr, sig_type);
        CALLBACK(OnOpcodeBlockSig, sig_type);
        break;
      }

      case Opcode::Else:
        CALLBACK0(OnElseExpr);
        CALLBACK0(OnOpcodeBare);
        break;

      case Opcode::Select:
        CALLBACK0(OnSelectExpr);
        CALLBACK0(OnOpcodeBare);
        break;

      case Opcode::Br: {
        Index depth;
        CHECK_RESULT(ReadIndex(&depth, "br depth"));
        CALLBACK(OnBrExpr, depth);
        CALLBACK(OnOpcodeIndex, depth);
        break;
      }

      case Opcode::BrIf: {
        Index depth;
        CHECK_RESULT(ReadIndex(&depth, "br_if depth"));
        CALLBACK(OnBrIfExpr, depth);
        CALLBACK(OnOpcodeIndex, depth);
        break;
      }

      case Opcode::BrTable: {
        Index num_targets;
        CHECK_RESULT(ReadIndex(&num_targets, "br_table target count"));
        target_depths_.resize(num_targets);

        for (Index i = 0; i < num_targets; ++i) {
          Index target_depth;
          CHECK_RESULT(ReadIndex(&target_depth, "br_table target depth"));
          target_depths_[i] = target_depth;
        }

        Index default_target_depth;
        CHECK_RESULT(
            ReadIndex(&default_target_depth, "br_table default target depth"));

        Index* target_depths = num_targets ? target_depths_.data() : nullptr;

        CALLBACK(OnBrTableExpr, num_targets, target_depths,
                 default_target_depth);
        break;
      }

      case Opcode::Return:
        CALLBACK0(OnReturnExpr);
        CALLBACK0(OnOpcodeBare);
        break;

      case Opcode::Nop:
        CALLBACK0(OnNopExpr);
        CALLBACK0(OnOpcodeBare);
        break;

      case Opcode::Drop:
        CALLBACK0(OnDropExpr);
        CALLBACK0(OnOpcodeBare);
        break;

      case Opcode::End:
        if (state_.offset == end_offset) {
          seen_end_opcode = true;
          CALLBACK0(OnEndFunc);
        } else {
          CALLBACK0(OnEndExpr);
        }
        break;

      case Opcode::I32Const: {
        uint32_t value;
        CHECK_RESULT(ReadS32Leb128(&value, "i32.const value"));
        CALLBACK(OnI32ConstExpr, value);
        CALLBACK(OnOpcodeUint32, value);
        break;
      }

      case Opcode::I64Const: {
        uint64_t value;
        CHECK_RESULT(ReadS64Leb128(&value, "i64.const value"));
        CALLBACK(OnI64ConstExpr, value);
        CALLBACK(OnOpcodeUint64, value);
        break;
      }

      case Opcode::F32Const: {
        uint32_t value_bits = 0;
        CHECK_RESULT(ReadF32(&value_bits, "f32.const value"));
        CALLBACK(OnF32ConstExpr, value_bits);
        CALLBACK(OnOpcodeF32, value_bits);
        break;
      }

      case Opcode::F64Const: {
        uint64_t value_bits = 0;
        CHECK_RESULT(ReadF64(&value_bits, "f64.const value"));
        CALLBACK(OnF64ConstExpr, value_bits);
        CALLBACK(OnOpcodeF64, value_bits);
        break;
      }

      case Opcode::V128Const: {
        ERROR_UNLESS_OPCODE_ENABLED(opcode);
        v128 value_bits;
        ZeroMemory(value_bits);
        CHECK_RESULT(ReadV128(&value_bits, "v128.const value"));
        CALLBACK(OnV128ConstExpr, value_bits);
        CALLBACK(OnOpcodeV128, value_bits);
        break;
      }

      case Opcode::GetGlobal: {
        Index global_index;
        CHECK_RESULT(ReadIndex(&global_index, "get_global global index"));
        CALLBACK(OnGetGlobalExpr, global_index);
        CALLBACK(OnOpcodeIndex, global_index);
        break;
      }

      case Opcode::GetLocal: {
        Index local_index;
        CHECK_RESULT(ReadIndex(&local_index, "get_local local index"));
        CALLBACK(OnGetLocalExpr, local_index);
        CALLBACK(OnOpcodeIndex, local_index);
        break;
      }

      case Opcode::SetGlobal: {
        Index global_index;
        CHECK_RESULT(ReadIndex(&global_index, "set_global global index"));
        CALLBACK(OnSetGlobalExpr, global_index);
        CALLBACK(OnOpcodeIndex, global_index);
        break;
      }

      case Opcode::SetLocal: {
        Index local_index;
        CHECK_RESULT(ReadIndex(&local_index, "set_local local index"));
        CALLBACK(OnSetLocalExpr, local_index);
        CALLBACK(OnOpcodeIndex, local_index);
        break;
      }

      case Opcode::Call: {
        Index func_index;
        CHECK_RESULT(ReadIndex(&func_index, "call function index"));
        ERROR_UNLESS(func_index < NumTotalFuncs(),
                     "invalid call function index: %" PRIindex, func_index);
        CALLBACK(OnCallExpr, func_index);
        CALLBACK(OnOpcodeIndex, func_index);
        break;
      }

      case Opcode::CallIndirect: {
        Index sig_index;
        CHECK_RESULT(ReadIndex(&sig_index, "call_indirect signature index"));
        ERROR_UNLESS(sig_index < num_signatures_,
                     "invalid call_indirect signature index");
        uint32_t reserved;
        CHECK_RESULT(ReadU32Leb128(&reserved, "call_indirect reserved"));
        ERROR_UNLESS(reserved == 0, "call_indirect reserved value must be 0");
        CALLBACK(OnCallIndirectExpr, sig_index);
        CALLBACK(OnOpcodeUint32Uint32, sig_index, reserved);
        break;
      }

      case Opcode::TeeLocal: {
        Index local_index;
        CHECK_RESULT(ReadIndex(&local_index, "tee_local local index"));
        CALLBACK(OnTeeLocalExpr, local_index);
        CALLBACK(OnOpcodeIndex, local_index);
        break;
      }

      case Opcode::I32Load8S:
      case Opcode::I32Load8U:
      case Opcode::I32Load16S:
      case Opcode::I32Load16U:
      case Opcode::I64Load8S:
      case Opcode::I64Load8U:
      case Opcode::I64Load16S:
      case Opcode::I64Load16U:
      case Opcode::I64Load32S:
      case Opcode::I64Load32U:
      case Opcode::I32Load:
      case Opcode::I64Load:
      case Opcode::F32Load:
      case Opcode::F64Load:
      case Opcode::V128Load: {
        uint32_t alignment_log2;
        CHECK_RESULT(ReadU32Leb128(&alignment_log2, "load alignment"));
        Address offset;
        CHECK_RESULT(ReadU32Leb128(&offset, "load offset"));

        CALLBACK(OnLoadExpr, opcode, alignment_log2, offset);
        CALLBACK(OnOpcodeUint32Uint32, alignment_log2, offset);
        break;
      }

      case Opcode::I32Store8:
      case Opcode::I32Store16:
      case Opcode::I64Store8:
      case Opcode::I64Store16:
      case Opcode::I64Store32:
      case Opcode::I32Store:
      case Opcode::I64Store:
      case Opcode::F32Store:
      case Opcode::F64Store:
      case Opcode::V128Store: {
        uint32_t alignment_log2;
        CHECK_RESULT(ReadU32Leb128(&alignment_log2, "store alignment"));
        Address offset;
        CHECK_RESULT(ReadU32Leb128(&offset, "store offset"));

        CALLBACK(OnStoreExpr, opcode, alignment_log2, offset);
        CALLBACK(OnOpcodeUint32Uint32, alignment_log2, offset);
        break;
      }

      case Opcode::MemorySize: {
        uint32_t reserved;
        CHECK_RESULT(ReadU32Leb128(&reserved, "memory.size reserved"));
        ERROR_UNLESS(reserved == 0, "memory.size reserved value must be 0");
        CALLBACK0(OnMemorySizeExpr);
        CALLBACK(OnOpcodeUint32, reserved);
        break;
      }

      case Opcode::MemoryGrow: {
        uint32_t reserved;
        CHECK_RESULT(ReadU32Leb128(&reserved, "memory.grow reserved"));
        ERROR_UNLESS(reserved == 0, "memory.grow reserved value must be 0");
        CALLBACK0(OnMemoryGrowExpr);
        CALLBACK(OnOpcodeUint32, reserved);
        break;
      }

      case Opcode::I32Add:
      case Opcode::I32Sub:
      case Opcode::I32Mul:
      case Opcode::I32DivS:
      case Opcode::I32DivU:
      case Opcode::I32RemS:
      case Opcode::I32RemU:
      case Opcode::I32And:
      case Opcode::I32Or:
      case Opcode::I32Xor:
      case Opcode::I32Shl:
      case Opcode::I32ShrU:
      case Opcode::I32ShrS:
      case Opcode::I32Rotr:
      case Opcode::I32Rotl:
      case Opcode::I64Add:
      case Opcode::I64Sub:
      case Opcode::I64Mul:
      case Opcode::I64DivS:
      case Opcode::I64DivU:
      case Opcode::I64RemS:
      case Opcode::I64RemU:
      case Opcode::I64And:
      case Opcode::I64Or:
      case Opcode::I64Xor:
      case Opcode::I64Shl:
      case Opcode::I64ShrU:
      case Opcode::I64ShrS:
      case Opcode::I64Rotr:
      case Opcode::I64Rotl:
      case Opcode::F32Add:
      case Opcode::F32Sub:
      case Opcode::F32Mul:
      case Opcode::F32Div:
      case Opcode::F32Min:
      case Opcode::F32Max:
      case Opcode::F32Copysign:
      case Opcode::F64Add:
      case Opcode::F64Sub:
      case Opcode::F64Mul:
      case Opcode::F64Div:
      case Opcode::F64Min:
      case Opcode::F64Max:
      case Opcode::F64Copysign:
      case Opcode::I8X16Add:
      case Opcode::I16X8Add:
      case Opcode::I32X4Add:
      case Opcode::I64X2Add:
      case Opcode::I8X16Sub:
      case Opcode::I16X8Sub:
      case Opcode::I32X4Sub:
      case Opcode::I64X2Sub:
      case Opcode::I8X16Mul:
      case Opcode::I16X8Mul:
      case Opcode::I32X4Mul:
      case Opcode::I8X16AddSaturateS:
      case Opcode::I8X16AddSaturateU:
      case Opcode::I16X8AddSaturateS:
      case Opcode::I16X8AddSaturateU:
      case Opcode::I8X16SubSaturateS:
      case Opcode::I8X16SubSaturateU:
      case Opcode::I16X8SubSaturateS:
      case Opcode::I16X8SubSaturateU:
      case Opcode::I8X16Shl:
      case Opcode::I16X8Shl:
      case Opcode::I32X4Shl:
      case Opcode::I64X2Shl:
      case Opcode::I8X16ShrS:
      case Opcode::I8X16ShrU:
      case Opcode::I16X8ShrS:
      case Opcode::I16X8ShrU:
      case Opcode::I32X4ShrS:
      case Opcode::I32X4ShrU:
      case Opcode::I64X2ShrS:
      case Opcode::I64X2ShrU:
      case Opcode::V128And:
      case Opcode::V128Or:
      case Opcode::V128Xor:
      case Opcode::F32X4Min:
      case Opcode::F64X2Min:
      case Opcode::F32X4Max:
      case Opcode::F64X2Max:
      case Opcode::F32X4Add:
      case Opcode::F64X2Add:
      case Opcode::F32X4Sub:
      case Opcode::F64X2Sub:
      case Opcode::F32X4Div:
      case Opcode::F64X2Div:
      case Opcode::F32X4Mul:
      case Opcode::F64X2Mul:
        ERROR_UNLESS_OPCODE_ENABLED(opcode);
        CALLBACK(OnBinaryExpr, opcode);
        CALLBACK0(OnOpcodeBare);
        break;

      case Opcode::I32Eq:
      case Opcode::I32Ne:
      case Opcode::I32LtS:
      case Opcode::I32LeS:
      case Opcode::I32LtU:
      case Opcode::I32LeU:
      case Opcode::I32GtS:
      case Opcode::I32GeS:
      case Opcode::I32GtU:
      case Opcode::I32GeU:
      case Opcode::I64Eq:
      case Opcode::I64Ne:
      case Opcode::I64LtS:
      case Opcode::I64LeS:
      case Opcode::I64LtU:
      case Opcode::I64LeU:
      case Opcode::I64GtS:
      case Opcode::I64GeS:
      case Opcode::I64GtU:
      case Opcode::I64GeU:
      case Opcode::F32Eq:
      case Opcode::F32Ne:
      case Opcode::F32Lt:
      case Opcode::F32Le:
      case Opcode::F32Gt:
      case Opcode::F32Ge:
      case Opcode::F64Eq:
      case Opcode::F64Ne:
      case Opcode::F64Lt:
      case Opcode::F64Le:
      case Opcode::F64Gt:
      case Opcode::F64Ge:
      case Opcode::I8X16Eq:
      case Opcode::I16X8Eq:
      case Opcode::I32X4Eq:
      case Opcode::F32X4Eq:
      case Opcode::F64X2Eq:
      case Opcode::I8X16Ne:
      case Opcode::I16X8Ne:
      case Opcode::I32X4Ne:
      case Opcode::F32X4Ne:
      case Opcode::F64X2Ne:
      case Opcode::I8X16LtS:
      case Opcode::I8X16LtU:
      case Opcode::I16X8LtS:
      case Opcode::I16X8LtU:
      case Opcode::I32X4LtS:
      case Opcode::I32X4LtU:
      case Opcode::F32X4Lt:
      case Opcode::F64X2Lt:
      case Opcode::I8X16LeS:
      case Opcode::I8X16LeU:
      case Opcode::I16X8LeS:
      case Opcode::I16X8LeU:
      case Opcode::I32X4LeS:
      case Opcode::I32X4LeU:
      case Opcode::F32X4Le:
      case Opcode::F64X2Le:
      case Opcode::I8X16GtS:
      case Opcode::I8X16GtU:
      case Opcode::I16X8GtS:
      case Opcode::I16X8GtU:
      case Opcode::I32X4GtS:
      case Opcode::I32X4GtU:
      case Opcode::F32X4Gt:
      case Opcode::F64X2Gt:
      case Opcode::I8X16GeS:
      case Opcode::I8X16GeU:
      case Opcode::I16X8GeS:
      case Opcode::I16X8GeU:
      case Opcode::I32X4GeS:
      case Opcode::I32X4GeU:
      case Opcode::F32X4Ge:
      case Opcode::F64X2Ge:
        ERROR_UNLESS_OPCODE_ENABLED(opcode);
        CALLBACK(OnCompareExpr, opcode);
        CALLBACK0(OnOpcodeBare);
        break;

      case Opcode::I32Clz:
      case Opcode::I32Ctz:
      case Opcode::I32Popcnt:
      case Opcode::I64Clz:
      case Opcode::I64Ctz:
      case Opcode::I64Popcnt:
      case Opcode::F32Abs:
      case Opcode::F32Neg:
      case Opcode::F32Ceil:
      case Opcode::F32Floor:
      case Opcode::F32Trunc:
      case Opcode::F32Nearest:
      case Opcode::F32Sqrt:
      case Opcode::F64Abs:
      case Opcode::F64Neg:
      case Opcode::F64Ceil:
      case Opcode::F64Floor:
      case Opcode::F64Trunc:
      case Opcode::F64Nearest:
      case Opcode::F64Sqrt:
      case Opcode::I8X16Splat:
      case Opcode::I16X8Splat:
      case Opcode::I32X4Splat:
      case Opcode::I64X2Splat:
      case Opcode::F32X4Splat:
      case Opcode::F64X2Splat:
      case Opcode::I8X16Neg:
      case Opcode::I16X8Neg:
      case Opcode::I32X4Neg:
      case Opcode::I64X2Neg:
      case Opcode::V128Not:
      case Opcode::I8X16AnyTrue:
      case Opcode::I16X8AnyTrue:
      case Opcode::I32X4AnyTrue:
      case Opcode::I64X2AnyTrue:
      case Opcode::I8X16AllTrue:
      case Opcode::I16X8AllTrue:
      case Opcode::I32X4AllTrue:
      case Opcode::I64X2AllTrue:
      case Opcode::F32X4Neg:
      case Opcode::F64X2Neg:
      case Opcode::F32X4Abs:
      case Opcode::F64X2Abs:
      case Opcode::F32X4Sqrt:
      case Opcode::F64X2Sqrt:
        ERROR_UNLESS_OPCODE_ENABLED(opcode);
        CALLBACK(OnUnaryExpr, opcode);
        CALLBACK0(OnOpcodeBare);
        break;

      case Opcode::V128BitSelect:
        ERROR_UNLESS_OPCODE_ENABLED(opcode);
        CALLBACK(OnTernaryExpr, opcode);
        CALLBACK0(OnOpcodeBare);
        break;

      case Opcode::I8X16ExtractLaneS:
      case Opcode::I8X16ExtractLaneU:
      case Opcode::I16X8ExtractLaneS:
      case Opcode::I16X8ExtractLaneU:
      case Opcode::I32X4ExtractLane:
      case Opcode::I64X2ExtractLane:
      case Opcode::F32X4ExtractLane:
      case Opcode::F64X2ExtractLane:
      case Opcode::I8X16ReplaceLane:
      case Opcode::I16X8ReplaceLane:
      case Opcode::I32X4ReplaceLane:
      case Opcode::I64X2ReplaceLane:
      case Opcode::F32X4ReplaceLane:
      case Opcode::F64X2ReplaceLane: {
        ERROR_UNLESS_OPCODE_ENABLED(opcode);
        uint8_t lane_val;
        CHECK_RESULT(ReadU8(&lane_val, "Lane idx"));
        CALLBACK(OnSimdLaneOpExpr, opcode, lane_val);
        CALLBACK(OnOpcodeUint64, lane_val);
        break;
      }

      case Opcode::V8X16Shuffle: {
        ERROR_UNLESS_OPCODE_ENABLED(opcode);
        v128 value;
        CHECK_RESULT(ReadV128(&value, "Lane idx [16]"));
        CALLBACK(OnSimdShuffleOpExpr, opcode, value);
        CALLBACK(OnOpcodeV128, value);
        break;
      }

      case Opcode::I32TruncSF32:
      case Opcode::I32TruncSF64:
      case Opcode::I32TruncUF32:
      case Opcode::I32TruncUF64:
      case Opcode::I32WrapI64:
      case Opcode::I64TruncSF32:
      case Opcode::I64TruncSF64:
      case Opcode::I64TruncUF32:
      case Opcode::I64TruncUF64:
      case Opcode::I64ExtendSI32:
      case Opcode::I64ExtendUI32:
      case Opcode::F32ConvertSI32:
      case Opcode::F32ConvertUI32:
      case Opcode::F32ConvertSI64:
      case Opcode::F32ConvertUI64:
      case Opcode::F32DemoteF64:
      case Opcode::F32ReinterpretI32:
      case Opcode::F64ConvertSI32:
      case Opcode::F64ConvertUI32:
      case Opcode::F64ConvertSI64:
      case Opcode::F64ConvertUI64:
      case Opcode::F64PromoteF32:
      case Opcode::F64ReinterpretI64:
      case Opcode::I32ReinterpretF32:
      case Opcode::I64ReinterpretF64:
      case Opcode::I32Eqz:
      case Opcode::I64Eqz:
      case Opcode::F32X4ConvertSI32X4:
      case Opcode::F32X4ConvertUI32X4:
      case Opcode::F64X2ConvertSI64X2:
      case Opcode::F64X2ConvertUI64X2:
      case Opcode::I32X4TruncSF32X4Sat:
      case Opcode::I32X4TruncUF32X4Sat:
      case Opcode::I64X2TruncSF64X2Sat:
      case Opcode::I64X2TruncUF64X2Sat:
        ERROR_UNLESS_OPCODE_ENABLED(opcode);
        CALLBACK(OnConvertExpr, opcode);
        CALLBACK0(OnOpcodeBare);
        break;

      case Opcode::Try: {
        ERROR_UNLESS_OPCODE_ENABLED(opcode);
        Type sig_type;
        CHECK_RESULT(ReadType(&sig_type, "try signature type"));
        ERROR_UNLESS(IsBlockType(sig_type),
                     "expected valid block signature type");
        CALLBACK(OnTryExpr, sig_type);
        CALLBACK(OnOpcodeBlockSig, sig_type);
        break;
      }

      case Opcode::Catch: {
        ERROR_UNLESS_OPCODE_ENABLED(opcode);
        CALLBACK0(OnCatchExpr);
        CALLBACK0(OnOpcodeBare);
        break;
      }

      case Opcode::Rethrow: {
        ERROR_UNLESS_OPCODE_ENABLED(opcode);
        CALLBACK0(OnRethrowExpr);
        CALLBACK0(OnOpcodeBare);
        break;
      }

      case Opcode::Throw: {
        ERROR_UNLESS_OPCODE_ENABLED(opcode);
        Index index;
        CHECK_RESULT(ReadIndex(&index, "exception index"));
        CALLBACK(OnThrowExpr, index);
        CALLBACK(OnOpcodeIndex, index);
        break;
      }

      case Opcode::IfExcept: {
        ERROR_UNLESS_OPCODE_ENABLED(opcode);
        Type sig_type;
        CHECK_RESULT(ReadType(&sig_type, "if signature type"));
        ERROR_UNLESS(IsBlockType(sig_type),
                     "expected valid block signature type");
        Index except_index;
        CHECK_RESULT(ReadIndex(&except_index, "exception index"));
        CALLBACK(OnIfExceptExpr, sig_type, except_index);
        break;
      }

      case Opcode::I32Extend8S:
      case Opcode::I32Extend16S:
      case Opcode::I64Extend8S:
      case Opcode::I64Extend16S:
      case Opcode::I64Extend32S:
        ERROR_UNLESS_OPCODE_ENABLED(opcode);
        CALLBACK(OnUnaryExpr, opcode);
        CALLBACK0(OnOpcodeBare);
        break;

      case Opcode::I32TruncSSatF32:
      case Opcode::I32TruncUSatF32:
      case Opcode::I32TruncSSatF64:
      case Opcode::I32TruncUSatF64:
      case Opcode::I64TruncSSatF32:
      case Opcode::I64TruncUSatF32:
      case Opcode::I64TruncSSatF64:
      case Opcode::I64TruncUSatF64:
        ERROR_UNLESS_OPCODE_ENABLED(opcode);
        CALLBACK(OnConvertExpr, opcode);
        CALLBACK0(OnOpcodeBare);
        break;

      case Opcode::AtomicWake: {
        uint32_t alignment_log2;
        CHECK_RESULT(ReadU32Leb128(&alignment_log2, "load alignment"));
        Address offset;
        CHECK_RESULT(ReadU32Leb128(&offset, "load offset"));

        CALLBACK(OnAtomicWakeExpr, opcode, alignment_log2, offset);
        CALLBACK(OnOpcodeUint32Uint32, alignment_log2, offset);
        break;
      }

      case Opcode::I32AtomicWait:
      case Opcode::I64AtomicWait: {
        uint32_t alignment_log2;
        CHECK_RESULT(ReadU32Leb128(&alignment_log2, "load alignment"));
        Address offset;
        CHECK_RESULT(ReadU32Leb128(&offset, "load offset"));

        CALLBACK(OnAtomicWaitExpr, opcode, alignment_log2, offset);
        CALLBACK(OnOpcodeUint32Uint32, alignment_log2, offset);
        break;
      }

      case Opcode::I32AtomicLoad8U:
      case Opcode::I32AtomicLoad16U:
      case Opcode::I64AtomicLoad8U:
      case Opcode::I64AtomicLoad16U:
      case Opcode::I64AtomicLoad32U:
      case Opcode::I32AtomicLoad:
      case Opcode::I64AtomicLoad: {
        uint32_t alignment_log2;
        CHECK_RESULT(ReadU32Leb128(&alignment_log2, "load alignment"));
        Address offset;
        CHECK_RESULT(ReadU32Leb128(&offset, "load offset"));

        CALLBACK(OnAtomicLoadExpr, opcode, alignment_log2, offset);
        CALLBACK(OnOpcodeUint32Uint32, alignment_log2, offset);
        break;
      }

      case Opcode::I32AtomicStore8:
      case Opcode::I32AtomicStore16:
      case Opcode::I64AtomicStore8:
      case Opcode::I64AtomicStore16:
      case Opcode::I64AtomicStore32:
      case Opcode::I32AtomicStore:
      case Opcode::I64AtomicStore: {
        uint32_t alignment_log2;
        CHECK_RESULT(ReadU32Leb128(&alignment_log2, "store alignment"));
        Address offset;
        CHECK_RESULT(ReadU32Leb128(&offset, "store offset"));

        CALLBACK(OnAtomicStoreExpr, opcode, alignment_log2, offset);
        CALLBACK(OnOpcodeUint32Uint32, alignment_log2, offset);
        break;
      }

      case Opcode::I32AtomicRmwAdd:
      case Opcode::I64AtomicRmwAdd:
      case Opcode::I32AtomicRmw8UAdd:
      case Opcode::I32AtomicRmw16UAdd:
      case Opcode::I64AtomicRmw8UAdd:
      case Opcode::I64AtomicRmw16UAdd:
      case Opcode::I64AtomicRmw32UAdd:
      case Opcode::I32AtomicRmwSub:
      case Opcode::I64AtomicRmwSub:
      case Opcode::I32AtomicRmw8USub:
      case Opcode::I32AtomicRmw16USub:
      case Opcode::I64AtomicRmw8USub:
      case Opcode::I64AtomicRmw16USub:
      case Opcode::I64AtomicRmw32USub:
      case Opcode::I32AtomicRmwAnd:
      case Opcode::I64AtomicRmwAnd:
      case Opcode::I32AtomicRmw8UAnd:
      case Opcode::I32AtomicRmw16UAnd:
      case Opcode::I64AtomicRmw8UAnd:
      case Opcode::I64AtomicRmw16UAnd:
      case Opcode::I64AtomicRmw32UAnd:
      case Opcode::I32AtomicRmwOr:
      case Opcode::I64AtomicRmwOr:
      case Opcode::I32AtomicRmw8UOr:
      case Opcode::I32AtomicRmw16UOr:
      case Opcode::I64AtomicRmw8UOr:
      case Opcode::I64AtomicRmw16UOr:
      case Opcode::I64AtomicRmw32UOr:
      case Opcode::I32AtomicRmwXor:
      case Opcode::I64AtomicRmwXor:
      case Opcode::I32AtomicRmw8UXor:
      case Opcode::I32AtomicRmw16UXor:
      case Opcode::I64AtomicRmw8UXor:
      case Opcode::I64AtomicRmw16UXor:
      case Opcode::I64AtomicRmw32UXor:
      case Opcode::I32AtomicRmwXchg:
      case Opcode::I64AtomicRmwXchg:
      case Opcode::I32AtomicRmw8UXchg:
      case Opcode::I32AtomicRmw16UXchg:
      case Opcode::I64AtomicRmw8UXchg:
      case Opcode::I64AtomicRmw16UXchg:
      case Opcode::I64AtomicRmw32UXchg: {
        uint32_t alignment_log2;
        CHECK_RESULT(ReadU32Leb128(&alignment_log2, "memory alignment"));
        Address offset;
        CHECK_RESULT(ReadU32Leb128(&offset, "memory offset"));

        CALLBACK(OnAtomicRmwExpr, opcode, alignment_log2, offset);
        CALLBACK(OnOpcodeUint32Uint32, alignment_log2, offset);
        break;
      }

      case Opcode::I32AtomicRmwCmpxchg:
      case Opcode::I64AtomicRmwCmpxchg:
      case Opcode::I32AtomicRmw8UCmpxchg:
      case Opcode::I32AtomicRmw16UCmpxchg:
      case Opcode::I64AtomicRmw8UCmpxchg:
      case Opcode::I64AtomicRmw16UCmpxchg:
      case Opcode::I64AtomicRmw32UCmpxchg: {
        uint32_t alignment_log2;
        CHECK_RESULT(ReadU32Leb128(&alignment_log2, "memory alignment"));
        Address offset;
        CHECK_RESULT(ReadU32Leb128(&offset, "memory offset"));

        CALLBACK(OnAtomicRmwCmpxchgExpr, opcode, alignment_log2, offset);
        CALLBACK(OnOpcodeUint32Uint32, alignment_log2, offset);
        break;
      }

      default:
        return ReportUnexpectedOpcode(opcode);
    }
  }
  ERROR_UNLESS(state_.offset == end_offset,
               "function body longer than given size");
  ERROR_UNLESS(seen_end_opcode, "function body must end with END opcode");
  return Result::Ok;
}

Result BinaryReader::ReadNameSection(Offset section_size) {
  CALLBACK(BeginNamesSection, section_size);
  Index i = 0;
  uint32_t previous_subsection_type = 0;
  while (state_.offset < read_end_) {
    uint32_t name_type;
    Offset subsection_size;
    CHECK_RESULT(ReadU32Leb128(&name_type, "name type"));
    if (i != 0) {
      ERROR_UNLESS(name_type != previous_subsection_type,
                   "duplicate sub-section");
      ERROR_UNLESS(name_type >= previous_subsection_type,
                   "out-of-order sub-section");
    }
    previous_subsection_type = name_type;
    CHECK_RESULT(ReadOffset(&subsection_size, "subsection size"));
    size_t subsection_end = state_.offset + subsection_size;
    ERROR_UNLESS(subsection_end <= read_end_,
                 "invalid sub-section size: extends past end");
    ReadEndRestoreGuard guard(this);
    read_end_ = subsection_end;

    switch (static_cast<NameSectionSubsection>(name_type)) {
      case NameSectionSubsection::Module:
        CALLBACK(OnModuleNameSubsection, i, name_type, subsection_size);
        if (subsection_size) {
          string_view name;
          CHECK_RESULT(ReadStr(&name, "module name"));
          CALLBACK(OnModuleName, name);
        }
        break;
      case NameSectionSubsection::Function:
        CALLBACK(OnFunctionNameSubsection, i, name_type, subsection_size);
        if (subsection_size) {
          Index num_names;
          CHECK_RESULT(ReadCount(&num_names, "name count"));
          CALLBACK(OnFunctionNamesCount, num_names);
          Index last_function_index = kInvalidIndex;

          for (Index j = 0; j < num_names; ++j) {
            Index function_index;
            string_view function_name;

            CHECK_RESULT(ReadIndex(&function_index, "function index"));
            ERROR_UNLESS(function_index != last_function_index,
                         "duplicate function name: %u", function_index);
            ERROR_UNLESS(last_function_index == kInvalidIndex ||
                             function_index > last_function_index,
                         "function index out of order: %u", function_index);
            last_function_index = function_index;
            ERROR_UNLESS(function_index < NumTotalFuncs(),
                         "invalid function index: %" PRIindex, function_index);
            CHECK_RESULT(ReadStr(&function_name, "function name"));
            CALLBACK(OnFunctionName, function_index, function_name);
          }
        }
        break;
      case NameSectionSubsection::Local:
        CALLBACK(OnLocalNameSubsection, i, name_type, subsection_size);
        if (subsection_size) {
          Index num_funcs;
          CHECK_RESULT(ReadCount(&num_funcs, "function count"));
          CALLBACK(OnLocalNameFunctionCount, num_funcs);
          Index last_function_index = kInvalidIndex;
          for (Index j = 0; j < num_funcs; ++j) {
            Index function_index;
            CHECK_RESULT(ReadIndex(&function_index, "function index"));
            ERROR_UNLESS(function_index < NumTotalFuncs(),
                         "invalid function index: %u", function_index);
            ERROR_UNLESS(last_function_index == kInvalidIndex ||
                             function_index > last_function_index,
                         "locals function index out of order: %u",
                         function_index);
            last_function_index = function_index;
            Index num_locals;
            CHECK_RESULT(ReadCount(&num_locals, "local count"));
            CALLBACK(OnLocalNameLocalCount, function_index, num_locals);
            Index last_local_index = kInvalidIndex;
            for (Index k = 0; k < num_locals; ++k) {
              Index local_index;
              string_view local_name;

              CHECK_RESULT(ReadIndex(&local_index, "named index"));
              ERROR_UNLESS(local_index != last_local_index,
                           "duplicate local index: %u", local_index);
              ERROR_UNLESS(last_local_index == kInvalidIndex ||
                               local_index > last_local_index,
                           "local index out of order: %u", local_index);
              last_local_index = local_index;
              CHECK_RESULT(ReadStr(&local_name, "name"));
              CALLBACK(OnLocalName, function_index, local_index, local_name);
            }
          }
        }
        break;
      default:
        // Unknown subsection, skip it.
        state_.offset = subsection_end;
        break;
    }
    ++i;
    ERROR_UNLESS(state_.offset == subsection_end,
                 "unfinished sub-section (expected end: 0x%" PRIzx ")",
                 subsection_end);
  }
  CALLBACK0(EndNamesSection);
  return Result::Ok;
}

Result BinaryReader::ReadRelocSection(Offset section_size) {
  CALLBACK(BeginRelocSection, section_size);
  uint32_t section_index;
  CHECK_RESULT(ReadU32Leb128(&section_index, "section index"));
  Index num_relocs;
  CHECK_RESULT(ReadCount(&num_relocs, "relocation count"));
  CALLBACK(OnRelocCount, num_relocs, section_index);
  for (Index i = 0; i < num_relocs; ++i) {
    Offset offset;
    Index index;
    uint32_t reloc_type, addend = 0;
    CHECK_RESULT(ReadU32Leb128(&reloc_type, "relocation type"));
    CHECK_RESULT(ReadOffset(&offset, "offset"));
    CHECK_RESULT(ReadIndex(&index, "index"));
    RelocType type = static_cast<RelocType>(reloc_type);
    switch (type) {
      case RelocType::MemoryAddressLEB:
      case RelocType::MemoryAddressSLEB:
      case RelocType::MemoryAddressI32:
      case RelocType::FunctionOffsetI32:
      case RelocType::SectionOffsetI32:
        CHECK_RESULT(ReadS32Leb128(&addend, "addend"));
        break;
      default:
        break;
    }
    CALLBACK(OnReloc, type, offset, index, addend);
  }
  CALLBACK0(EndRelocSection);
  return Result::Ok;
}

Result BinaryReader::ReadLinkingSection(Offset section_size) {
  CALLBACK(BeginLinkingSection, section_size);
  uint32_t version;
  CHECK_RESULT(ReadU32Leb128(&version, "version"));
  ERROR_UNLESS(version == 1, "invalid linking metadata version: %u", version);
  while (state_.offset < read_end_) {
    uint32_t linking_type;
    Offset subsection_size;
    CHECK_RESULT(ReadU32Leb128(&linking_type, "type"));
    CHECK_RESULT(ReadOffset(&subsection_size, "subsection size"));
    size_t subsection_end = state_.offset + subsection_size;
    ERROR_UNLESS(subsection_end <= read_end_,
                 "invalid sub-section size: extends past end");
    ReadEndRestoreGuard guard(this);
    read_end_ = subsection_end;

    uint32_t count;
    switch (static_cast<LinkingEntryType>(linking_type)) {
      case LinkingEntryType::SymbolTable:
        CHECK_RESULT(ReadU32Leb128(&count, "sym count"));
        CALLBACK(OnSymbolCount, count);
        for (Index i = 0; i < count; ++i) {
          string_view name;
          uint32_t flags = 0;
          uint32_t kind = 0;
          CHECK_RESULT(ReadU32Leb128(&kind, "sym type"));
          CHECK_RESULT(ReadU32Leb128(&flags, "sym flags"));
          SymbolType sym_type = static_cast<SymbolType>(kind);
          CALLBACK(OnSymbol, i, sym_type, flags);
          switch (sym_type) {
            case SymbolType::Function:
            case SymbolType::Global: {
              uint32_t index = 0;
              CHECK_RESULT(ReadU32Leb128(&index, "index"));
              if ((flags & WABT_SYMBOL_FLAG_UNDEFINED) == 0)
                CHECK_RESULT(ReadStr(&name, "symbol name"));
              if (sym_type == SymbolType::Function) {
                CALLBACK(OnFunctionSymbol, i, flags, name, index);
              } else {
                CALLBACK(OnGlobalSymbol, i, flags, name, index);
              }
              break;
            }
            case SymbolType::Data: {
              uint32_t segment = 0;
              uint32_t offset = 0;
              uint32_t size = 0;
              CHECK_RESULT(ReadStr(&name, "symbol name"));
              if ((flags & WABT_SYMBOL_FLAG_UNDEFINED) == 0) {
                CHECK_RESULT(ReadU32Leb128(&segment, "segment"));
                CHECK_RESULT(ReadU32Leb128(&offset, "offset"));
                CHECK_RESULT(ReadU32Leb128(&size, "size"));
              }
              CALLBACK(OnDataSymbol, i, flags, name, segment, offset, size);
              break;
            }
            case SymbolType::Section: {
              uint32_t index = 0;
              CHECK_RESULT(ReadU32Leb128(&index, "index"));
              CALLBACK(OnSectionSymbol, i, flags, index);
              break;
            }
          }
        }
        break;
      case LinkingEntryType::SegmentInfo:
        CHECK_RESULT(ReadU32Leb128(&count, "info count"));
        CALLBACK(OnSegmentInfoCount, count);
        for (Index i = 0; i < count; i++) {
          string_view name;
          uint32_t alignment;
          uint32_t flags;
          CHECK_RESULT(ReadStr(&name, "segment name"));
          CHECK_RESULT(ReadU32Leb128(&alignment, "segment alignment"));
          CHECK_RESULT(ReadU32Leb128(&flags, "segment flags"));
          CALLBACK(OnSegmentInfo, i, name, alignment, flags);
        }
        break;
      case LinkingEntryType::InitFunctions:
        CHECK_RESULT(ReadU32Leb128(&count, "info count"));
        CALLBACK(OnInitFunctionCount, count);
        while (count--) {
          uint32_t priority;
          uint32_t func;
          CHECK_RESULT(ReadU32Leb128(&priority, "priority"));
          CHECK_RESULT(ReadU32Leb128(&func, "function index"));
          CALLBACK(OnInitFunction, priority, func);
        }
        break;
      default:
        // Unknown subsection, skip it.
        state_.offset = subsection_end;
        break;
    }
    ERROR_UNLESS(state_.offset == subsection_end,
                 "unfinished sub-section (expected end: 0x%" PRIzx ")",
                 subsection_end);
  }
  CALLBACK0(EndLinkingSection);
  return Result::Ok;
}

Result BinaryReader::ReadExceptionType(TypeVector& sig) {
  Index num_values;
  CHECK_RESULT(ReadCount(&num_values, "exception type count"));
  sig.resize(num_values);
  for (Index j = 0; j < num_values; ++j) {
    Type value_type;
    CHECK_RESULT(ReadType(&value_type, "exception value type"));
    ERROR_UNLESS(IsConcreteType(value_type),
                 "excepted valid exception value type (got %d)",
                 static_cast<int>(value_type));
    sig[j] = value_type;
  }
  return Result::Ok;
}

Result BinaryReader::ReadExceptionSection(Offset section_size) {
  CALLBACK(BeginExceptionSection, section_size);
  CHECK_RESULT(ReadCount(&num_exceptions_, "exception count"));
  CALLBACK(OnExceptionCount, num_exceptions_);

  for (Index i = 0; i < num_exceptions_; ++i) {
    TypeVector sig;
    CHECK_RESULT(ReadExceptionType(sig));
    CALLBACK(OnExceptionType, i, sig);
  }

  CALLBACK(EndExceptionSection);
  return Result::Ok;
}

Result BinaryReader::ReadCustomSection(Offset section_size) {
  string_view section_name;
  CHECK_RESULT(ReadStr(&section_name, "section name"));
  CALLBACK(BeginCustomSection, section_size, section_name);
  ValueRestoreGuard<bool, &BinaryReader::reading_custom_section_> guard(this);
  reading_custom_section_ = true;

  if (options_->read_debug_names && section_name == WABT_BINARY_SECTION_NAME) {
    CHECK_RESULT(ReadNameSection(section_size));
    did_read_names_section_ = true;
  } else if (section_name.rfind(WABT_BINARY_SECTION_RELOC, 0) == 0) {
    // Reloc sections always begin with "reloc."
    CHECK_RESULT(ReadRelocSection(section_size));
  } else if (section_name == WABT_BINARY_SECTION_LINKING) {
    CHECK_RESULT(ReadLinkingSection(section_size));
  } else if (options_->features.exceptions_enabled() &&
             section_name == WABT_BINARY_SECTION_EXCEPTION) {
    CHECK_RESULT(ReadExceptionSection(section_size));
  } else {
    // This is an unknown custom section, skip it.
    state_.offset = read_end_;
  }
  CALLBACK0(EndCustomSection);
  return Result::Ok;
}

Result BinaryReader::ReadTypeSection(Offset section_size) {
  CALLBACK(BeginTypeSection, section_size);
  CHECK_RESULT(ReadCount(&num_signatures_, "type count"));
  CALLBACK(OnTypeCount, num_signatures_);

  for (Index i = 0; i < num_signatures_; ++i) {
    Type form;
    CHECK_RESULT(ReadType(&form, "type form"));
    ERROR_UNLESS(form == Type::Func,
                 "unexpected type form (got " PRItypecode ")",
                 WABT_PRINTF_TYPE_CODE(form));

    Index num_params;
    CHECK_RESULT(ReadCount(&num_params, "function param count"));

    param_types_.resize(num_params);

    for (Index j = 0; j < num_params; ++j) {
      Type param_type;
      CHECK_RESULT(ReadType(&param_type, "function param type"));
      ERROR_UNLESS(IsConcreteType(param_type),
                   "expected valid param type (got " PRItypecode ")",
                   WABT_PRINTF_TYPE_CODE(param_type));
      param_types_[j] = param_type;
    }

    Index num_results;
    CHECK_RESULT(ReadCount(&num_results, "function result count"));
    ERROR_UNLESS(num_results <= 1 || options_->features.multi_value_enabled(),
                 "result count must be 0 or 1");

    result_types_.resize(num_results);

    for (Index j = 0; j < num_results; ++j) {
      Type result_type;
      CHECK_RESULT(ReadType(&result_type, "function result type"));
      ERROR_UNLESS(IsConcreteType(result_type),
                   "expected valid result type (got " PRItypecode ")",
                   WABT_PRINTF_TYPE_CODE(result_type));
      result_types_[j] = result_type;
    }

    Type* param_types = num_params ? param_types_.data() : nullptr;
    Type* result_types = num_results ? result_types_.data() : nullptr;

    CALLBACK(OnType, i, num_params, param_types, num_results, result_types);
  }
  CALLBACK0(EndTypeSection);
  return Result::Ok;
}

Result BinaryReader::ReadImportSection(Offset section_size) {
  CALLBACK(BeginImportSection, section_size);
  CHECK_RESULT(ReadCount(&num_imports_, "import count"));
  CALLBACK(OnImportCount, num_imports_);
  for (Index i = 0; i < num_imports_; ++i) {
    string_view module_name;
    CHECK_RESULT(ReadStr(&module_name, "import module name"));
    string_view field_name;
    CHECK_RESULT(ReadStr(&field_name, "import field name"));

    uint8_t kind;
    CHECK_RESULT(ReadU8(&kind, "import kind"));
    switch (static_cast<ExternalKind>(kind)) {
      case ExternalKind::Func: {
        Index sig_index;
        CHECK_RESULT(ReadIndex(&sig_index, "import signature index"));
        ERROR_UNLESS(sig_index < num_signatures_,
                     "invalid import signature index");
        CALLBACK(OnImport, i, module_name, field_name);
        CALLBACK(OnImportFunc, i, module_name, field_name, num_func_imports_,
                 sig_index);
        num_func_imports_++;
        break;
      }

      case ExternalKind::Table: {
        Type elem_type;
        Limits elem_limits;
        CHECK_RESULT(ReadTable(&elem_type, &elem_limits));
        CALLBACK(OnImport, i, module_name, field_name);
        CALLBACK(OnImportTable, i, module_name, field_name, num_table_imports_,
                 elem_type, &elem_limits);
        num_table_imports_++;
        break;
      }

      case ExternalKind::Memory: {
        Limits page_limits;
        CHECK_RESULT(ReadMemory(&page_limits));
        CALLBACK(OnImport, i, module_name, field_name);
        CALLBACK(OnImportMemory, i, module_name, field_name,
                 num_memory_imports_, &page_limits);
        num_memory_imports_++;
        break;
      }

      case ExternalKind::Global: {
        Type type;
        bool mutable_;
        CHECK_RESULT(ReadGlobalHeader(&type, &mutable_));
        CALLBACK(OnImport, i, module_name, field_name);
        CALLBACK(OnImportGlobal, i, module_name, field_name,
                 num_global_imports_, type, mutable_);
        num_global_imports_++;
        break;
      }

      case ExternalKind::Except: {
        ERROR_UNLESS(options_->features.exceptions_enabled(),
                     "invalid import exception kind: exceptions not allowed");
        TypeVector sig;
        CHECK_RESULT(ReadExceptionType(sig));
        CALLBACK(OnImport, i, module_name, field_name);
        CALLBACK(OnImportException, i, module_name, field_name,
                 num_exception_imports_, sig);
        num_exception_imports_++;
        break;
      }
    }
  }
  CALLBACK0(EndImportSection);
  return Result::Ok;
}

Result BinaryReader::ReadFunctionSection(Offset section_size) {
  CALLBACK(BeginFunctionSection, section_size);
  CHECK_RESULT(
      ReadCount(&num_function_signatures_, "function signature count"));
  CALLBACK(OnFunctionCount, num_function_signatures_);
  for (Index i = 0; i < num_function_signatures_; ++i) {
    Index func_index = num_func_imports_ + i;
    Index sig_index;
    CHECK_RESULT(ReadIndex(&sig_index, "function signature index"));
    ERROR_UNLESS(sig_index < num_signatures_,
                 "invalid function signature index: %" PRIindex, sig_index);
    CALLBACK(OnFunction, func_index, sig_index);
  }
  CALLBACK0(EndFunctionSection);
  return Result::Ok;
}

Result BinaryReader::ReadTableSection(Offset section_size) {
  CALLBACK(BeginTableSection, section_size);
  CHECK_RESULT(ReadCount(&num_tables_, "table count"));
  ERROR_UNLESS(num_tables_ <= 1, "table count (%" PRIindex ") must be 0 or 1",
               num_tables_);
  CALLBACK(OnTableCount, num_tables_);
  for (Index i = 0; i < num_tables_; ++i) {
    Index table_index = num_table_imports_ + i;
    Type elem_type;
    Limits elem_limits;
    CHECK_RESULT(ReadTable(&elem_type, &elem_limits));
    CALLBACK(OnTable, table_index, elem_type, &elem_limits);
  }
  CALLBACK0(EndTableSection);
  return Result::Ok;
}

Result BinaryReader::ReadMemorySection(Offset section_size) {
  CALLBACK(BeginMemorySection, section_size);
  CHECK_RESULT(ReadCount(&num_memories_, "memory count"));
  ERROR_UNLESS(num_memories_ <= 1, "memory count must be 0 or 1");
  CALLBACK(OnMemoryCount, num_memories_);
  for (Index i = 0; i < num_memories_; ++i) {
    Index memory_index = num_memory_imports_ + i;
    Limits page_limits;
    CHECK_RESULT(ReadMemory(&page_limits));
    CALLBACK(OnMemory, memory_index, &page_limits);
  }
  CALLBACK0(EndMemorySection);
  return Result::Ok;
}

Result BinaryReader::ReadGlobalSection(Offset section_size) {
  CALLBACK(BeginGlobalSection, section_size);
  CHECK_RESULT(ReadCount(&num_globals_, "global count"));
  CALLBACK(OnGlobalCount, num_globals_);
  for (Index i = 0; i < num_globals_; ++i) {
    Index global_index = num_global_imports_ + i;
    Type global_type;
    bool mutable_;
    CHECK_RESULT(ReadGlobalHeader(&global_type, &mutable_));
    CALLBACK(BeginGlobal, global_index, global_type, mutable_);
    CALLBACK(BeginGlobalInitExpr, global_index);
    CHECK_RESULT(ReadInitExpr(global_index));
    CALLBACK(EndGlobalInitExpr, global_index);
    CALLBACK(EndGlobal, global_index);
  }
  CALLBACK0(EndGlobalSection);
  return Result::Ok;
}

Result BinaryReader::ReadExportSection(Offset section_size) {
  CALLBACK(BeginExportSection, section_size);
  CHECK_RESULT(ReadCount(&num_exports_, "export count"));
  CALLBACK(OnExportCount, num_exports_);
  for (Index i = 0; i < num_exports_; ++i) {
    string_view name;
    CHECK_RESULT(ReadStr(&name, "export item name"));

    uint8_t kind = 0;
    CHECK_RESULT(ReadU8(&kind, "export kind"));
    ERROR_UNLESS(is_valid_external_kind(kind),
                 "invalid export external kind: %d", kind);

    Index item_index;
    CHECK_RESULT(ReadIndex(&item_index, "export item index"));
    switch (static_cast<ExternalKind>(kind)) {
      case ExternalKind::Func:
        ERROR_UNLESS(item_index < NumTotalFuncs(),
                     "invalid export func index: %" PRIindex, item_index);
        break;
      case ExternalKind::Table:
        ERROR_UNLESS(item_index < NumTotalTables(),
                     "invalid export table index: %" PRIindex, item_index);
        break;
      case ExternalKind::Memory:
        ERROR_UNLESS(item_index < NumTotalMemories(),
                     "invalid export memory index: %" PRIindex, item_index);
        break;
      case ExternalKind::Global:
        ERROR_UNLESS(item_index < NumTotalGlobals(),
                     "invalid export global index: %" PRIindex, item_index);
        break;
      case ExternalKind::Except:
        // Note: Can't check if index valid, exceptions section comes later.
        ERROR_UNLESS(options_->features.exceptions_enabled(),
                     "invalid export exception kind: exceptions not allowed");
        break;
    }

    CALLBACK(OnExport, i, static_cast<ExternalKind>(kind), item_index, name);
  }
  CALLBACK0(EndExportSection);
  return Result::Ok;
}

Result BinaryReader::ReadStartSection(Offset section_size) {
  CALLBACK(BeginStartSection, section_size);
  Index func_index;
  CHECK_RESULT(ReadIndex(&func_index, "start function index"));
  ERROR_UNLESS(func_index < NumTotalFuncs(),
               "invalid start function index: %" PRIindex, func_index);
  CALLBACK(OnStartFunction, func_index);
  CALLBACK0(EndStartSection);
  return Result::Ok;
}

Result BinaryReader::ReadElemSection(Offset section_size) {
  CALLBACK(BeginElemSection, section_size);
  Index num_elem_segments;
  CHECK_RESULT(ReadCount(&num_elem_segments, "elem segment count"));
  CALLBACK(OnElemSegmentCount, num_elem_segments);
  ERROR_UNLESS(num_elem_segments == 0 || NumTotalTables() > 0,
               "elem section without table section");
  for (Index i = 0; i < num_elem_segments; ++i) {
    Index table_index;
    CHECK_RESULT(ReadIndex(&table_index, "elem segment table index"));
    CALLBACK(BeginElemSegment, i, table_index);
    CALLBACK(BeginElemSegmentInitExpr, i);
    CHECK_RESULT(ReadI32InitExpr(i));
    CALLBACK(EndElemSegmentInitExpr, i);

    Index num_function_indexes;
    CHECK_RESULT(
        ReadCount(&num_function_indexes, "elem segment function index count"));
    CALLBACK(OnElemSegmentFunctionIndexCount, i, num_function_indexes);
    for (Index j = 0; j < num_function_indexes; ++j) {
      Index func_index;
      CHECK_RESULT(ReadIndex(&func_index, "elem segment function index"));
      CALLBACK(OnElemSegmentFunctionIndex, i, func_index);
    }
    CALLBACK(EndElemSegment, i);
  }
  CALLBACK0(EndElemSection);
  return Result::Ok;
}

Result BinaryReader::ReadCodeSection(Offset section_size) {
  CALLBACK(BeginCodeSection, section_size);
  CHECK_RESULT(ReadCount(&num_function_bodies_, "function body count"));
  ERROR_UNLESS(num_function_signatures_ == num_function_bodies_,
               "function signature count != function body count");
  CALLBACK(OnFunctionBodyCount, num_function_bodies_);
  for (Index i = 0; i < num_function_bodies_; ++i) {
    Index func_index = num_func_imports_ + i;
    Offset func_offset = state_.offset;
    state_.offset = func_offset;
    CALLBACK(BeginFunctionBody, func_index);
    uint32_t body_size;
    CHECK_RESULT(ReadU32Leb128(&body_size, "function body size"));
    Offset body_start_offset = state_.offset;
    Offset end_offset = body_start_offset + body_size;

    uint64_t total_locals = 0;
    Index num_local_decls;
    CHECK_RESULT(ReadCount(&num_local_decls, "local declaration count"));
    CALLBACK(OnLocalDeclCount, num_local_decls);
    for (Index k = 0; k < num_local_decls; ++k) {
      Index num_local_types;
      CHECK_RESULT(ReadIndex(&num_local_types, "local type count"));
      ERROR_UNLESS(num_local_types > 0, "local count must be > 0");
      total_locals += num_local_types;
      ERROR_UNLESS(total_locals < UINT32_MAX,
                   "local count must be < 0x10000000");
      Type local_type;
      CHECK_RESULT(ReadType(&local_type, "local type"));
      ERROR_UNLESS(IsConcreteType(local_type), "expected valid local type");
      CALLBACK(OnLocalDecl, k, num_local_types, local_type);
    }

    CHECK_RESULT(ReadFunctionBody(end_offset));

    CALLBACK(EndFunctionBody, func_index);
  }
  CALLBACK0(EndCodeSection);
  return Result::Ok;
}

Result BinaryReader::ReadDataSection(Offset section_size) {
  CALLBACK(BeginDataSection, section_size);
  Index num_data_segments;
  CHECK_RESULT(ReadCount(&num_data_segments, "data segment count"));
  CALLBACK(OnDataSegmentCount, num_data_segments);
  ERROR_UNLESS(num_data_segments == 0 || NumTotalMemories() > 0,
               "data section without memory section");
  for (Index i = 0; i < num_data_segments; ++i) {
    Index memory_index;
    CHECK_RESULT(ReadIndex(&memory_index, "data segment memory index"));
    CALLBACK(BeginDataSegment, i, memory_index);
    CALLBACK(BeginDataSegmentInitExpr, i);
    CHECK_RESULT(ReadI32InitExpr(i));
    CALLBACK(EndDataSegmentInitExpr, i);

    Address data_size;
    const void* data;
    CHECK_RESULT(ReadBytes(&data, &data_size, "data segment data"));
    CALLBACK(OnDataSegmentData, i, data, data_size);
    CALLBACK(EndDataSegment, i);
  }
  CALLBACK0(EndDataSection);
  return Result::Ok;
}

Result BinaryReader::ReadSections() {
  Result result = Result::Ok;

  while (state_.offset < state_.size) {
    uint32_t section_code;
    Offset section_size;
    CHECK_RESULT(ReadU32Leb128(&section_code, "section code"));
    CHECK_RESULT(ReadOffset(&section_size, "section size"));
    ReadEndRestoreGuard guard(this);
    read_end_ = state_.offset + section_size;
    if (section_code >= kBinarySectionCount) {
      PrintError("invalid section code: %u; max is %u", section_code,
                 kBinarySectionCount - 1);
      return Result::Error;
    }

    BinarySection section = static_cast<BinarySection>(section_code);

    ERROR_UNLESS(read_end_ <= state_.size,
                 "invalid section size: extends past end");

    ERROR_UNLESS(last_known_section_ == BinarySection::Invalid ||
                     section == BinarySection::Custom ||
                     section > last_known_section_,
                 "section %s out of order", GetSectionName(section));

    ERROR_UNLESS(!did_read_names_section_ || section == BinarySection::Custom,
                 "%s section can not occur after Name section",
                 GetSectionName(section));

    CALLBACK(BeginSection, section, section_size);

    bool stop_on_first_error = options_->stop_on_first_error;
    Result section_result = Result::Error;
    switch (section) {
      case BinarySection::Custom:
        section_result = ReadCustomSection(section_size);
        if (options_->fail_on_custom_section_error) {
          result |= section_result;
        } else {
          stop_on_first_error = false;
        }
        break;
      case BinarySection::Type:
        section_result = ReadTypeSection(section_size);
        result |= section_result;
        break;
      case BinarySection::Import:
        section_result = ReadImportSection(section_size);
        result |= section_result;
        break;
      case BinarySection::Function:
        section_result = ReadFunctionSection(section_size);
        result |= section_result;
        break;
      case BinarySection::Table:
        section_result = ReadTableSection(section_size);
        result |= section_result;
        break;
      case BinarySection::Memory:
        section_result = ReadMemorySection(section_size);
        result |= section_result;
        break;
      case BinarySection::Global:
        section_result = ReadGlobalSection(section_size);
        result |= section_result;
        break;
      case BinarySection::Export:
        section_result = ReadExportSection(section_size);
        result |= section_result;
        break;
      case BinarySection::Start:
        section_result = ReadStartSection(section_size);
        result |= section_result;
        break;
      case BinarySection::Elem:
        section_result = ReadElemSection(section_size);
        result |= section_result;
        break;
      case BinarySection::Code:
        section_result = ReadCodeSection(section_size);
        result |= section_result;
        break;
      case BinarySection::Data:
        section_result = ReadDataSection(section_size);
        result |= section_result;
        break;
      case BinarySection::Invalid:
        WABT_UNREACHABLE;
    }

    if (Failed(section_result)) {
      if (stop_on_first_error) {
        return Result::Error;
      }

      // If we're continuing after failing to read this section, move the
      // offset to the expected section end. This way we may be able to read
      // further sections.
      state_.offset = read_end_;
    }

    ERROR_UNLESS(state_.offset == read_end_,
                 "unfinished section (expected end: 0x%" PRIzx ")", read_end_);

    if (section != BinarySection::Custom) {
      last_known_section_ = section;
    }
  }

  return result;
}

Result BinaryReader::ReadModule() {
  uint32_t magic = 0;
  CHECK_RESULT(ReadU32(&magic, "magic"));
  ERROR_UNLESS(magic == WABT_BINARY_MAGIC, "bad magic value");
  uint32_t version = 0;
  CHECK_RESULT(ReadU32(&version, "version"));
  ERROR_UNLESS(version == WABT_BINARY_VERSION,
               "bad wasm file version: %#x (expected %#x)", version,
               WABT_BINARY_VERSION);

  CALLBACK(BeginModule, version);
  CHECK_RESULT(ReadSections());
  CALLBACK0(EndModule);

  return Result::Ok;
}

}  // end anonymous namespace

Result ReadBinary(const void* data,
                  size_t size,
                  BinaryReaderDelegate* delegate,
                  const ReadBinaryOptions* options) {
  BinaryReader reader(data, size, delegate, options);
  return reader.ReadModule();
}

}  // namespace wabt

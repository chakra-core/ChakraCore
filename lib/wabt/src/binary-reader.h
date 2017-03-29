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

#ifndef WABT_BINARY_READER_H_
#define WABT_BINARY_READER_H_

#include <stddef.h>
#include <stdint.h>

#include "binary.h"
#include "common.h"

#define WABT_READ_BINARY_OPTIONS_DEFAULT \
  { nullptr, false }

namespace wabt {

struct ReadBinaryOptions {
  struct Stream* log_stream;
  bool read_debug_names;
};

struct BinaryReaderContext {
  const uint8_t* data;
  size_t size;
  size_t offset;
  void* user_data;
};

struct BinaryReader {
  void* user_data;

  bool (*on_error)(BinaryReaderContext* ctx, const char* message);

  /* module */
  Result (*begin_module)(uint32_t version, void* user_data);
  Result (*end_module)(void* user_data);

  Result (*begin_section)(BinaryReaderContext* ctx,
                          BinarySection section_type,
                          uint32_t size);

  /* custom section */
  Result (*begin_custom_section)(BinaryReaderContext* ctx,
                                 uint32_t size,
                                 StringSlice section_name);
  Result (*end_custom_section)(BinaryReaderContext* ctx);

  /* signatures section */
  /* TODO(binji): rename to "type" section */
  Result (*begin_signature_section)(BinaryReaderContext* ctx, uint32_t size);
  Result (*on_signature_count)(uint32_t count, void* user_data);
  Result (*on_signature)(uint32_t index,
                         uint32_t param_count,
                         Type* param_types,
                         uint32_t result_count,
                         Type* result_types,
                         void* user_data);
  Result (*end_signature_section)(BinaryReaderContext* ctx);

  /* import section */
  Result (*begin_import_section)(BinaryReaderContext* ctx, uint32_t size);
  Result (*on_import_count)(uint32_t count, void* user_data);
  Result (*on_import)(uint32_t index,
                      StringSlice module_name,
                      StringSlice field_name,
                      void* user_data);
  Result (*on_import_func)(uint32_t import_index,
                           StringSlice module_name,
                           StringSlice field_name,
                           uint32_t func_index,
                           uint32_t sig_index,
                           void* user_data);
  Result (*on_import_table)(uint32_t import_index,
                            StringSlice module_name,
                            StringSlice field_name,
                            uint32_t table_index,
                            Type elem_type,
                            const Limits* elem_limits,
                            void* user_data);
  Result (*on_import_memory)(uint32_t import_index,
                             StringSlice module_name,
                             StringSlice field_name,
                             uint32_t memory_index,
                             const Limits* page_limits,
                             void* user_data);
  Result (*on_import_global)(uint32_t import_index,
                             StringSlice module_name,
                             StringSlice field_name,
                             uint32_t global_index,
                             Type type,
                             bool mutable_,
                             void* user_data);
  Result (*end_import_section)(BinaryReaderContext* ctx);

  /* function signatures section */
  /* TODO(binji): rename to "function" section */
  Result (*begin_function_signatures_section)(BinaryReaderContext* ctx,
                                              uint32_t size);
  Result (*on_function_signatures_count)(uint32_t count, void* user_data);
  Result (*on_function_signature)(uint32_t index,
                                  uint32_t sig_index,
                                  void* user_data);
  Result (*end_function_signatures_section)(BinaryReaderContext* ctx);

  /* table section */
  Result (*begin_table_section)(BinaryReaderContext* ctx, uint32_t size);
  Result (*on_table_count)(uint32_t count, void* user_data);
  Result (*on_table)(uint32_t index,
                     Type elem_type,
                     const Limits* elem_limits,
                     void* user_data);
  Result (*end_table_section)(BinaryReaderContext* ctx);

  /* memory section */
  Result (*begin_memory_section)(BinaryReaderContext* ctx, uint32_t size);
  Result (*on_memory_count)(uint32_t count, void* user_data);
  Result (*on_memory)(uint32_t index, const Limits* limits, void* user_data);
  Result (*end_memory_section)(BinaryReaderContext* ctx);

  /* global section */
  Result (*begin_global_section)(BinaryReaderContext* ctx, uint32_t size);
  Result (*on_global_count)(uint32_t count, void* user_data);
  Result (*begin_global)(uint32_t index,
                         Type type,
                         bool mutable_,
                         void* user_data);
  Result (*begin_global_init_expr)(uint32_t index, void* user_data);
  Result (*end_global_init_expr)(uint32_t index, void* user_data);
  Result (*end_global)(uint32_t index, void* user_data);
  Result (*end_global_section)(BinaryReaderContext* ctx);

  /* exports section */
  Result (*begin_export_section)(BinaryReaderContext* ctx, uint32_t size);
  Result (*on_export_count)(uint32_t count, void* user_data);
  Result (*on_export)(uint32_t index,
                      ExternalKind kind,
                      uint32_t item_index,
                      StringSlice name,
                      void* user_data);
  Result (*end_export_section)(BinaryReaderContext* ctx);

  /* start section */
  Result (*begin_start_section)(BinaryReaderContext* ctx, uint32_t size);
  Result (*on_start_function)(uint32_t func_index, void* user_data);
  Result (*end_start_section)(BinaryReaderContext* ctx);

  /* function bodies section */
  /* TODO(binji): rename to code section */
  Result (*begin_function_bodies_section)(BinaryReaderContext* ctx,
                                          uint32_t size);
  Result (*on_function_bodies_count)(uint32_t count, void* user_data);
  Result (*begin_function_body_pass)(uint32_t index,
                                     uint32_t pass,
                                     void* user_data);
  Result (*begin_function_body)(BinaryReaderContext* ctx, uint32_t index);
  Result (*on_local_decl_count)(uint32_t count, void* user_data);
  Result (*on_local_decl)(uint32_t decl_index,
                          uint32_t count,
                          Type type,
                          void* user_data);

  /* function expressions; called between begin_function_body and
   end_function_body */
  Result (*on_opcode)(BinaryReaderContext* ctx, Opcode Opcode);
  Result (*on_opcode_bare)(BinaryReaderContext* ctx);
  Result (*on_opcode_uint32)(BinaryReaderContext* ctx, uint32_t value);
  Result (*on_opcode_uint32_uint32)(BinaryReaderContext* ctx,
                                    uint32_t value,
                                    uint32_t value2);
  Result (*on_opcode_uint64)(BinaryReaderContext* ctx, uint64_t value);
  Result (*on_opcode_f32)(BinaryReaderContext* ctx, uint32_t value);
  Result (*on_opcode_f64)(BinaryReaderContext* ctx, uint64_t value);
  Result (*on_opcode_block_sig)(BinaryReaderContext* ctx,
                                uint32_t num_types,
                                Type* sig_types);
  Result (*on_binary_expr)(Opcode opcode, void* user_data);
  Result (*on_block_expr)(uint32_t num_types, Type* sig_types, void* user_data);
  Result (*on_br_expr)(uint32_t depth, void* user_data);
  Result (*on_br_if_expr)(uint32_t depth, void* user_data);
  Result (*on_br_table_expr)(BinaryReaderContext* ctx,
                             uint32_t num_targets,
                             uint32_t* target_depths,
                             uint32_t default_target_depth);
  Result (*on_call_expr)(uint32_t func_index, void* user_data);
  Result (*on_call_import_expr)(uint32_t import_index, void* user_data);
  Result (*on_call_indirect_expr)(uint32_t sig_index, void* user_data);
  Result (*on_compare_expr)(Opcode opcode, void* user_data);
  Result (*on_convert_expr)(Opcode opcode, void* user_data);
  Result (*on_drop_expr)(void* user_data);
  Result (*on_else_expr)(void* user_data);
  Result (*on_end_expr)(void* user_data);
  Result (*on_end_func)(void* user_data);
  Result (*on_f32_const_expr)(uint32_t value_bits, void* user_data);
  Result (*on_f64_const_expr)(uint64_t value_bits, void* user_data);
  Result (*on_get_global_expr)(uint32_t global_index, void* user_data);
  Result (*on_get_local_expr)(uint32_t local_index, void* user_data);
  Result (*on_grow_memory_expr)(void* user_data);
  Result (*on_i32_const_expr)(uint32_t value, void* user_data);
  Result (*on_i64_const_expr)(uint64_t value, void* user_data);
  Result (*on_if_expr)(uint32_t num_types, Type* sig_types, void* user_data);
  Result (*on_load_expr)(Opcode opcode,
                         uint32_t alignment_log2,
                         uint32_t offset,
                         void* user_data);
  Result (*on_loop_expr)(uint32_t num_types, Type* sig_types, void* user_data);
  Result (*on_current_memory_expr)(void* user_data);
  Result (*on_nop_expr)(void* user_data);
  Result (*on_return_expr)(void* user_data);
  Result (*on_select_expr)(void* user_data);
  Result (*on_set_global_expr)(uint32_t global_index, void* user_data);
  Result (*on_set_local_expr)(uint32_t local_index, void* user_data);
  Result (*on_store_expr)(Opcode opcode,
                          uint32_t alignment_log2,
                          uint32_t offset,
                          void* user_data);
  Result (*on_tee_local_expr)(uint32_t local_index, void* user_data);
  Result (*on_unary_expr)(Opcode opcode, void* user_data);
  Result (*on_unreachable_expr)(void* user_data);
  Result (*end_function_body)(uint32_t index, void* user_data);
  Result (*end_function_body_pass)(uint32_t index,
                                   uint32_t pass,
                                   void* user_data);
  Result (*end_function_bodies_section)(BinaryReaderContext* ctx);

  /* elem section */
  Result (*begin_elem_section)(BinaryReaderContext* ctx, uint32_t size);
  Result (*on_elem_segment_count)(uint32_t count, void* user_data);
  Result (*begin_elem_segment)(uint32_t index,
                               uint32_t table_index,
                               void* user_data);
  Result (*begin_elem_segment_init_expr)(uint32_t index, void* user_data);
  Result (*end_elem_segment_init_expr)(uint32_t index, void* user_data);
  Result (*on_elem_segment_function_index_count)(BinaryReaderContext* ctx,
                                                 uint32_t index,
                                                 uint32_t count);
  Result (*on_elem_segment_function_index)(uint32_t index,
                                           uint32_t func_index,
                                           void* user_data);
  Result (*end_elem_segment)(uint32_t index, void* user_data);
  Result (*end_elem_section)(BinaryReaderContext* ctx);

  /* data section */
  Result (*begin_data_section)(BinaryReaderContext* ctx, uint32_t size);
  Result (*on_data_segment_count)(uint32_t count, void* user_data);
  Result (*begin_data_segment)(uint32_t index,
                               uint32_t memory_index,
                               void* user_data);
  Result (*begin_data_segment_init_expr)(uint32_t index, void* user_data);
  Result (*end_data_segment_init_expr)(uint32_t index, void* user_data);
  Result (*on_data_segment_data)(uint32_t index,
                                 const void* data,
                                 uint32_t size,
                                 void* user_data);
  Result (*end_data_segment)(uint32_t index, void* user_data);
  Result (*end_data_section)(BinaryReaderContext* ctx);

  /* names section */
  Result (*begin_names_section)(BinaryReaderContext* ctx, uint32_t size);
  Result (*on_function_name_subsection)(uint32_t index,
                                        uint32_t name_type,
                                        uint32_t subsection_size,
                                        void* user_data);
  Result (*on_function_names_count)(uint32_t num_functions,
                                    void* user_data);
  Result (*on_function_name)(uint32_t function_index,
                             StringSlice function_name,
                             void* user_data);
  Result (*on_local_name_subsection)(uint32_t index,
                                     uint32_t name_type,
                                     uint32_t subsection_size,
                                     void* user_data);
  Result (*on_local_name_function_count)(uint32_t num_functions,
                                         void* user_data);
  Result (*on_local_name_local_count)(uint32_t function_index,
                                      uint32_t num_locals,
                                      void* user_data);
  Result (*on_local_name)(uint32_t function_index,
                          uint32_t local_index,
                          StringSlice local_name,
                          void* user_data);
  Result (*end_names_section)(BinaryReaderContext* ctx);

  /* names section */
  Result (*begin_reloc_section)(BinaryReaderContext* ctx, uint32_t size);
  Result (*on_reloc_count)(uint32_t count,
                           BinarySection section_code,
                           StringSlice section_name,
                           void* user_data);
  Result (*on_reloc)(RelocType type,
                     uint32_t offset,
                     uint32_t index,
                     int32_t addend,
                     void* user_data);
  Result (*end_reloc_section)(BinaryReaderContext* ctx);

  /* init_expr - used by elem, data and global sections; these functions are
   * only called between calls to begin_*_init_expr and end_*_init_expr */
  Result (*on_init_expr_f32_const_expr)(uint32_t index,
                                        uint32_t value,
                                        void* user_data);
  Result (*on_init_expr_f64_const_expr)(uint32_t index,
                                        uint64_t value,
                                        void* user_data);
  Result (*on_init_expr_get_global_expr)(uint32_t index,
                                         uint32_t global_index,
                                         void* user_data);
  Result (*on_init_expr_i32_const_expr)(uint32_t index,
                                        uint32_t value,
                                        void* user_data);
  Result (*on_init_expr_i64_const_expr)(uint32_t index,
                                        uint64_t value,
                                        void* user_data);
};

Result read_binary(const void* data,
                   size_t size,
                   BinaryReader* reader,
                   uint32_t num_function_passes,
                   const ReadBinaryOptions* options);

size_t read_u32_leb128(const uint8_t* ptr,
                       const uint8_t* end,
                       uint32_t* out_value);

size_t read_i32_leb128(const uint8_t* ptr,
                       const uint8_t* end,
                       uint32_t* out_value);

}  // namespace wabt

#endif /* WABT_BINARY_READER_H_ */

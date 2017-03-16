//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

#ifdef ENABLE_WABT
#include "../WasmReader/WasmReaderPch.h"

#include "ast-lexer.h"
#include "common.h"
#include "ast.h"
#include "ast-parser.h"
#include "binary-writer.h"
#include "binary-writer-spec.h"
#include "resolve-names.h"
#include "validator.h"
#include "Codex/Utf8Helper.h"
using namespace wabt;

struct MemoryWriterContext
{
    MemoryWriter* writer;
    ArenaAllocator* alloc;
};

void ensure_output_buffer_capacity(OutputBuffer* buf, size_t ensure_capacity, MemoryWriterContext* context)
{
    if (ensure_capacity > buf->capacity)
    {
        Assert(buf->capacity != 0);
        size_t newCapacity = buf->capacity * 2;
        while (newCapacity < ensure_capacity)
            newCapacity *= 2;
        char* new_data = AnewArrayZ(context->alloc, char, newCapacity);
        js_memcpy_s(new_data, newCapacity, buf->start, buf->capacity);
        buf->start = new_data;
        buf->capacity = newCapacity;
    }
}

wabt::Result write_data_to_output_buffer(size_t offset,
                                          const void* data,
                                          size_t size,
                                          void* user_data)
{
    MemoryWriterContext* context = static_cast<MemoryWriterContext*>(user_data);
    MemoryWriter* writer = context->writer;
    size_t end = offset + size;
    ensure_output_buffer_capacity(&writer->buf, end, context);
    memcpy(writer->buf.start + offset, data, size);
    if (end > writer->buf.size)
        writer->buf.size = end;
    return Result::Ok;
}

wabt::Result move_data_in_output_buffer(size_t dst_offset,
                                         size_t src_offset,
                                         size_t size,
                                         void* user_data)
{
    MemoryWriterContext* context = static_cast<MemoryWriterContext*>(user_data);
    MemoryWriter* writer = context->writer;
    size_t src_end = src_offset + size;
    size_t dst_end = dst_offset + size;
    size_t end = src_end > dst_end ? src_end : dst_end;
    ensure_output_buffer_capacity(&writer->buf, end, context);
    void* dst = reinterpret_cast<void*>(
        reinterpret_cast<size_t>(writer->buf.start) + dst_offset);
    void* src = reinterpret_cast<void*>(
        reinterpret_cast<size_t>(writer->buf.start) + src_offset);
    memmove(dst, src, size);
    if (end > writer->buf.size)
        writer->buf.size = end;
    return Result::Ok;
}

Js::Var Js::WabtInterface::EntryConvertWast2Wasm(RecyclableObject* function, CallInfo callInfo, ...)
{
    Js::ScriptContext* scriptContext = function->GetScriptContext();
    Var returnValue = scriptContext->GetLibrary()->GetUndefined();
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

    Assert(!(callInfo.Flags & CallFlags_New));

    if (args.Info.Count < 2 || !Js::JavascriptString::Is(args[1]))
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedBufferSource);
    }
    Js::JavascriptString* string = (Js::JavascriptString*)args[1];
    const char16* str = string->GetString();
    ArenaAllocator arena(_u("Wast2Wasm"), scriptContext->GetThreadContext()->GetPageAllocator(), Js::Throw::OutOfMemory);
    size_t origSize = string->GetLength();
    size_t wastSize;
    char* wastBuffer = nullptr;
    auto allocator = [&arena](size_t size) {return (utf8char_t*)AnewArray(&arena, byte, size);};
    utf8::WideStringToNarrow(allocator, str, origSize, &wastBuffer, &wastSize);

    AstLexer* lexer = new_ast_buffer_lexer("", wastBuffer, wastSize);
    struct AutoCleanLexer
    {
        AstLexer* lexer;
        ~AutoCleanLexer()
        {
            wabt::destroy_ast_lexer(lexer);
        }
    };
    AutoCleanLexer autoCleanLexer = { lexer };
    Unused(autoCleanLexer);

    auto errorCallback = [](const wabt::Location*,
                            const char* error,
                            const char* source_line,
                            size_t source_line_length,
                            size_t source_line_column_offset,
                            void* user_data)
    {
        Js::JavascriptError::ThrowError((ScriptContext*)user_data, WASMERR_WasmCompileError);
    };
    wabt::SourceErrorHandler s_error_handler = {
        errorCallback,
        120,
        scriptContext
    };
    bool s_validate = true;

    wabt::Script script;
    wabt::Result result = wabt::parse_ast(lexer, &script, &s_error_handler);
    struct AutoCleanScript
    {
        wabt::Script* script;
        ~AutoCleanScript()
        {
            wabt::destroy_script(script);
        }
    };
    AutoCleanScript autoCleanScript = { &script };
    Unused(autoCleanScript);

    //wabt::WriteBinarySpecOptions s_write_binary_spec_options;

    if (WABT_SUCCEEDED(result))
    {
        result = wabt::resolve_names_script(lexer, &script, &s_error_handler);

        if (WABT_SUCCEEDED(result) && s_validate)
            result = wabt::validate_script(lexer, &script, &s_error_handler);

        if (WABT_SUCCEEDED(result))
        {
            /*
            if (s_spec)
            {
                s_write_binary_spec_options.json_filename = s_outfile;
                s_write_binary_spec_options.write_binary_options =
                    s_write_binary_options;
                result = write_binary_spec_script(&script, s_infile,
                                                  &s_write_binary_spec_options);
            }
            else
            */
            {
                MemoryWriter writer;
                MemoryWriterContext context{ &writer, &arena };
                writer.base.move_data = move_data_in_output_buffer;
                writer.base.write_data = write_data_to_output_buffer;
                writer.base.user_data = &context;
                writer.buf.size = 0;
                writer.buf.capacity = 256;
                writer.buf.start = AnewArrayZ(&arena, char, writer.buf.capacity);
                Module* module = get_first_module(&script);
                if (module)
                {
                    wabt::WriteBinaryOptions s_write_binary_options = { nullptr, true, false, false };
                    result = write_binary_module(&writer.base, module, &s_write_binary_options);
                }
                else
                {
                    WABT_FATAL("no module found\n");
                }

                if (WABT_SUCCEEDED(result))
                {
                    uint32 size = (uint32)writer.buf.size;
                    Js::ArrayBuffer* arrayBuffer = scriptContext->GetLibrary()->CreateArrayBuffer(size);
                    js_memcpy_s(arrayBuffer->GetBuffer(), arrayBuffer->GetByteLength(), writer.buf.start, size);
                    returnValue = arrayBuffer;
                }
            }
        }
    }
    return returnValue;
}
#endif // ENABLE_WABT

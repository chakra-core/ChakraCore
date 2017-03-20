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

namespace Js
{

struct Context
{
    ArenaAllocator* allocator;
    ScriptContext* scriptContext;
};

struct MemoryWriterContext
{
    MemoryWriter* writer;
    ArenaAllocator* alloc;
};

struct AutoCleanLexer
{
    AstLexer* lexer;
    ~AutoCleanLexer()
    {
        wabt::destroy_ast_lexer(lexer);
    }
};
struct AutoCleanScript
{
    wabt::Script* script;
    ~AutoCleanScript()
    {
        wabt::destroy_script(script);
    }
};

char16* NarrowStringToWide(Context* ctx, const char* src, const size_t* srcSize = nullptr, size_t* dstSize = nullptr)
{
    auto allocator = [&ctx](size_t size) {return (char16*)AnewArray(ctx->allocator, char16, size); };
    char16* dst = nullptr;
    size_t size;
    HRESULT hr = utf8::NarrowStringToWide(allocator, src, srcSize ? *srcSize : strlen(src), &dst, &size);
    if (hr != S_OK)
    {
        JavascriptError::ThrowOutOfMemoryError(ctx->scriptContext);
    }
    if (dstSize)
    {
        *dstSize = size;
    }
    return dst;
}

void lexer_error_callback(const wabt::Location* loc,
                        const char* error,
                        const char* source_line,
                        size_t source_line_length,
                        size_t source_line_column_offset,
                        void* user_data)
{
    Context* ctx = (Context*)user_data;
    char16* errorMsg = NarrowStringToWide(ctx, error);
    char16* sourceText = NarrowStringToWide(ctx, source_line, &source_line_length);
    char16 buf[1024];
    _snwprintf_s(buf, _countof(buf), _TRUNCATE, _u("%*s^%*s^"), loc->first_column - 1, _u(""), loc->last_column - loc->first_column - 1, _u(""));

    JavascriptError::ThrowSyntaxErrorVar(ctx->scriptContext, (int)WABTERR_WastParsingError, 
                                         loc->line,
                                         loc->first_column,
                                         sourceText,
                                         buf,
                                         errorMsg);
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

JavascriptString* ToJavascriptString(Context* ctx, wabt::StringSlice src)
{
    size_t bufSize = 0;
    char16* buf = NarrowStringToWide(ctx, src.start, &src.length, &bufSize);
    Assert(bufSize < UINT32_MAX);
    return JavascriptString::NewCopyBuffer(buf, (charcount_t)bufSize, ctx->scriptContext);
}

template<typename T>
void write_number(Context* ctx, Js::Var obj, PropertyId id, const T value)
{
    Js::Var line = JavascriptNumber::ToVar(value, ctx->scriptContext);
    BOOL hr = JavascriptOperators::OP_SetProperty(obj, id, line, ctx->scriptContext);
    if (!hr)
    {
        JavascriptError::ThrowTypeErrorVar(ctx->scriptContext, WABTERR_WabtError, _u("Unable to write number"));
    }
}

void write_string(Context* ctx, Js::Var obj, PropertyId id, const char16* src)
{
    JavascriptString* str = LiteralString::NewCopySz(src, ctx->scriptContext);
    BOOL hr = JavascriptOperators::OP_SetProperty(obj, id, str, ctx->scriptContext);
    if (!hr)
    {
        JavascriptError::ThrowTypeErrorVar(ctx->scriptContext, WABTERR_WabtError, _u("Unable to write string"));
    }
}

void write_string(Context* ctx, Js::Var obj, PropertyId id, wabt::StringSlice src)
{
    JavascriptString* str = ToJavascriptString(ctx, src);
    BOOL hr = JavascriptOperators::OP_SetProperty(obj, id, str, ctx->scriptContext);
    if (!hr)
    {
        JavascriptError::ThrowTypeErrorVar(ctx->scriptContext, WABTERR_WabtError, _u("Unable to write string"));
    }
}

void write_location(Context* ctx, Js::Var obj, const Location* loc)
{
    write_number(ctx, obj, PropertyIds::line, loc->line);
}

void write_var(Context* ctx, Js::Var obj, PropertyId id, const wabt::Var* var)
{
    if (var->type == wabt::VarType::Index)
    {
        write_number(ctx, obj, id, var->index);
    }
    else
    {
        Assert(var->type == wabt::VarType::Name);
        write_string(ctx, obj, id, NarrowStringToWide(ctx, var->name.start, &var->name.length));
    }
}

void write_command_type(Context* ctx, Command* command, Js::Var cmdObj)
{
    static const char16* s_command_names[] = {
        _u("module"), _u("action"), _u("register"), _u("assert_malformed"), _u("assert_invalid"),
        nullptr, // ASSERT_INVALID_NON_BINARY, this command will never be written
        _u("assert_unlinkable"), _u("assert_uninstantiable"), _u("assert_return"),
        _u("assert_return_nan"), _u("assert_trap"), _u("assert_exhaustion"),
    };
    CompileAssert(ARRAYSIZE(s_command_names) == (int)CommandType::Last + 1);
    uint i = (uint)command->type;
    if (i > (uint)CommandType::Last)
    {
        JavascriptError::ThrowTypeErrorVar(ctx->scriptContext, WABTERR_WabtError, _u("invalid command type"));
    }
    write_string(ctx, cmdObj, PropertyIds::type, s_command_names[i]);
}

Js::Var create_const_vector(Context* ctx, const ConstVector* consts)
{
    ScriptContext* scriptContext = ctx->scriptContext;
    Js::Var constsArr = JavascriptOperators::NewJavascriptArrayNoArg(scriptContext);

    size_t i;
    for (i = 0; i < consts->size; ++i)
    {
        Js::Var constDescriptor = JavascriptOperators::NewJavascriptObjectNoArg(scriptContext);
        JavascriptArray::Push(scriptContext, constsArr, constDescriptor);

        char16 buf[32];
        const wabt::Const* const_ = &consts->data[i];
        switch (const_->type)
        {
        case wabt::Type::I32:
            write_string(ctx, constDescriptor, PropertyIds::type, _u("i32"));
            _snwprintf_s(buf, _countof(buf), _TRUNCATE, _u("%u"), const_->u32);
            break;
        case wabt::Type::I64:
            write_string(ctx, constDescriptor, PropertyIds::type, _u("i64"));
            _snwprintf_s(buf, _countof(buf), _TRUNCATE, _u("%llu"), const_->u64);
            break;
        case wabt::Type::F32:
            write_string(ctx, constDescriptor, PropertyIds::type, _u("f32"));
            _snwprintf_s(buf, _countof(buf), _TRUNCATE, _u("%u"), const_->f32_bits);
            break;
        case wabt::Type::F64:
            write_string(ctx, constDescriptor, PropertyIds::type, _u("f64"));
            _snwprintf_s(buf, _countof(buf), _TRUNCATE, _u("%llu"), const_->f64_bits);
            break;
        default:
            Assert(UNREACHED);
            JavascriptError::ThrowTypeErrorVar(ctx->scriptContext, WABTERR_WabtError, _u("invalid constant type"));
        }
        write_string(ctx, constDescriptor, PropertyIds::value, buf);
    }
    return constsArr;
}

void write_const_vector(Context* ctx, Js::Var obj, PropertyId id, const ConstVector* consts)
{
    Js::Var constsArr = create_const_vector(ctx, consts);
    BOOL hr = JavascriptOperators::OP_SetProperty(obj, id, constsArr, ctx->scriptContext);
    if (!hr)
    {
        JavascriptError::ThrowTypeErrorVar(ctx->scriptContext, WABTERR_WabtError, _u("Unable to write const vector"));
    }
}

Js::Var create_type_object(Context* ctx, wabt::Type type)
{
    Js::Var typeObj = JavascriptOperators::NewJavascriptObjectNoArg(ctx->scriptContext);
    wabt::StringSlice str;
    str.start = get_type_name(type);
    str.length = strlen(str.start);
    write_string(ctx, typeObj, PropertyIds::type, str);
    return typeObj;
}

void write_action_result_type(Context* ctx, Js::Var obj, PropertyId id, Script* script, const Action* action)
{
    const Module* module = get_module_by_var(script, &action->module_var);
    const Export* export_;
    Js::Var resultArr = JavascriptOperators::NewJavascriptArrayNoArg(ctx->scriptContext);
    JavascriptOperators::OP_SetProperty(obj, id, resultArr, ctx->scriptContext);

    switch (action->type)
    {
    case ActionType::Invoke:
    {
        export_ = get_export_by_name(module, &action->invoke.name);
        assert(export_->kind == ExternalKind::Func);
        wabt::Func* func = get_func_by_var(module, &export_->var);
        int num_results = (int)get_num_results(func);
        int i;
        for (i = 0; i < num_results; ++i)
        {
            Js::Var typeObj = create_type_object(ctx, get_result_type(func, i));
            JavascriptArray::Push(ctx->scriptContext, resultArr, typeObj);
        }
        break;
    }

    case ActionType::Get:
    {
        export_ = get_export_by_name(module, &action->get.name);
        assert(export_->kind == ExternalKind::Global);
        wabt::Global* global = get_global_by_var(module, &export_->var);
        Js::Var typeObj = create_type_object(ctx, global->type);
        JavascriptArray::Push(ctx->scriptContext, resultArr, typeObj);
        break;
    }
    }
}

void write_action(Context* ctx, Js::Var obj, const Action* action)
{
    ScriptContext* scriptContext = ctx->scriptContext;
    Js::Var actionObj = JavascriptOperators::NewJavascriptObjectNoArg(scriptContext);
    JavascriptOperators::OP_SetProperty(obj, PropertyIds::action, actionObj, scriptContext);

    if (action->type == ActionType::Invoke)
    {
        write_string(ctx, actionObj, PropertyIds::type, _u("invoke"));
    }
    else
    {
        Assert(action->type == ActionType::Get);
        write_string(ctx, actionObj, PropertyIds::type, _u("get"));
    }

    if (action->module_var.type != wabt::VarType::Index)
    {
        write_var(ctx, actionObj, PropertyIds::module, &action->module_var);
    }
    if (action->type == ActionType::Invoke)
    {
        write_string(ctx, actionObj, PropertyIds::field, action->invoke.name);
        write_const_vector(ctx, actionObj, PropertyIds::args, &action->invoke.args);

    }
    else
    {
        write_string(ctx, actionObj, PropertyIds::field, action->get.name);
    }
}

ArrayBuffer* create_buffer(Context* ctx, const char* buf, size_t size)
{
    Assert(size < UINT32_MAX);
    ArrayBuffer* arrayBuffer = ctx->scriptContext->GetLibrary()->CreateArrayBuffer((uint32)size);
    js_memcpy_s(arrayBuffer->GetBuffer(), arrayBuffer->GetByteLength(), buf, (uint32)size);
    return arrayBuffer;
}

ArrayBuffer* create_module(Context* ctx, wabt::Module* module)
{
    ArenaAllocator* allocator = ctx->allocator;
    ScriptContext* scriptContext = ctx->scriptContext;
    if (!module)
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, WABTERR_WabtError, _u("No module found"));
    }
    MemoryWriter writer;
    MemoryWriterContext context{ &writer, allocator };
    writer.base.move_data = move_data_in_output_buffer;
    writer.base.write_data = write_data_to_output_buffer;
    writer.base.user_data = &context;
    writer.buf.size = 0;
    size_t capacity = writer.buf.capacity = 256;
    writer.buf.start = AnewArrayZ(allocator, char, capacity);
    wabt::WriteBinaryOptions s_write_binary_options = { nullptr, true, false, false };
    Result result = write_binary_module(&writer.base, module, &s_write_binary_options);
    if (!WABT_SUCCEEDED(result))
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, WABTERR_WabtError, _u("Error while writing module"));
    }
    return create_buffer(ctx, writer.buf.start, writer.buf.size);
}

void write_module(Context* ctx, Js::Var obj, wabt::Module* module)
{
    Js::Var buffer = create_module(ctx, module);
    JavascriptOperators::OP_SetProperty(obj, PropertyIds::buffer, buffer, ctx->scriptContext);
}

static void write_invalid_module(Context* ctx, Js::Var obj, const RawModule* module, StringSlice text)
{
    write_location(ctx, obj, get_raw_module_location(module));
    write_string(ctx, obj, PropertyIds::text, text);
    if (module->type == RawModuleType::Text)
    {
        write_module(ctx, obj, module->text);
        return;
    }
    Assert(module->type == RawModuleType::Binary);
    Js::Var buffer = create_buffer(ctx, module->binary.data, module->binary.size);
    JavascriptOperators::OP_SetProperty(obj, PropertyIds::buffer, buffer, ctx->scriptContext);
}

Js::Var write_commands(Context* ctx, Script* script) {
    ScriptContext* scriptContext = ctx->scriptContext;
    Js::Var resultObj = JavascriptOperators::NewJavascriptObjectNoArg(scriptContext);
    Js::Var commandsArr = JavascriptOperators::NewJavascriptArrayNoArg(scriptContext);
    JavascriptOperators::OP_SetProperty(resultObj, PropertyIds::commands, commandsArr, scriptContext);
    size_t i;
    int last_module_index = -1;
    for (i = 0; i < script->commands.size; ++i) {
        Command* command = &script->commands.data[i];

        if (command->type == CommandType::AssertInvalidNonBinary)
            continue;
        Js::Var cmdObj = JavascriptOperators::NewJavascriptObjectNoArg(scriptContext);
        JavascriptArray::Push(scriptContext, commandsArr, cmdObj);
        write_command_type(ctx, command, cmdObj);

        switch (command->type) {
        case CommandType::Module: {
            wabt::Module* module = &command->module;
            if (module->name.start) {
                Js::Var name = ToJavascriptString(ctx, module->name);
                JavascriptOperators::OP_SetProperty(cmdObj, PropertyIds::name, name, scriptContext);
            }
            write_module(ctx, cmdObj, module);
            last_module_index = static_cast<int>(i);
            break;
        }

        case CommandType::Action:
            write_location(ctx, cmdObj, &command->action.loc);
            write_action(ctx, cmdObj, &command->action);
            break;

        case CommandType::Register:
            write_location(ctx, cmdObj, &command->register_.var.loc);
            if (command->register_.var.type == wabt::VarType::Name) {
                write_var(ctx, cmdObj, PropertyIds::name, &command->register_.var);
            } else {
                /* If we're not registering by name, then we should only be
                * registering the last module. */
                WABT_USE(last_module_index);
                Assert(command->register_.var.index == last_module_index);
            }
            write_string(ctx, cmdObj, PropertyIds::as, command->register_.module_name);
            break;

        case CommandType::AssertMalformed:
            write_invalid_module(ctx, cmdObj, &command->assert_malformed.module,
                                 command->assert_malformed.text);
            break;

        case CommandType::AssertInvalid:
            write_invalid_module(ctx, cmdObj, &command->assert_invalid.module,
                                 command->assert_invalid.text);
            break;

        case CommandType::AssertUnlinkable:
            write_invalid_module(ctx, cmdObj, &command->assert_unlinkable.module,
                                 command->assert_unlinkable.text);
            break;

        case CommandType::AssertUninstantiable:
            write_invalid_module(ctx, cmdObj, &command->assert_uninstantiable.module,
                                 command->assert_uninstantiable.text);
            break;

        case CommandType::AssertReturn:
            write_location(ctx, cmdObj, &command->assert_return.action.loc);
            write_action(ctx, cmdObj, &command->assert_return.action);
            write_const_vector(ctx, cmdObj, PropertyIds::expected, &command->assert_return.expected);
            break;

        case CommandType::AssertReturnNan:
            write_location(ctx, cmdObj, &command->assert_return_nan.action.loc);
            write_action(ctx, cmdObj, &command->assert_return_nan.action);
            write_action_result_type(ctx, cmdObj, PropertyIds::expected, script,
                                     &command->assert_return_nan.action);
            break;

        case CommandType::AssertTrap:
            write_location(ctx, cmdObj, &command->assert_trap.action.loc);
            write_action(ctx, cmdObj, &command->assert_trap.action);
            write_string(ctx, cmdObj, PropertyIds::text, command->assert_trap.text);
            break;

        case CommandType::AssertExhaustion:
            write_location(ctx, cmdObj, &command->assert_trap.action.loc);
            write_action(ctx, cmdObj, &command->assert_trap.action);
            break;

        case CommandType::AssertInvalidNonBinary:
            assert(0);
            break;
        }
    }
    return resultObj;
}


Js::Var WabtInterface::EntryConvertWast2Wasm(RecyclableObject* function, CallInfo callInfo, ...)
{
    ScriptContext* scriptContext = function->GetScriptContext();
    Js::Var returnValue = scriptContext->GetLibrary()->GetUndefined();
    PROBE_STACK(function->GetScriptContext(), Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

    Assert(!(callInfo.Flags & CallFlags_New));

    if (args.Info.Count < 2 || !JavascriptString::Is(args[1]))
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedBufferSource);
    }
    bool isSpecText = false;
    if (args.Info.Count > 2)
    {
        // optional config object
        if (!JavascriptOperators::IsObject(args[2]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject, _u("config"));
        }
        DynamicObject * configObject = JavascriptObject::FromVar(args[2]);

        Js::Var isSpecVar = JavascriptOperators::OP_GetProperty(configObject, PropertyIds::spec, scriptContext);
        isSpecText = JavascriptConversion::ToBool(isSpecVar, scriptContext);
    }
    JavascriptString* string = (JavascriptString*)args[1];
    const char16* str = string->GetString();
    ArenaAllocator arena(_u("Wast2Wasm"), scriptContext->GetThreadContext()->GetPageAllocator(), Throw::OutOfMemory);
    Context context;
    context.allocator = &arena;
    context.scriptContext = scriptContext;

    size_t origSize = string->GetLength();
    size_t wastSize;
    char* wastBuffer = nullptr;
    auto allocator = [&arena](size_t size) {return (utf8char_t*)AnewArray(&arena, byte, size);};
    utf8::WideStringToNarrow(allocator, str, origSize, &wastBuffer, &wastSize);

    AstLexer* lexer = new_ast_buffer_lexer("", wastBuffer, wastSize);
    AutoCleanLexer autoCleanLexer = { lexer };
    Unused(autoCleanLexer);

    wabt::SourceErrorHandler s_error_handler = {
        lexer_error_callback,
        120,
        &context
    };
    bool s_validate = true;

    wabt::Script script;
    wabt::Result result = wabt::parse_ast(lexer, &script, &s_error_handler);
    AutoCleanScript autoCleanScript = { &script };
    Unused(autoCleanScript);

    if (WABT_SUCCEEDED(result))
    {
        result = wabt::resolve_names_script(lexer, &script, &s_error_handler);

        if (WABT_SUCCEEDED(result) && s_validate)
            result = wabt::validate_script(lexer, &script, &s_error_handler);

        if (WABT_SUCCEEDED(result))
        {
            if (isSpecText)
            {
                returnValue = write_commands(&context, &script);
            }
            else
            {
                Module* module = get_first_module(&script);
                returnValue = create_module(&context, module);
            }
        }
    }
    return returnValue;
}
}
#endif // ENABLE_WABT

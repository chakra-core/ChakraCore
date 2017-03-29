//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "wabtapi.h"
#include "writer.h"
#include "ast-lexer.h"
#include "ast.h"
#include "ast-parser.h"
#include "resolve-names.h"
#include "binary-writer.h"

using namespace wabt;
using namespace ChakraWabt;

struct MemoryWriterContext
{
    MemoryWriter* writer;
    Context* ctx;
};

struct AutoCleanLexer
{
    AstLexer* lexer;
    ~AutoCleanLexer()
    {
        destroy_ast_lexer(lexer);
    }
};

struct AutoCleanScript
{
    Script* script;
    ~AutoCleanScript()
    {
        delete script;
    }
};

uint TruncSizeT(size_t value)
{
    if (value > 0xffffffff)
    {
        throw Error("Out of Memory");
    }
    return (uint)value;
}

void ensure_output_buffer_capacity(OutputBuffer* buf, size_t ensure_capacity, MemoryWriterContext* context)
{
    if (ensure_capacity > buf->capacity)
    {
        assert(buf->capacity != 0);
        size_t newCapacity = buf->capacity * 2;
        while (newCapacity < ensure_capacity)
            newCapacity *= 2;
        char* new_data = (char*)context->ctx->Allocate(TruncSizeT(newCapacity));
        memcpy(new_data, buf->start, buf->capacity);
        buf->start = new_data;
        buf->capacity = newCapacity;
    }
}

Result write_data_to_output_buffer(size_t offset,
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

Result move_data_in_output_buffer(size_t dst_offset,
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

void set_property(Context* ctx, Js::Var obj, PropertyId id, Js::Var value, const char* messageIfFailed)
{
    bool success = ctx->spec->setProperty(obj, id, value, ctx->user_data);
    if (!success)
    {
        throw Error(messageIfFailed);
    }
}

void write_int32(Context* ctx, Js::Var obj, PropertyId id, int32 value)
{
    Js::Var line = ctx->spec->int32ToVar(value, ctx->user_data);
    set_property(ctx, obj, id, line, "Unable to write number");
}

void write_int64(Context* ctx, Js::Var obj, PropertyId id, int64 value)
{
    Js::Var line = ctx->spec->int64ToVar(value, ctx->user_data);
    set_property(ctx, obj, id, line, "Unable to write number");
}

void write_string(Context* ctx, Js::Var obj, PropertyId id, StringSlice src)
{
    Js::Var str = ctx->spec->stringToVar(src.start, TruncSizeT(src.length), ctx->user_data);
    set_property(ctx, obj, id, str, "Unable to write string");
}

void write_string(Context* ctx, Js::Var obj, PropertyId id, const char* src)
{
    write_string(ctx, obj, id, {src, strlen(src)});
}

void write_location(Context* ctx, Js::Var obj, const Location* loc)
{
    write_int32(ctx, obj, PropertyIds::line, loc->line);
}

void write_var(Context* ctx, Js::Var obj, PropertyId id, const Var* var)
{
    if (var->type == VarType::Index)
    {
        write_int64(ctx, obj, id, var->index);
    }
    else
    {
        assert(var->type == VarType::Name);
        write_string(ctx, obj, id, var->name);
    }
}

void write_command_type(Context* ctx, CommandType type, Js::Var cmdObj)
{
    static const char* s_command_names[] = {
        "module",
        "action",
        "register",
        "assert_malformed",
        "assert_invalid",
        nullptr, /* ASSERT_INVALID_NON_BINARY, this command will never be
                 written */
        "assert_unlinkable",
        "assert_uninstantiable",
        "assert_return",
        "assert_return_canonical_nan",
        "assert_return_arithmetic_nan",
        "assert_trap",
        "assert_exhaustion",
    };
    WABT_STATIC_ASSERT(sizeof(s_command_names)/sizeof(char*) == (int)CommandType::Last + 1);
    uint i = (uint)type;
    if (i > (uint)CommandType::Last)
    {
        throw Error("invalid command type");
    }
    write_string(ctx, cmdObj, PropertyIds::type, s_command_names[i]);
}

Js::Var create_const_vector(Context* ctx, const ConstVector* consts)
{
    Js::Var constsArr = ctx->spec->createArray(ctx->user_data);

    size_t i;
    for (i = 0; i < consts->size(); ++i)
    {
        Js::Var constDescriptor = ctx->spec->createObject(ctx->user_data);
        ctx->spec->push(constsArr, constDescriptor, ctx->user_data);

        char buf[32];
        const Const& const_ = consts->at(i);
        switch (const_.type)
        {
        case Type::I32:
            write_string(ctx, constDescriptor, PropertyIds::type, "i32");
            wabt_snprintf(buf, 32, "%u", const_.u32);
            break;
        case Type::I64:
            write_string(ctx, constDescriptor, PropertyIds::type, "i64");
            wabt_snprintf(buf, 32, "%llu", const_.u64);
            break;
        case Type::F32:
            write_string(ctx, constDescriptor, PropertyIds::type, "f32");
            wabt_snprintf(buf, 32, "%u", const_.f32_bits);
            break;
        case Type::F64:
            write_string(ctx, constDescriptor, PropertyIds::type, "f64");
            wabt_snprintf(buf, 32, "%llu", const_.f64_bits);
            break;
        default:
            assert(0);
            throw Error("invalid constant type");
        }
        write_string(ctx, constDescriptor, PropertyIds::value, buf);
    }
    return constsArr;
}

void write_const_vector(Context* ctx, Js::Var obj, PropertyId id, const ConstVector* consts)
{
    Js::Var constsArr = create_const_vector(ctx, consts);
    set_property(ctx, obj, id, constsArr, "Unable to write const vector");
}

Js::Var create_type_object(Context* ctx, Type type)
{
    Js::Var typeObj = ctx->spec->createObject(ctx->user_data);
    StringSlice str;
    str.start = get_type_name(type);
    str.length = strlen(str.start);
    write_string(ctx, typeObj, PropertyIds::type, str);
    return typeObj;
}

void write_action_result_type(Context* ctx, Js::Var obj, PropertyId id, Script* script, const Action* action)
{
    const Module* module = get_module_by_var(script, &action->module_var);
    const Export* export_;
    Js::Var resultArr = ctx->spec->createArray(ctx->user_data);
    set_property(ctx, obj, id, resultArr, "Unable to set action result type");

    switch (action->type)
    {
    case ActionType::Invoke:
    {
        export_ = get_export_by_name(module, &action->name);
        assert(export_->kind == ExternalKind::Func);
        Func* func = get_func_by_var(module, &export_->var);
        int num_results = (int)get_num_results(func);
        int i;
        for (i = 0; i < num_results; ++i)
        {
            Js::Var typeObj = create_type_object(ctx, get_result_type(func, i));
            ctx->spec->push(resultArr, typeObj, ctx->user_data);
        }
        break;
    }

    case ActionType::Get:
    {
        export_ = get_export_by_name(module, &action->name);
        assert(export_->kind == ExternalKind::Global);
        Global* global = get_global_by_var(module, &export_->var);
        Js::Var typeObj = create_type_object(ctx, global->type);
        ctx->spec->push(resultArr, typeObj, ctx->user_data);
        break;
    }
    }
}

void write_action(Context* ctx, Js::Var obj, const Action* action)
{
    Js::Var actionObj = ctx->spec->createObject(ctx->user_data);
    set_property(ctx, obj, PropertyIds::action, actionObj, "Unable to set action");

    if (action->type == ActionType::Invoke)
    {
        write_string(ctx, actionObj, PropertyIds::type, "invoke");
    }
    else
    {
        assert(action->type == ActionType::Get);
        write_string(ctx, actionObj, PropertyIds::type, "get");
    }

    if (action->module_var.type != VarType::Index)
    {
        write_var(ctx, actionObj, PropertyIds::module, &action->module_var);
    }
    if (action->type == ActionType::Invoke)
    {
        write_string(ctx, actionObj, PropertyIds::field, action->name);
        write_const_vector(ctx, actionObj, PropertyIds::args, &action->invoke->args);

    }
    else
    {
        write_string(ctx, actionObj, PropertyIds::field, action->name);
    }
}

Js::Var create_module(Context* ctx, Module* module)
{
    if (!module)
    {
        throw Error("No module found");
    }
    MemoryWriter writer;
    MemoryWriterContext context{ &writer, ctx };
    writer.base.move_data = move_data_in_output_buffer;
    writer.base.write_data = write_data_to_output_buffer;
    writer.base.user_data = &context;
    writer.buf.size = 0;
    size_t capacity = writer.buf.capacity = 256;
    writer.buf.start = (char*)ctx->Allocate(TruncSizeT(capacity));
    WriteBinaryOptions s_write_binary_options = { nullptr, true, false, false };
    Result result = write_binary_module(&writer.base, module, &s_write_binary_options);
    if (!WABT_SUCCEEDED(result))
    {
        throw Error("Error while writing module");
    }
    return ctx->createBuffer(writer.buf.start, TruncSizeT(writer.buf.size), ctx->user_data);
}

void write_module(Context* ctx, Js::Var obj, Module* module)
{
    Js::Var buffer = create_module(ctx, module);
    set_property(ctx, obj, PropertyIds::buffer, buffer, "Unable to set module");
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
    assert(module->type == RawModuleType::Binary);
    Js::Var buffer = ctx->createBuffer(module->binary.data, TruncSizeT(module->binary.size), ctx->user_data);
    set_property(ctx, obj, PropertyIds::buffer, buffer, "Unable to set invalid module");
}

Js::Var write_commands(Context* ctx, Script* script) {

    Js::Var resultObj = ctx->spec->createObject(ctx->user_data);
    Js::Var commandsArr = ctx->spec->createArray(ctx->user_data);
    set_property(ctx, resultObj, PropertyIds::commands, commandsArr, "Unable to set commands");
    size_t i;
    int last_module_index = -1;
    for (i = 0; i < script->commands.size(); ++i) {
        const Command* command = script->commands[i].get();

        if (command->type == CommandType::AssertInvalidNonBinary)
            continue;
        Js::Var cmdObj = ctx->spec->createObject(ctx->user_data);
        ctx->spec->push(commandsArr, cmdObj, ctx->user_data);
        write_command_type(ctx, command->type, cmdObj);

        switch (command->type) {
        case CommandType::Module: {
            Module* module = command->module;
            write_location(ctx, cmdObj, &module->loc);
            if (module->name.start) {
                write_string(ctx, cmdObj, PropertyIds::name, module->name);
            }
            write_module(ctx, cmdObj, module);
            last_module_index = static_cast<int>(i);
            break;
        }

        case CommandType::Action:
            write_location(ctx, cmdObj, &command->action->loc);
            write_action(ctx, cmdObj, command->action);
            break;

        case CommandType::Register:
            write_location(ctx, cmdObj, &command->register_.var.loc);
            if (command->register_.var.type == VarType::Name) {
                write_var(ctx, cmdObj, PropertyIds::name, &command->register_.var);
            } else {
                /* If we're not registering by name, then we should only be
                * registering the last module. */
                WABT_USE(last_module_index);
                assert(command->register_.var.index == last_module_index);
            }
            write_string(ctx, cmdObj, PropertyIds::as, command->register_.module_name);
            break;

        case CommandType::AssertMalformed:
            write_invalid_module(ctx, cmdObj, command->assert_malformed.module,
                                 command->assert_malformed.text);
            break;

        case CommandType::AssertInvalid:
            write_invalid_module(ctx, cmdObj, command->assert_invalid.module,
                                 command->assert_invalid.text);
            break;

        case CommandType::AssertUnlinkable:
            write_invalid_module(ctx, cmdObj, command->assert_unlinkable.module,
                                 command->assert_unlinkable.text);
            break;

        case CommandType::AssertUninstantiable:
            write_invalid_module(ctx, cmdObj, command->assert_uninstantiable.module,
                                 command->assert_uninstantiable.text);
            break;

        case CommandType::AssertReturn:
            write_location(ctx, cmdObj, &command->assert_return.action->loc);
            write_action(ctx, cmdObj, command->assert_return.action);
            write_const_vector(ctx, cmdObj, PropertyIds::expected, command->assert_return.expected);
            break;

        case CommandType::AssertReturnCanonicalNan:
        case CommandType::AssertReturnArithmeticNan:
            WABT_STATIC_ASSERT(offsetof(Command, assert_return_canonical_nan) == offsetof(Command, assert_return_arithmetic_nan));
            write_location(ctx, cmdObj, &command->assert_return_canonical_nan.action->loc);
            write_action(ctx, cmdObj, command->assert_return_canonical_nan.action);
            write_action_result_type(ctx, cmdObj, PropertyIds::expected, script,
                                     command->assert_return_canonical_nan.action);
            break;

        case CommandType::AssertTrap:
            write_location(ctx, cmdObj, &command->assert_trap.action->loc);
            write_action(ctx, cmdObj, command->assert_trap.action);
            write_string(ctx, cmdObj, PropertyIds::text, command->assert_trap.text);
            break;

        case CommandType::AssertExhaustion:
            write_location(ctx, cmdObj, &command->assert_trap.action->loc);
            write_action(ctx, cmdObj, command->assert_trap.action);
            break;

        case CommandType::AssertInvalidNonBinary:
            assert(0);
            break;
        }
    }
    return resultObj;
}

bool lexer_error_callback(const Location* loc,
                          const char* error,
                          const char* source_line,
                          size_t,
                          size_t source_line_column_offset,
                          void*)
{
    int colStart = loc->first_column - 1 - (int)source_line_column_offset;
    int sourceErrorLength = (loc->last_column - loc->first_column) - 2;
    if (sourceErrorLength < 0)
    {
        // -2 probably overflowed
        sourceErrorLength = 0;
    }
    char buf[4096];
    wabt_snprintf(buf, 4096, "Wast Parsing error:%u:%u:\n%s\n%s\n%*s^%*s^",
                loc->line,
                loc->first_column,
                error,
                source_line,
                colStart, "",
                sourceErrorLength, "");
    throw Error(buf);
};

void Context::Validate(bool isSpec) const
{
    if (!allocator) throw Error("Missing allocator");
    if (!createBuffer) throw Error("Missing createBuffer");
    if (isSpec)
    {
        if (!spec) throw Error("Missing Spec context");
        if (!spec->setProperty) throw Error("Missing spec->setProperty");
        if (!spec->int32ToVar) throw Error("Missing spec->int32ToVar");
        if (!spec->int64ToVar) throw Error("Missing spec->int64ToVar");
        if (!spec->stringToVar) throw Error("Missing spec->stringToVar");
        if (!spec->createObject) throw Error("Missing spec->createObject");
        if (!spec->createArray) throw Error("Missing spec->createArray");
        if (!spec->push) throw Error("Missing spec->push");
    }
}

Js::Var ChakraWabt::ConvertWast2Wasm(Context& ctx, char* buffer, uint bufferSize, bool isSpecText)
{
    ctx.Validate(isSpecText);

    AstLexer* lexer = new_ast_buffer_lexer("", buffer, (size_t)bufferSize);
    AutoCleanLexer autoCleanLexer = { lexer };

    SourceErrorHandler s_error_handler = {
        lexer_error_callback,
        256,
        nullptr
    };

    Script* script;
    Result result = parse_ast(lexer, &script, &s_error_handler);
    AutoCleanScript autoCleanScript = { script };
    if (!WABT_SUCCEEDED(result))
    {
        throw Error("Invalid wast script");
    }

    result = resolve_names_script(lexer, script, &s_error_handler);
    if (!WABT_SUCCEEDED(result))
    {
        throw Error("Unable to resolve script's names");
    }

    void* returnValue = nullptr;
    if (isSpecText)
    {
        returnValue = write_commands(&ctx, script);
    }
    else
    {
        Module* module = get_first_module(script);
        returnValue = create_module(&ctx, module);
    }
    return returnValue;
}

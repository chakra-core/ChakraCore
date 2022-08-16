//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

#ifdef ENABLE_WABT
#include "../WasmReader/WasmReaderPch.h"
#include "wabtapi.h"
#include "Codex/Utf8Helper.h"

namespace Js
{

struct Context
{
    ArenaAllocator* allocator;
    ScriptContext* scriptContext;
};

char16* NarrowStringToWide(Context* ctx, const char* src, const size_t* srcSize = nullptr, charcount_t* dstSize = nullptr)
{
    auto allocator = [&ctx](size_t size) {return (char16*)AnewArray(ctx->allocator, char16, size); };
    char16* dst = nullptr;
    charcount_t size;
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

static PropertyId propertyMap[] = {
    Js::PropertyIds::as,
    Js::PropertyIds::action,
    Js::PropertyIds::args,
    Js::PropertyIds::buffer,
    Js::PropertyIds::commands,
    Js::PropertyIds::expected,
    Js::PropertyIds::field,
    Js::PropertyIds::line,
    Js::PropertyIds::name,
    Js::PropertyIds::module,
    Js::PropertyIds::text,
    Js::PropertyIds::type,
    Js::PropertyIds::value,
};

bool SetProperty(Js::Var obj, PropertyId id, Js::Var value, void* user_data)
{
    CompileAssert((sizeof(propertyMap)/sizeof(PropertyId)) == ChakraWabt::PropertyIds::COUNT);
    Context* ctx = (Context*)user_data;
    Assert(id < ChakraWabt::PropertyIds::COUNT);
    return !!JavascriptOperators::OP_SetProperty(obj, propertyMap[id], value, ctx->scriptContext);
}
Js::Var CreateObject(void* user_data)
{
    Context* ctx = (Context*)user_data;
    return JavascriptOperators::NewJavascriptObjectNoArg(ctx->scriptContext);
}
Js::Var CreateArray(void* user_data)
{
    Context* ctx = (Context*)user_data;
    return JavascriptOperators::NewJavascriptArrayNoArg(ctx->scriptContext);
}
void Push(Js::Var arr, Js::Var obj, void* user_data)
{
    Context* ctx = (Context*)user_data;
    JavascriptArray::Push(ctx->scriptContext, arr, obj);
}
Js::Var Int32ToVar(int32 value, void* user_data)
{
    Context* ctx = (Context*)user_data;
    return JavascriptNumber::ToVar(value, ctx->scriptContext);
}
Js::Var Int64ToVar(int64 value, void* user_data)
{
    Context* ctx = (Context*)user_data;
    return JavascriptNumber::ToVar(value, ctx->scriptContext);
}
Js::Var StringToVar(const char* src, uint length, void* user_data)
{
    Context* ctx = (Context*)user_data;
    charcount_t bufSize = 0;
    size_t slength = (size_t)length;
    char16* buf = NarrowStringToWide(ctx, src, &slength, &bufSize);
    Assert(bufSize < UINT32_MAX);
    return JavascriptString::NewCopyBuffer(buf, bufSize, ctx->scriptContext);
}

Js::Var CreateBuffer(const uint8* buf, uint size, void* user_data)
{
    Context* ctx = (Context*)user_data;
    ArrayBuffer* arrayBuffer = ctx->scriptContext->GetLibrary()->CreateArrayBuffer(size);
    js_memcpy_s(arrayBuffer->GetBuffer(), arrayBuffer->GetByteLength(), buf, size);
    return arrayBuffer;
}

Js::Var WabtInterface::EntryConvertWast2Wasm(RecyclableObject* function, CallInfo callInfo, ...)
{
    ScriptContext* scriptContext = function->GetScriptContext();
    PROBE_STACK(function->GetScriptContext(), Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

    Assert(!(callInfo.Flags & CallFlags_New));

    if (args.Info.Count < 2 || !VarIs<JavascriptString>(args[1]))
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
        DynamicObject * configObject = VarTo<DynamicObject>(args[2]);

        Js::Var isSpecVar = JavascriptOperators::OP_GetProperty(configObject, PropertyIds::spec, scriptContext);
        isSpecText = JavascriptConversion::ToBool(isSpecVar, scriptContext);
    }

    ArenaAllocator arena(_u("Wast2Wasm"), scriptContext->GetThreadContext()->GetPageAllocator(), Throw::OutOfMemory);
    Context context;
    size_t wastSize;
    char* wastBuffer = nullptr;

    ENTER_PINNED_SCOPE(JavascriptString, string);
    string = (JavascriptString*)args[1];
    const char16* str = string->GetString();
    context.allocator = &arena;
    context.scriptContext = scriptContext;

    size_t origSize = string->GetLength();
    auto allocator = [&arena](size_t size) {return (utf8char_t*)AnewArray(&arena, byte, size);};
    utf8::WideStringToNarrow(allocator, str, origSize, &wastBuffer, &wastSize);
    LEAVE_PINNED_SCOPE();   //  string

    try
    {
        ChakraWabt::SpecContext spec;
        ChakraWabt::ChakraContext wabtCtx;
        wabtCtx.user_data = &context;
        wabtCtx.createBuffer = CreateBuffer;
        wabtCtx.features.sign_extends = CONFIG_FLAG(WasmSignExtends);
        wabtCtx.features.threads = Wasm::Threads::IsEnabled();
        wabtCtx.features.simd = Wasm::Simd::IsEnabled();
        wabtCtx.features.sat_float_to_int = Wasm::WasmNontrapping::IsEnabled();
        if (isSpecText)
        {
            wabtCtx.spec = &spec;
            spec.setProperty = SetProperty;
            spec.int32ToVar = Int32ToVar;
            spec.int64ToVar = Int64ToVar;
            spec.stringToVar = StringToVar;
            spec.createObject = CreateObject;
            spec.createArray = CreateArray;
            spec.push = Push;
        }
        void* result = ChakraWabt::ConvertWast2Wasm(wabtCtx, wastBuffer, (uint)wastSize, isSpecText);
        if (result == nullptr)
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }
        return result;
    }
    catch (ChakraWabt::WabtAPIError& e)
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, WABTERR_WabtError, NarrowStringToWide(&context, e.message));
    }
}
}
#endif // ENABLE_WABT

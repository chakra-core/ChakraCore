//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

// TODO: merge with WasmModule
namespace Js
{
class WebAssemblyModule : public DynamicObject
{
protected:
    WebAssemblyModule(Wasm::WasmModule * wasmModule, DynamicType * type);

public:

    class EntryInfo
    {
    public:
        static FunctionInfo NewInstance;
    };

    static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);

    static bool Is(Var aValue);
    static WebAssemblyModule * FromVar(Var aValue);

    static Wasm::WasmModule * CompileModule(
        ScriptContext* scriptContext,
        const char16* script,
        SRCINFO const * pSrcInfo,
        CompileScriptException * pse,
        Utf8SourceInfo** ppSourceInfo,
        const uint lengthBytes,
        bool validateOnly = false,
        Var bufferSrc = nullptr);

private:
    static char16* lastWasmExceptionMessage;
    Wasm::WasmModule * module;
};

} // namespace Js

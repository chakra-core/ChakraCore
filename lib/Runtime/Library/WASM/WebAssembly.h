//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{

class WebAssembly
{
#ifdef ENABLE_WASM
public:
    // WebAssembly spec internal definition for page size
    static const uint PageSize = 64 * 1024;

    class EntryInfo
    {
    public:
        static FunctionInfo Compile;
        static FunctionInfo CompileStreaming;
        static FunctionInfo Validate;
        static FunctionInfo Instantiate;
        static FunctionInfo InstantiateStreaming;
        static FunctionInfo InstantiateBound;
        static FunctionInfo QueryResponse;
    };
    static Var EntryCompile(RecyclableObject* function, CallInfo callInfo, ...);
    static Var EntryCompileStreaming(RecyclableObject* function, CallInfo callInfo, ...);
    static Var EntryValidate(RecyclableObject* function, CallInfo callInfo, ...);
    static Var EntryInstantiate(RecyclableObject* function, CallInfo callInfo, ...);
    static Var EntryInstantiateStreaming(RecyclableObject* function, CallInfo callInfo, ...);
    // The import object is the first argument, then the buffer source
    static Var EntryInstantiateBound(RecyclableObject* function, CallInfo callInfo, ...);
    static Var EntryQueryResponse(RecyclableObject* function, CallInfo callInfo, ...);

    static uint32 ToNonWrappingUint32(Var val, ScriptContext * ctx);
    static void CheckSignature(ScriptContext * scriptContext, Wasm::WasmSignature * sig1, Wasm::WasmSignature * sig2);
    static uint GetSignatureSize();

private:
    static bool IsResponseObject(Var responseObject, ScriptContext* scriptContext);
    static Var TryResolveResponse(RecyclableObject* function, Var thisArg, Var responseArg);
#endif
};

}

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
        static FunctionInfo Validate;
        static FunctionInfo NativeTypeCallTest;
    };
    static Var EntryCompile(RecyclableObject* function, CallInfo callInfo, ...);
    static Var EntryValidate(RecyclableObject* function, CallInfo callInfo, ...);
    static Var EntryNativeTypeCallTest(RecyclableObject* function, CallInfo callInfo, ...);

    static uint32 ToNonWrappingUint32(Var val, ScriptContext * ctx);
    static void ReadBufferSource(Var val, ScriptContext * ctx, _Out_ BYTE** buffer, _Out_ uint *byteLength);
    static void CheckSignature(ScriptContext * scriptContext, Wasm::WasmSignature * sig1, Wasm::WasmSignature * sig2);
    static uint GetSignatureSize();
#endif
};

}

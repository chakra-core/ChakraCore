//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class WasmLibrary
    {
    public:
        static JavascriptMethod WasmDeferredParseEntryPoint(AsmJsScriptFunction** funcPtr, int internalCall);
#ifdef ENABLE_WASM
        class EntryInfo
        {
        public:
            static FunctionInfo instantiateModule;
            static FunctionInfo Compile;
            static FunctionInfo Validate;
        };

        static Var instantiateModule(RecyclableObject* function, CallInfo callInfo, ...);
        static const unsigned int experimentalVersion;

        static Var WasmLazyTrapCallback(RecyclableObject *callee, CallInfo, ...);
        static Var WasmDeferredParseInternalThunk(RecyclableObject* function, CallInfo callInfo, ...);
        static Var WasmDeferredParseExternalThunk(RecyclableObject* function, CallInfo callInfo, ...);

#endif
    };
}
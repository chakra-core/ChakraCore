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
        static JavascriptMethod EnsureWasmEntrypoint(ScriptFunction* funcPtr);
        static void ResetFunctionBodyDefaultEntryPoint(FunctionBody* body);
#ifdef ENABLE_WASM
        static Var WasmLazyTrapCallback(RecyclableObject *callee, CallInfo, ...);
#endif
    };
}

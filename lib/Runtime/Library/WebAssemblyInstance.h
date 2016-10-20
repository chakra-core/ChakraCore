//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Js
{
    class WebAssemblyInstance : public DynamicObject
    {
    public:
        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
        };

        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);

        static bool Is(Var aValue);
        static WebAssemblyInstance * FromVar(Var aValue);

        static WebAssemblyInstance * CreateInstance(WebAssemblyModule * module, Var importObject);
    private:
        WebAssemblyInstance(WebAssemblyModule * wasmModule, DynamicType * type);

        static void LoadDataSegs(WebAssemblyModule * wasmModule, Var* heap, ScriptContext* ctx);
        static void LoadFunctions(WebAssemblyModule * wasmModule, ScriptContext* ctx, Var* moduleMemoryPtr, Var* localModuleFunctions);
        static void BuildObject(WebAssemblyModule * wasmModule, ScriptContext* ctx, Var exportsNamespace, Var* heap, Var exportObj, Var* localModuleFunctions, Var* importFunctions);
        static void LoadImports(WebAssemblyModule * wasmModule, ScriptContext* ctx, Var* importFunctions, Var moduleEnv, Var ffi);
        static void LoadGlobals(WebAssemblyModule * wasmModule, ScriptContext* ctx, Var moduleEnv, Var ffi);
        static void LoadIndirectFunctionTables(WebAssemblyModule * wasmModule, ScriptContext* ctx, Var** indirectFunctionTables, Var* localModuleFunctions, Var* importFunctions);
        static Var GetFunctionObjFromFunctionIndex(WebAssemblyModule * wasmModule, ScriptContext* ctx, uint32 funcIndex, Var* localModuleFunctions, Var* importFunctions);


        WebAssemblyModule * m_module;
    };

} // namespace Js

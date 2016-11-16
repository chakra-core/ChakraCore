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

        struct WebAssemblyEnvironment
        {
            WebAssemblyEnvironment(WebAssemblyModule* module);
            Var* ptr;
            Var* memory;
            Var* imports;
            Var* functions;
            Var* table;
            Var* globals;
        };

        static void LoadDataSegs(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment env);
        static void LoadFunctions(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment env);
        static Var  BuildObject(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment env);
        static void LoadImports(WebAssemblyModule * wasmModule, ScriptContext* ctx, Var ffi, WebAssemblyEnvironment env);
        static void LoadGlobals(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment env);
        static void LoadIndirectFunctionTable(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment env);
        static Var GetFunctionObjFromFunctionIndex(WebAssemblyModule * wasmModule, ScriptContext* ctx, uint32 funcIndex, WebAssemblyEnvironment env);


        WebAssemblyModule * m_module;
    };

} // namespace Js

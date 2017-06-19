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
            static FunctionInfo GetterExports;
        };

        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);
        static Var GetterExports(RecyclableObject* function, CallInfo callInfo, ...);

        static bool Is(Var aValue);
        static WebAssemblyInstance * FromVar(Var aValue);

        static WebAssemblyInstance * CreateInstance(WebAssemblyModule * module, Var importObject);
    private:
        WebAssemblyInstance(WebAssemblyModule * wasmModule, DynamicType * type);

        static void LoadDataSegs(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment* env);
        static void LoadFunctions(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment* env);
        static Var  BuildObject(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment* env);
        static void LoadImports(WebAssemblyModule * wasmModule, ScriptContext* ctx, Var ffi, WebAssemblyEnvironment* env);
        static void LoadGlobals(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment* env);
        static void LoadIndirectFunctionTable(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment* env);
        static void ValidateTableAndMemory(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment* env);

        Field(WebAssemblyModule *) m_module;
        Field(Js::Var) m_exports;
    };

} // namespace Js

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
#ifdef ENABLE_WASM

namespace Js
{
    class WebAssemblyInstance : public DynamicObject
    {
    protected:
        DEFINE_VTABLE_CTOR(WebAssemblyInstance, DynamicObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(WebAssemblyInstance);

    public:
        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
            static FunctionInfo GetterExports;
        };

        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);
        static Var GetterExports(RecyclableObject* function, CallInfo callInfo, ...);

        static WebAssemblyInstance * CreateInstance(WebAssemblyModule * module, Var importObject);
    private:
        WebAssemblyInstance(WebAssemblyModule * wasmModule, DynamicType * type);

        static void InitializeDataSegs(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment* env);
        static void CreateWasmFunctions(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment* env);
        static Var  CreateExportObject(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment* env);
        static void LoadImports(WebAssemblyModule * wasmModule, ScriptContext* ctx, Var ffi, WebAssemblyEnvironment* env);
        static void InitialGlobals(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment* env);
        static void InitializeFunctionTable(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment* env);
        static void ValidateTableAndMemory(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment* env);

        Field(WebAssemblyModule *) m_module;
        Field(Js::Var) m_exports;
    };

    template <> inline bool VarIsImpl<WebAssemblyInstance>(RecyclableObject* obj)
    {
        return JavascriptOperators::GetTypeId(obj) == TypeIds_WebAssemblyInstance;
    }
} // namespace Js
#endif

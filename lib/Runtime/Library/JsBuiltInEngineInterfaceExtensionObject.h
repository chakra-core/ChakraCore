//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#ifdef ENABLE_JS_BUILTINS
namespace Js
{

    #define enum_file(class, type, obj) class##_##type,
    typedef enum {
        JsBuiltIns(enum_file)
    } JsBuiltInFile;
    #undef enum_file

    class JsBuiltInEngineInterfaceExtensionObject : public EngineExtensionObjectBase
    {
    public:
        JsBuiltInEngineInterfaceExtensionObject(ScriptContext* scriptContext);
        void Initialize();
        void InitializePrototypes(ScriptContext * scriptContext);
        void InjectJsBuiltInLibraryCode(ScriptContext * scriptContext, JsBuiltInFile file);

        static bool __cdecl InitializeJsBuiltInNativeInterfaces(DynamicObject* intlNativeInterfaces, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);

#if DBG
        void DumpByteCode() override;
#endif

        class EntryInfo
        {
        public:
            static NoProfileFunctionInfo JsBuiltIn_RegisterChakraLibraryFunction;
            static NoProfileFunctionInfo JsBuiltIn_RegisterFunction;
            static NoProfileFunctionInfo JsBuiltIn_Internal_GetLength;
            static NoProfileFunctionInfo JsBuiltIn_Internal_GetIteratorPrototype;
            static NoProfileFunctionInfo JsBuiltIn_Internal_InitInternalProperties;
            static NoProfileFunctionInfo JsBuiltIn_Internal_ToLengthFunction;
            static NoProfileFunctionInfo JsBuiltIn_Internal_ToIntegerFunction;
            static NoProfileFunctionInfo JsBuiltIn_Internal_ArraySpeciesCreate;
            static NoProfileFunctionInfo JsBuiltIn_Internal_ArrayCreateDataPropertyOrThrow;
        };

    private:
        Field(DynamicObject*) builtInNativeInterfaces;

        #define BuiltInFunctionBody(class, type, obj) Field(FunctionBody*) jsBuiltIn##class##_##type##Bytecode = nullptr;
        JsBuiltIns(BuiltInFunctionBody)
        #undef BuiltInFunctionBody

        Field(JsBuiltInFile) current;
        Field(bool) wasInitialized = false;
        Field(SRCINFO*) sourceInfo = nullptr;

        void EnsureSourceInfo();
        void JsBuiltInEngineInterfaceExtensionObject::DumpByteCode(JsBuiltInFile file);

        static Var EntryJsBuiltIn_RegisterChakraLibraryFunction(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryJsBuiltIn_RegisterFunction(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryJsBuiltIn_Internal_GetLength(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryJsBuiltIn_Internal_GetIteratorPrototype(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryJsBuiltIn_Internal_InitInternalProperties(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryJsBuiltIn_Internal_ToLengthFunction(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryJsBuiltIn_Internal_ToIntegerFunction(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryJsBuiltIn_Internal_ArraySpeciesCreate(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryJsBuiltIn_Internal_ArrayCreateDataPropertyOrThrow(RecyclableObject* function, CallInfo callInfo, ...);
    };
}
#endif // ENABLE_JS_BUILTINS

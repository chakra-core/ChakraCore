//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#ifdef ENABLE_BUILTIN_OBJECT
namespace Js
{
    class BuiltInEngineInterfaceExtensionObject : public EngineExtensionObjectBase
    {
    public:
        BuiltInEngineInterfaceExtensionObject(ScriptContext* scriptContext);
        void Initialize();
        void InjectBuiltInLibraryCode(_In_ ScriptContext * scriptContext);

        static void __cdecl InitializeBuiltInNativeInterfaces(DynamicObject* intlNativeInterfaces, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);

#if DBG
        void DumpByteCode() override;
#endif

        class EntryInfo
        {
        public:
            static NoProfileFunctionInfo BuiltIn_RegisterFunction;

            static NoProfileFunctionInfo BuiltIn_Internal_ToLengthFunction;
            static NoProfileFunctionInfo BuiltIn_Internal_ToIntegerFunction;
        };

    private:
        Field(DynamicObject*) builtInNativeInterfaces;
        Field(FunctionBody*) builtInByteCode;

        Field(bool) wasInitialized;

        void EnsureBuiltInByteCode(_In_ ScriptContext * scriptContext);

        static DynamicObject* GetPrototypeFromName(Js::PropertyIds propertyId, ScriptContext* scriptContext);
        static Var EntryBuiltIn_RegisterFunction(RecyclableObject* function, CallInfo callInfo, ...);

        static Var EntryBuiltIn_Internal_ToLengthFunction(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryBuiltIn_Internal_ToIntegerFunction(RecyclableObject* function, CallInfo callInfo, ...);
    };
}
#endif // ENABLE_BUILTIN_OBJECT

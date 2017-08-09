//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class GeneratorVirtualScriptFunction;

    class JavascriptGeneratorFunction : public ScriptFunctionBase
    {
    private:
        static FunctionInfo functionInfo;
        Field(GeneratorVirtualScriptFunction*) scriptFunction;

        bool GetPropertyBuiltIns(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext, BOOL* result);
        bool SetPropertyBuiltIns(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info, BOOL* result);

    protected:
        DEFINE_VTABLE_CTOR(JavascriptGeneratorFunction, ScriptFunctionBase);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptGeneratorFunction);

        JavascriptGeneratorFunction(DynamicType* type);
        JavascriptGeneratorFunction(DynamicType* type, FunctionInfo* functionInfo, GeneratorVirtualScriptFunction* scriptFunction);

    public:
        JavascriptGeneratorFunction(DynamicType* type, GeneratorVirtualScriptFunction* scriptFunction);

        virtual JavascriptString* GetDisplayNameImpl() const override;
        GeneratorVirtualScriptFunction* GetGeneratorVirtualScriptFunction() { return scriptFunction; }

        static JavascriptGeneratorFunction* FromVar(Var var);
        static bool Is(Var var);
        inline static bool Test(JavascriptFunction *obj)
        {
            return VirtualTableInfo<JavascriptGeneratorFunction>::HasVirtualTable(obj)
              || VirtualTableInfo<CrossSiteObject<JavascriptGeneratorFunction>>::HasVirtualTable(obj);
        }

        static JavascriptGeneratorFunction* OP_NewScGenFunc(FrameDisplay* environment, FunctionInfoPtrPtr infoRef);
        static Var EntryGeneratorFunctionImplementation(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryAsyncFunctionImplementation(RecyclableObject* function, CallInfo callInfo, ...);
        static DWORD GetOffsetOfScriptFunction() { return offsetof(JavascriptGeneratorFunction, scriptFunction); }

        virtual Var GetHomeObj() const override;
        virtual void SetHomeObj(Var homeObj) override;
        virtual void SetComputedNameVar(Var computedNameVar) override;
        virtual Var GetComputedNameVar() const override;
        virtual bool IsAnonymousFunction() const override;

        virtual Var GetSourceString() const;
        virtual Var EnsureSourceString();

        virtual PropertyQueryFlags HasPropertyQuery(PropertyId propertyId) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;

        virtual BOOL SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags = PropertyOperation_None) override;
        virtual BOOL GetAccessors(PropertyId propertyId, Var *getter, Var *setter, ScriptContext * requestContext) override;
        virtual DescriptorFlags GetSetter(PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual DescriptorFlags GetSetter(JavascriptString* propertyNameString, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;

        virtual BOOL InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags = PropertyOperation_None, PropertyValueInfo* info = NULL) override;
        virtual BOOL DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags) override;
        virtual BOOL DeleteProperty(JavascriptString *propertyNameString, PropertyOperationFlags flags) override;

        virtual BOOL IsWritable(PropertyId propertyId) override;
        virtual BOOL IsEnumerable(PropertyId propertyId) override;
        virtual bool IsGeneratorFunction() const { return true; };

        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
        };

        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);
        static Var NewInstanceRestrictedMode(RecyclableObject* function, CallInfo callInfo, ...);

#if ENABLE_TTD
        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif

    public:
        virtual VTableValue DummyVirtualFunctionToHinderLinkerICF()
        {
            return VTableValue::VtableJavascriptGeneratorFunction;
        }
    };

    class JavascriptAsyncFunction : public JavascriptGeneratorFunction
    {
    private:
        static FunctionInfo functionInfo;

        DEFINE_VTABLE_CTOR(JavascriptAsyncFunction, JavascriptGeneratorFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptAsyncFunction);

    protected:
        JavascriptAsyncFunction(DynamicType* type);

    public:
        JavascriptAsyncFunction(DynamicType* type, GeneratorVirtualScriptFunction* scriptFunction);

        static JavascriptAsyncFunction* New(ScriptContext* scriptContext, GeneratorVirtualScriptFunction* scriptFunction);
        static DWORD GetOffsetOfScriptFunction() { return JavascriptGeneratorFunction::GetOffsetOfScriptFunction(); }

        static JavascriptAsyncFunction* FromVar(Var var);
        static bool Is(Var var);
        inline static bool Test(JavascriptFunction *obj)
        {
            return VirtualTableInfo<JavascriptAsyncFunction>::HasVirtualTable(obj)
              || VirtualTableInfo<CrossSiteObject<JavascriptAsyncFunction>>::HasVirtualTable(obj);
        }

#if ENABLE_TTD
        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif

    public:
        virtual VTableValue DummyVirtualFunctionToHinderLinkerICF()
        {
            return VTableValue::VtableJavascriptAsyncFunction;
        }
    };

    class GeneratorVirtualScriptFunction : public ScriptFunction
    {
    private:
        friend class JavascriptGeneratorFunction;
        friend Var Js::JavascriptFunction::NewInstanceHelper(ScriptContext*, RecyclableObject*, CallInfo, ArgumentReader&, Js::JavascriptFunction::FunctionKind);

        Field(JavascriptGeneratorFunction*) realFunction;

        void SetRealGeneratorFunction(JavascriptGeneratorFunction* realFunction) { this->realFunction = realFunction; }

    public:
        GeneratorVirtualScriptFunction(FunctionProxy* proxy, ScriptFunctionType* deferredPrototypeType) : ScriptFunction(proxy, deferredPrototypeType) { }

        static uint32 GetRealFunctionOffset() { return offsetof(GeneratorVirtualScriptFunction, realFunction); }

        virtual JavascriptFunction* GetRealFunctionObject() override { return realFunction; }

#if ENABLE_TTD
        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };
}

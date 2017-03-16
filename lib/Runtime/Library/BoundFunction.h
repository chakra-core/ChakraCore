//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class BoundFunction : public JavascriptFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(BoundFunction, JavascriptFunction);
        virtual void MarshalToScriptContext(Js::ScriptContext * scriptContext) override;

#if ENABLE_TTD
        virtual void MarshalCrossSite_TTDInflate() override;
#endif

    private:
        bool GetPropertyBuiltIns(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext, BOOL* result);
        bool SetPropertyBuiltIns(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info, BOOL* result);

    protected:
        BoundFunction(DynamicType * type);
        BoundFunction(Arguments args, DynamicType * type);
        BoundFunction(RecyclableObject* targetFunction, Var boundThis, Var* args, uint argsCount, DynamicType *type);
    public:
        static BoundFunction* New(ScriptContext* scriptContext, ArgumentReader args);

        static bool Is(Var func){ return JavascriptFunction::Is(func) && JavascriptFunction::FromVar(func)->IsBoundFunction(); }
        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);
        virtual JavascriptString* GetDisplayNameImpl() const override;
        virtual PropertyQueryFlags HasPropertyQuery(PropertyId propertyId) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;

        virtual BOOL GetAccessors(PropertyId propertyId, Var *getter, Var *setter, ScriptContext * requestContext) override;
        virtual DescriptorFlags GetSetter(PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual DescriptorFlags GetSetter(JavascriptString* propertyNameString, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;

        virtual BOOL InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags = PropertyOperation_None, PropertyValueInfo* info = NULL) override;
        virtual BOOL DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags) override;
        virtual BOOL DeleteProperty(JavascriptString *propertyNameString, PropertyOperationFlags flags) override;

        virtual BOOL IsWritable(PropertyId propertyId) override;
        virtual BOOL IsConfigurable(PropertyId propertyId) override;
        virtual BOOL IsEnumerable(PropertyId propertyId) override;
        virtual BOOL HasInstance(Var instance, ScriptContext* scriptContext, IsInstInlineCache* inlineCache = NULL) override;
        virtual inline BOOL IsConstructor() const override;

        // Below functions are used by debugger to identify and emit event handler information
        virtual bool IsBoundFunction() const { return true; }
        JavascriptFunction * GetTargetFunction() const;
        // Below functions are used by heap enumerator
        uint GetArgsCountForHeapEnum() { return count;}
        Field(Var)* GetArgsForHeapEnum() { return boundArgs;}
        RecyclableObject* GetBoundThis();

#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;
        virtual void ProcessCorePaths() override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;

        static BoundFunction* InflateBoundFunction(ScriptContext* ctx, RecyclableObject* function, Var bThis, uint32 ct, Var* args);
#endif

    private:
        static FunctionInfo        functionInfo;
        Field(RecyclableObject*)   targetFunction;
        Field(Var)                 boundThis;
        Field(uint)                count;
        Field(Field(Var)*)                boundArgs;
    };
} // namespace Js

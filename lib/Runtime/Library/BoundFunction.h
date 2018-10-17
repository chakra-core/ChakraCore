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
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(BoundFunction);

    private:
        bool GetPropertyBuiltIns(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext, BOOL* result);
        bool SetPropertyBuiltIns(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info, BOOL* result);

    protected:
        BoundFunction(DynamicType * type);
        BoundFunction(Arguments args, DynamicType * type);

    public:
        static BoundFunction* New(ScriptContext* scriptContext, ArgumentReader args);

        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);
        virtual JavascriptString* GetDisplayNameImpl() const override;
        virtual PropertyQueryFlags GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;

        _Check_return_ _Success_(return) virtual BOOL GetAccessors(PropertyId propertyId, _Outptr_result_maybenull_ Var* getter, _Outptr_result_maybenull_ Var* setter, ScriptContext* requestContext) override;
        virtual DescriptorFlags GetSetter(PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual DescriptorFlags GetSetter(JavascriptString* propertyNameString, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;

        virtual BOOL InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags = PropertyOperation_None, PropertyValueInfo* info = NULL) override;
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

        static BoundFunction* InflateBoundFunction(
            ScriptContext* ctx, RecyclableObject* function, Var bThis, uint32 ct, Field(Var)* args);
#endif

    private:
        static FunctionInfo        functionInfo;
        Field(RecyclableObject*)   targetFunction;
        Field(Var)                 boundThis;
        Field(uint)                count;
        Field(Field(Var)*)         boundArgs;
    };

    template <> inline bool VarIsImpl<BoundFunction>(RecyclableObject* obj)
    {
        return VarIs<JavascriptFunction>(obj) && UnsafeVarTo<JavascriptFunction>(obj)->IsBoundFunction();
    }
} // namespace Js

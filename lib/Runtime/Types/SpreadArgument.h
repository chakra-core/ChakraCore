//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
namespace Js
{

    class SpreadArgument : public DynamicObject
    {
    private:
        typedef JsUtil::List<Var, Recycler> VarList;
        Field(VarList*) iteratorIndices;

        void AssertAndFailFast() { AssertMsg(false, "This function should not be invoked"); Js::Throw::InternalError();}
    protected:
        DEFINE_VTABLE_CTOR(SpreadArgument, DynamicObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(SpreadArgument);

    public:
        static bool Is(Var aValue);
        static SpreadArgument* FromVar(Var value);
        SpreadArgument(Var iterator, bool useDirectCall, DynamicType * type);
        const Var* GetArgumentSpread() const { return iteratorIndices ? iteratorIndices->GetBuffer() : nullptr; }
        uint GetArgumentSpreadCount()  const { return iteratorIndices ? iteratorIndices->Count() : 0; }

        // A SpreadArgument should never call the Functions defined below this comment
        virtual PropertyQueryFlags HasPropertyQuery(PropertyId propertyId) override { AssertAndFailFast();  return PropertyQueryFlags::Property_NotFound; };
        virtual BOOL HasOwnProperty(PropertyId propertyId) override { AssertAndFailFast(); return FALSE; };
        virtual BOOL SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override { AssertAndFailFast(); return FALSE; };
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override { AssertAndFailFast(); return PropertyQueryFlags::Property_NotFound; };
        virtual BOOL DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags) override{ AssertAndFailFast(); return FALSE;};
        virtual BOOL DeleteProperty(JavascriptString * propertyNameString, PropertyOperationFlags flags) override { AssertAndFailFast(); return FALSE; };
        virtual PropertyQueryFlags GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override { AssertAndFailFast(); return PropertyQueryFlags::Property_NotFound; };
        virtual BOOL SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override { AssertAndFailFast(); return FALSE; };
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override { AssertAndFailFast(); return PropertyQueryFlags::Property_NotFound; };
        virtual DescriptorFlags GetSetter(PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override { AssertAndFailFast(); return None; };
        virtual DescriptorFlags GetSetter(JavascriptString* propertyNameString, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override { AssertAndFailFast(); return None; };
        virtual int GetPropertyCount() override { AssertAndFailFast(); return 0; };
        virtual PropertyId GetPropertyId(PropertyIndex index) override { AssertAndFailFast();  return Constants::NoProperty; };
        virtual PropertyId GetPropertyId(BigPropertyIndex index) override { AssertAndFailFast(); return Constants::NoProperty;; };
        virtual BOOL SetInternalProperty(PropertyId internalPropertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override { AssertAndFailFast(); return FALSE; };
        virtual BOOL InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags = PropertyOperation_None, PropertyValueInfo* info = NULL) override { AssertAndFailFast(); return FALSE; };
        virtual BOOL SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags = PropertyOperation_None, SideEffects possibleSideEffects = SideEffects_Any) override { AssertAndFailFast(); return FALSE; };
#if ENABLE_FIXED_FIELDS
        virtual BOOL IsFixedProperty(PropertyId propertyId) override { AssertAndFailFast(); return FALSE; };
#endif
        virtual PropertyQueryFlags HasItemQuery(uint32 index) override { AssertAndFailFast(); return PropertyQueryFlags::Property_NotFound; };
        virtual BOOL HasOwnItem(uint32 index) override { AssertAndFailFast(); return FALSE; };
        virtual PropertyQueryFlags GetItemQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override { AssertAndFailFast(); return PropertyQueryFlags::Property_NotFound; };
        virtual PropertyQueryFlags GetItemReferenceQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override { AssertAndFailFast(); return PropertyQueryFlags::Property_NotFound; };
        virtual DescriptorFlags GetItemSetter(uint32 index, Var* setterValue, ScriptContext* requestContext) override { AssertAndFailFast(); return None; };
        virtual BOOL SetItem(uint32 index, Var value, PropertyOperationFlags flags) override { AssertAndFailFast(); return FALSE; };
        virtual BOOL DeleteItem(uint32 index, PropertyOperationFlags flags) override { AssertAndFailFast(); return FALSE; };
        virtual BOOL ToPrimitive(JavascriptHint hint, Var* result, ScriptContext * requestContext) override { AssertAndFailFast(); return FALSE; };
        virtual BOOL GetEnumerator(JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext* requestContext, ForInCache * forInCache = nullptr) override { AssertAndFailFast(); return FALSE; };
        virtual BOOL SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags = PropertyOperation_None) override { AssertAndFailFast(); return FALSE; };
        virtual BOOL GetAccessors(PropertyId propertyId, Var *getter, Var *setter, ScriptContext * requestContext) override { AssertAndFailFast(); return FALSE; };
        virtual BOOL IsWritable(PropertyId propertyId) override { AssertAndFailFast(); return FALSE; };
        virtual BOOL IsConfigurable(PropertyId propertyId) override { AssertAndFailFast(); return FALSE; };
        virtual BOOL IsEnumerable(PropertyId propertyId) override { AssertAndFailFast(); return FALSE; };
        virtual BOOL SetEnumerable(PropertyId propertyId, BOOL value) override { AssertAndFailFast(); return FALSE; };
        virtual BOOL SetWritable(PropertyId propertyId, BOOL value) override { AssertAndFailFast(); return FALSE; };
        virtual BOOL SetConfigurable(PropertyId propertyId, BOOL value) override { AssertAndFailFast(); return FALSE; };
        virtual BOOL SetAttributes(PropertyId propertyId, PropertyAttributes attributes) override { AssertAndFailFast(); return FALSE; };
        virtual BOOL IsExtensible() override { AssertAndFailFast(); return FALSE; };
        virtual BOOL PreventExtensions() override { AssertAndFailFast(); return FALSE; };
        virtual BOOL Seal() override { AssertAndFailFast(); return FALSE; };
        virtual BOOL Freeze() override { AssertAndFailFast(); return FALSE; };
        virtual BOOL IsSealed() override { AssertAndFailFast(); return FALSE; };
        virtual BOOL IsFrozen() override { AssertAndFailFast(); return FALSE; };
        virtual BOOL GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override { AssertAndFailFast(); return FALSE; };
        virtual Var GetTypeOfString(ScriptContext * requestContext) override { AssertAndFailFast(); return RecyclableObject::GetTypeOfString(requestContext); };
    };
}

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// The UnscopablesWrapperObject is a psuedo-object that provides us with convenience. By wrapping a normal object in this class,
// we can intercept object operations requiring checks to @@unscopables. For all other operations, the object needs to be
// unwrapped. The caller is responsible for unwrapping, which must happen only once the @@unscopables check has happened.
// For example, a getter needs to have the propId checked against @@unscopables, but for the actual getter call the
// object passed needs to be unwrapped.
//
#pragma once

#define UNWRAP_FAILFAST() AssertOrFailFastMsg(false, "This UnscopablesWrapperObject must be unwrapped by the caller handling the scope before performing this operation.")

namespace Js
{
    class UnscopablesWrapperObject : public RecyclableObject
    {
        private:
            Field(RecyclableObject *) wrappedObject;

        protected:
            DEFINE_VTABLE_CTOR(UnscopablesWrapperObject, RecyclableObject);

        public:
            UnscopablesWrapperObject(RecyclableObject *wrappedObject, StaticType * type) : RecyclableObject(type), wrappedObject(wrappedObject) {}
            RecyclableObject *GetWrappedObject() { return wrappedObject; }
            virtual PropertyQueryFlags HasPropertyQuery(PropertyId propertyId, _Inout_opt_ PropertyValueInfo* info) override;
            virtual BOOL HasOwnProperty(PropertyId propertyId) override;
            virtual BOOL SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
            virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
            virtual BOOL DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags) override;
            virtual PropertyQueryFlags GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
            virtual DescriptorFlags GetSetter(PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;

            // A UnscopablesWrapperObject should never call the Functions defined below this comment
            virtual BOOL SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override { UNWRAP_FAILFAST(); return FALSE; };
            virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override { UNWRAP_FAILFAST(); return PropertyQueryFlags::Property_NotFound; };
            virtual DescriptorFlags GetSetter(JavascriptString* propertyNameString, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override { UNWRAP_FAILFAST(); return None; };
            virtual int GetPropertyCount() override { UNWRAP_FAILFAST(); return 0; };
            virtual PropertyId GetPropertyId(PropertyIndex index) override { UNWRAP_FAILFAST();  return Constants::NoProperty; };
            virtual PropertyId GetPropertyId(BigPropertyIndex index) override { UNWRAP_FAILFAST(); return Constants::NoProperty;; };
            virtual BOOL SetInternalProperty(PropertyId internalPropertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override { UNWRAP_FAILFAST(); return FALSE; };
            virtual BOOL InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags = PropertyOperation_None, PropertyValueInfo* info = NULL) override { UNWRAP_FAILFAST(); return FALSE; };
            virtual BOOL SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags = PropertyOperation_None, SideEffects possibleSideEffects = SideEffects_Any) override { UNWRAP_FAILFAST(); return FALSE; };
#if ENABLE_FIXED_FIELDS
            virtual BOOL IsFixedProperty(PropertyId propertyId) override { UNWRAP_FAILFAST(); return FALSE; };
#endif
            virtual PropertyQueryFlags HasItemQuery(uint32 index) override { UNWRAP_FAILFAST(); return PropertyQueryFlags::Property_NotFound; };
            virtual BOOL HasOwnItem(uint32 index) override { UNWRAP_FAILFAST(); return FALSE; };
            virtual PropertyQueryFlags GetItemQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override { UNWRAP_FAILFAST(); return PropertyQueryFlags::Property_NotFound; };
            virtual PropertyQueryFlags GetItemReferenceQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override { UNWRAP_FAILFAST(); return PropertyQueryFlags::Property_NotFound; };
            virtual DescriptorFlags GetItemSetter(uint32 index, Var* setterValue, ScriptContext* requestContext) override { UNWRAP_FAILFAST(); return None; };
            virtual BOOL SetItem(uint32 index, Var value, PropertyOperationFlags flags) override { UNWRAP_FAILFAST(); return FALSE; };
            virtual BOOL DeleteItem(uint32 index, PropertyOperationFlags flags) override { UNWRAP_FAILFAST(); return FALSE; };
            virtual BOOL ToPrimitive(JavascriptHint hint, Var* result, ScriptContext * requestContext) override { UNWRAP_FAILFAST(); return FALSE; };
            virtual BOOL GetEnumerator(JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext* requestContext, EnumeratorCache * enumeratorCache = nullptr) override { UNWRAP_FAILFAST(); return FALSE; };
            virtual BOOL SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags = PropertyOperation_None) override { UNWRAP_FAILFAST(); return FALSE; };
            _Check_return_ _Success_(return) virtual BOOL GetAccessors(PropertyId propertyId, _Outptr_result_maybenull_ Var* getter, _Outptr_result_maybenull_ Var* setter, ScriptContext * requestContext) override { UNWRAP_FAILFAST(); return FALSE; };
            virtual BOOL IsWritable(PropertyId propertyId) override { UNWRAP_FAILFAST(); return FALSE; };
            virtual BOOL IsConfigurable(PropertyId propertyId) override { UNWRAP_FAILFAST(); return FALSE; };
            virtual BOOL IsEnumerable(PropertyId propertyId) override { UNWRAP_FAILFAST(); return FALSE; };
            virtual BOOL SetEnumerable(PropertyId propertyId, BOOL value) override { UNWRAP_FAILFAST(); return FALSE; };
            virtual BOOL SetWritable(PropertyId propertyId, BOOL value) override { UNWRAP_FAILFAST(); return FALSE; };
            virtual BOOL SetConfigurable(PropertyId propertyId, BOOL value) override { UNWRAP_FAILFAST(); return FALSE; };
            virtual BOOL SetAttributes(PropertyId propertyId, PropertyAttributes attributes) override { UNWRAP_FAILFAST(); return FALSE; };
            virtual BOOL IsExtensible() override { UNWRAP_FAILFAST(); return FALSE; };
            virtual BOOL PreventExtensions() override { UNWRAP_FAILFAST(); return FALSE; };
            virtual BOOL Seal() override { UNWRAP_FAILFAST(); return FALSE; };
            virtual BOOL Freeze() override { UNWRAP_FAILFAST(); return FALSE; };
            virtual BOOL IsSealed() override { UNWRAP_FAILFAST(); return FALSE; };
            virtual BOOL IsFrozen() override { UNWRAP_FAILFAST(); return FALSE; };
            virtual BOOL GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override { UNWRAP_FAILFAST(); return FALSE; };
            virtual Var GetTypeOfString(ScriptContext * requestContext) override { UNWRAP_FAILFAST(); return RecyclableObject::GetTypeOfString(requestContext); };
    };

    template <> inline bool VarIsImpl<UnscopablesWrapperObject>(RecyclableObject* obj)
    {
        return JavascriptOperators::GetTypeId(obj) == TypeIds_UnscopablesWrapperObject;
    }
} // namespace Js

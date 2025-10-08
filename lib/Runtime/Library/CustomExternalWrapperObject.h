//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    typedef void (*JsTraceCallback)(void * data);
    typedef void (*JsFinalizeCallback)(void * data);

    typedef struct _JsGetterSetterInterceptor {
        Field(void *) getTrap;
        Field(void *) setTrap;
        Field(void *) deletePropertyTrap;
        Field(void *) enumerateTrap;
        Field(void *) ownKeysTrap;
        Field(void *) hasTrap;
        Field(void *) getOwnPropertyDescriptorTrap;
        Field(void *) definePropertyTrap;
        Field(void *) initializeTrap;
    } JsGetterSetterInterceptor;

    class CustomExternalWrapperType sealed : public DynamicType
    {
    public:
        CustomExternalWrapperType(CustomExternalWrapperType * type) : DynamicType(type), jsTraceCallback(type->jsTraceCallback), jsFinalizeCallback(type->jsFinalizeCallback), jsGetterSetterInterceptor(type->jsGetterSetterInterceptor) {}
        CustomExternalWrapperType(ScriptContext * scriptContext, JsTraceCallback traceCallback, JsFinalizeCallback finalizeCallback, RecyclableObject * prototype);

        JsTraceCallback GetJsTraceCallback() const { return this->jsTraceCallback; }
        JsFinalizeCallback GetJsFinalizeCallback() const { return this->jsFinalizeCallback; }
        JsGetterSetterInterceptor * GetJsGetterSetterInterceptor() const { return this->jsGetterSetterInterceptor; }

    private:
        FieldNoBarrier(JsTraceCallback const) jsTraceCallback;
        FieldNoBarrier(JsFinalizeCallback const) jsFinalizeCallback;
        FieldNoBarrier(JsGetterSetterInterceptor *) jsGetterSetterInterceptor;
    };

    class CustomExternalWrapperObject : public DynamicObject
    {
    protected:
        DEFINE_VTABLE_CTOR(CustomExternalWrapperObject, DynamicObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(CustomExternalWrapperObject);

    public:
        typedef enum SetPropertyTrapKind {
            SetItemOnTaggedNumberKind,
            SetPropertyOnTaggedNumberKind,
            SetPropertyKind,
            SetItemKind,
            SetPropertyWPCacheKind,
        } SetPropertyTrapKind;

        enum KeysTrapKind {
            GetOwnEnumerablePropertyNamesKind,
            GetOwnPropertyNamesKind,
            GetOwnPropertySymbolKind,
            EnumerableKeysKind,
            KeysKind
        };

        CustomExternalWrapperObject(CustomExternalWrapperType * type, void *data, uint inlineSlotSize);
        CustomExternalWrapperObject(CustomExternalWrapperObject* instance, bool deepCopy);

        BOOL IsObjectAlive();
        BOOL VerifyObjectAlive();

        static CustomExternalWrapperObject * Create(void *data, uint inlineSlotSize, JsTraceCallback traceCallback, JsFinalizeCallback finalizeCallback, JsGetterSetterInterceptor ** getterSetterInterceptor, RecyclableObject * prototype, ScriptContext *scriptContext);

        virtual CustomExternalWrapperObject* Copy(bool deepCopy) override;

        static BOOL GetOwnPropertyDescriptor(RecyclableObject * obj, PropertyId propertyId, ScriptContext* requestContext, PropertyDescriptor* propertyDescriptor);
        static BOOL DefineOwnPropertyDescriptor(RecyclableObject * obj, PropertyId propId, const PropertyDescriptor& descriptor, bool throwOnError, ScriptContext* requestContext);

        CustomExternalWrapperType * GetExternalType() const { return (CustomExternalWrapperType *)this->GetType(); }

        BOOL EnsureInitialized(ScriptContext* requestContext);

        void Mark(Recycler * recycler) override;
        void Finalize(bool isShutdown) override;
        void Dispose(bool isShutdown) override;

        bool HasReadOnlyPropertiesInvisibleToTypeHandler() override { return true; }
        bool IsInitialized() const { return this->initialized; }

        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var * value, PropertyValueInfo * info, ScriptContext * requestContext) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var * value, PropertyValueInfo * info, ScriptContext * requestContext) override;
        virtual PropertyQueryFlags GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var * value, PropertyValueInfo * info, ScriptContext * requestContext) override;
        virtual PropertyQueryFlags HasPropertyQuery(PropertyId propertyId, _Inout_opt_ PropertyValueInfo* info) override;
        virtual BOOL SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo * info) override;
        virtual BOOL SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo * info) override;
        virtual BOOL EnsureNoRedeclProperty(PropertyId propertyId) override;
        virtual BOOL InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags = PropertyOperation_None, PropertyValueInfo* info = nullptr) override;
        virtual BOOL DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags) override;
        virtual BOOL DeleteProperty(JavascriptString *propertyNameString, PropertyOperationFlags flags) override;
        virtual PropertyQueryFlags HasItemQuery(uint32 index) override;
        virtual BOOL HasOwnItem(uint32 index) override;
        virtual PropertyQueryFlags GetItemQuery(Var originalInstance, uint32 index, Var * value, ScriptContext * requestContext) override;
        virtual PropertyQueryFlags GetItemReferenceQuery(Var originalInstance, uint32 index, Var * value, ScriptContext * requestContext) override;
        virtual BOOL SetItem(uint32 index, Var value, PropertyOperationFlags flags) override;
        virtual BOOL DeleteItem(uint32 index, PropertyOperationFlags flags) override;
        virtual BOOL GetEnumerator(JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext * requestContext, EnumeratorCache * enumeratorCache = nullptr) override;
        virtual BOOL Equals(_In_ Var other, _Out_ BOOL* value, ScriptContext* requestContext) override;
        virtual BOOL StrictEquals(_In_ Var other, _Out_ BOOL* value, ScriptContext* requestContext) override;

        virtual DynamicType* DuplicateType() override;
        virtual void SetPrototype(RecyclableObject* newPrototype) override;

        void PropertyIdFromInt(uint32 index, PropertyRecord const** propertyRecord);

        template <class Fn, class GetPropertyNameFunc>
        BOOL SetPropertyTrap(Var receiver, SetPropertyTrapKind setPropertyTrapKind, GetPropertyNameFunc getPropertyName, Var newValue, ScriptContext * requestContext, PropertyOperationFlags propertyOperationFlags, BOOL skipPrototypeCheck, Fn fn, Js::PropertyValueInfo* info);
        BOOL SetPropertyTrap(Var receiver, SetPropertyTrapKind setPropertyTrapKind, JavascriptString * propertyString, Var newValue, ScriptContext * requestContext, PropertyOperationFlags propertyOperationFlags, Js::PropertyValueInfo* info);

        void * GetSlotData() const;
        void SetSlotData(void * data);
        int GetInlineSlotSize() const;
        void* GetInlineSlots() const;

        virtual PropertyId GetPropertyId(PropertyIndex index) override { if (!EnsureInitialized(GetScriptContext())) { return Constants::NoProperty; } return DynamicObject::GetPropertyId(index); }
        virtual PropertyId GetPropertyId(BigPropertyIndex index) override { if (!EnsureInitialized(GetScriptContext())) { return Constants::NoProperty; } return DynamicObject::GetPropertyId(index); }
        virtual BOOL HasOwnProperty(PropertyId propertyId) override { if (!EnsureInitialized(GetScriptContext())) { return FALSE; } return DynamicObject::HasOwnProperty(propertyId); }
        virtual DescriptorFlags GetSetter(PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override { if (!EnsureInitialized(GetScriptContext())) { return None; }  return DynamicObject::GetSetter(propertyId, setterValue, info, requestContext); }
        virtual DescriptorFlags GetSetter(JavascriptString* propertyNameString, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override { if (!EnsureInitialized(GetScriptContext())) { return None; }  return DynamicObject::GetSetter(propertyNameString, setterValue, info, requestContext); }
        virtual BOOL SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags = PropertyOperation_None, SideEffects possibleSideEffects = SideEffects_Any) override { if (!EnsureInitialized(GetScriptContext())) { return FALSE; } return DynamicObject::SetPropertyWithAttributes(propertyId, value, attributes, info, flags, possibleSideEffects); }
#if ENABLE_FIXED_FIELDS
        virtual BOOL IsFixedProperty(PropertyId propertyId) override { if (!EnsureInitialized(GetScriptContext())) { return FALSE; } return DynamicObject::IsFixedProperty(propertyId); }
#endif
        virtual DescriptorFlags GetItemSetter(uint32 index, Var* setterValue, ScriptContext* requestContext) override { if (!EnsureInitialized(GetScriptContext())) { return None; } return DynamicObject::GetItemSetter(index, setterValue, requestContext); }
        virtual BOOL ToPrimitive(JavascriptHint hint, Var* result, ScriptContext * requestContext) override { if (!EnsureInitialized(GetScriptContext())) { return FALSE; } return DynamicObject::ToPrimitive(hint, result, requestContext); }
        virtual BOOL SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags = PropertyOperation_None) override { if (!EnsureInitialized(GetScriptContext())) { return FALSE; } return DynamicObject::SetAccessors(propertyId, getter, setter, flags); }
        _Check_return_ _Success_(return) virtual BOOL GetAccessors(PropertyId propertyId, _Outptr_result_maybenull_ Var* getter, _Outptr_result_maybenull_ Var* setter, ScriptContext* requestContext) override { if (!EnsureInitialized(GetScriptContext())) { return FALSE; } return DynamicObject::GetAccessors(propertyId, getter, setter, requestContext); }
        virtual BOOL IsWritable(PropertyId propertyId) override { if (!EnsureInitialized(GetScriptContext())) { return FALSE; } return DynamicObject::IsWritable(propertyId); }
        virtual BOOL IsConfigurable(PropertyId propertyId) override { if (!EnsureInitialized(GetScriptContext())) { return FALSE; } return DynamicObject::IsConfigurable(propertyId); }
        virtual BOOL IsEnumerable(PropertyId propertyId) override { if (!EnsureInitialized(GetScriptContext())) { return FALSE; } return DynamicObject::IsEnumerable(propertyId); }
        virtual BOOL SetEnumerable(PropertyId propertyId, BOOL value) override { if (!EnsureInitialized(GetScriptContext())) { return FALSE; } return DynamicObject::SetEnumerable(propertyId, value); }
        virtual BOOL SetWritable(PropertyId propertyId, BOOL value) override { if (!EnsureInitialized(GetScriptContext())) { return FALSE; } return DynamicObject::SetWritable(propertyId, value); }
        virtual BOOL SetConfigurable(PropertyId propertyId, BOOL value) override { if (!EnsureInitialized(GetScriptContext())) { return FALSE; } return DynamicObject::SetConfigurable(propertyId, value); }
        virtual BOOL SetAttributes(PropertyId propertyId, PropertyAttributes attributes) override { if (!EnsureInitialized(GetScriptContext())) { return FALSE; } return DynamicObject::SetAttributes(propertyId, attributes); }
        virtual BOOL Seal() override { if (!EnsureInitialized(GetScriptContext())) { return FALSE; } return DynamicObject::Seal(); }
        virtual BOOL Freeze() override { if (!EnsureInitialized(GetScriptContext())) { return FALSE; } return DynamicObject::Freeze(); }

#if DBG
        virtual bool CanStorePropertyValueDirectly(PropertyId propertyId, bool allowLetConst) override { if (!EnsureInitialized(GetScriptContext())) { return FALSE; } return DynamicObject::CanStorePropertyValueDirectly(propertyId, allowLetConst); }
#endif

        virtual void RemoveFromPrototype(ScriptContext * requestContext, bool * allProtoCachesInvalidated) override { if (!EnsureInitialized(GetScriptContext())) { return; } return DynamicObject::RemoveFromPrototype(requestContext, allProtoCachesInvalidated); }
        virtual void AddToPrototype(ScriptContext * requestContext, bool * allProtoCachesInvalidated) override { if (!EnsureInitialized(GetScriptContext())) { return; } return DynamicObject::AddToPrototype(requestContext, allProtoCachesInvalidated); }
        virtual bool ClearProtoCachesWereInvalidated() override { if (!EnsureInitialized(GetScriptContext())) { return FALSE; } return DynamicObject::ClearProtoCachesWereInvalidated(); }
    private:
        enum class SlotType {
            Inline,
            External
        };

        Field(bool) initialized = false;
        Field(SlotType) slotType;
        union SlotInfo
        {
            Field(void *) slot;
            Field(uint) inlineSlotSize;
            SlotInfo()
            {
                memset((void*)this, 0, sizeof(SlotInfo));
            }
        };
        Field(SlotInfo) u;

        Var GetValueFromDescriptor(Var instance, PropertyDescriptor propertyDescriptor, ScriptContext * requestContext);
        Var GetName(ScriptContext* requestContext, PropertyId propertyId, Var * isPropertyNameNumeric, Var * propertyNameNumericValue);

        BOOL GetPropertyDescriptorTrap(PropertyId propertyId, PropertyDescriptor * resultDescriptor, ScriptContext * requestContext);

        template <class Fn, class GetPropertyNameFunc>
        BOOL GetPropertyTrap(Var instance, PropertyDescriptor * propertyDescriptor, Fn fn, GetPropertyNameFunc getPropertyName, ScriptContext * requestContext, Js::PropertyValueInfo* info);

        template <class Fn, class GetPropertyNameFunc>
        BOOL HasPropertyTrap(Fn fn, GetPropertyNameFunc getPropertyName, Js::PropertyValueInfo* info);

        template <class Fn>
        void GetOwnPropertyKeysHelper(ScriptContext * scriptContext, RecyclableObject * trapResultArray, uint32 len, JavascriptArray * trapResult,
            JsUtil::BaseDictionary<PropertyId, bool, Memory::ArenaAllocator>& targetToTrapResultMap, Fn fn)
        {
            Var element = nullptr;
            const PropertyRecord* propertyRecord;
            uint32 trapResultIndex = 0;
            PropertyId propertyId;
            for (uint32 i = 0; i < len; i++)
            {
                if (!JavascriptOperators::GetItem(trapResultArray, i, &element, scriptContext) || // missing
                    !(VarIs<JavascriptString>(element) || VarIs<JavascriptSymbol>(element)))  // neither String nor Symbol
                {
                    JavascriptError::ThrowTypeError(scriptContext, JSERR_InconsistentTrapResult, _u("ownKeys"));
                }

                JavascriptConversion::ToPropertyKey(element, scriptContext, &propertyRecord, nullptr);
                propertyId = propertyRecord->GetPropertyId();

                if (propertyId != Constants::NoProperty)
                {
                    if (targetToTrapResultMap.AddNew(propertyId, true) == -1)
                    {
                        JavascriptError::ThrowTypeError(scriptContext, JSERR_InconsistentTrapResult);
                    }
                }

                if (fn(propertyRecord))
                {
                    trapResult->DirectSetItemAt(trapResultIndex++, element);
                }
            }
        }

#if ENABLE_TTD
    public:
        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

    template <> inline bool VarIsImpl<CustomExternalWrapperObject>(RecyclableObject* obj)
    {
        return (VirtualTableInfo<CustomExternalWrapperObject>::HasVirtualTable(obj)) ||
            (VirtualTableInfo<Js::CrossSiteObject<CustomExternalWrapperObject>>::HasVirtualTable(obj));
    }

}

#ifndef __APPLE__ // TODO: for some reason on OSX builds this initialization happens before PAL initialization
AUTO_REGISTER_RECYCLER_OBJECT_DUMPER(Js::CustomExternalWrapperType, &Js::Type::DumpObjectFunction);
AUTO_REGISTER_RECYCLER_OBJECT_DUMPER(Js::CustomExternalWrapperObject, &Js::RecyclableObject::DumpObjectFunction);
#endif

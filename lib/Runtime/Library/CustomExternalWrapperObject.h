//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    typedef void (*JsTraceCallback)(void * data);
    typedef void (*JsFinalizeCallback)(void * data);

    typedef struct JsGetterSetterInterceptor {
        void * getTrap;
        void * setTrap;
        void * deletePropertyTrap;
        void * enumerateTrap;
        void * ownKeysTrap;
        void * hasTrap;
        void * getOwnPropertyDescriptorTrap;
        void * definePropertyTrap;

        explicit JsGetterSetterInterceptor(JsGetterSetterInterceptor * getterSetterInterceptor);

        JsGetterSetterInterceptor();

        bool AreInterceptorsRequired();
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
    AUTO_REGISTER_RECYCLER_OBJECT_DUMPER(Js::CustomExternalWrapperType, &Js::Type::DumpObjectFunction);

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
            GetOwnPropertyNamesKind,
            GetOwnPropertySymbolKind,
            KeysKind
        };

        CustomExternalWrapperObject(CustomExternalWrapperType * type, void *data, uint inlineSlotSize);

        BOOL IsObjectAlive();
        BOOL VerifyObjectAlive();

        static CustomExternalWrapperObject * Create(void *data, uint inlineSlotSize, JsTraceCallback traceCallback, JsFinalizeCallback finalizeCallback, JsGetterSetterInterceptor ** getterSetterInterceptor, RecyclableObject * prototype, ScriptContext *scriptContext);
        static CustomExternalWrapperObject * Clone(CustomExternalWrapperObject * source, ScriptContext * scriptContext);

        static BOOL GetOwnPropertyDescriptor(RecyclableObject * obj, PropertyId propertyId, ScriptContext* requestContext, PropertyDescriptor* propertyDescriptor);
        static BOOL DefineOwnPropertyDescriptor(RecyclableObject * obj, PropertyId propId, const PropertyDescriptor& descriptor, bool throwOnError, ScriptContext* requestContext);

        CustomExternalWrapperType * GetExternalType() const { return (CustomExternalWrapperType *)this->GetType(); }

        void Mark(Recycler * recycler) override;
        void Finalize(bool isShutdown) override;
        void Dispose(bool isShutdown) override;

        bool HasReadOnlyPropertiesInvisibleToTypeHandler() override { return true; }

        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var * value, PropertyValueInfo * info, ScriptContext * requestContext) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var * value, PropertyValueInfo * info, ScriptContext * requestContext) override;
        virtual PropertyQueryFlags GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var * value, PropertyValueInfo * info, ScriptContext * requestContext) override;
        virtual PropertyQueryFlags HasPropertyQuery(PropertyId propertyId, _Inout_opt_ PropertyValueInfo* info) override;
        virtual BOOL SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo * info) override;
        virtual BOOL SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo * info) override;
        virtual BOOL SetInternalProperty(PropertyId internalPropertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo * info) override;
        virtual BOOL EnsureNoRedeclProperty(PropertyId propertyId) override;
        virtual BOOL DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags) override;
        virtual BOOL DeleteProperty(JavascriptString *propertyNameString, PropertyOperationFlags flags) override;
        virtual PropertyQueryFlags HasItemQuery(uint32 index) override;
        virtual BOOL HasOwnItem(uint32 index) override;
        virtual PropertyQueryFlags GetItemQuery(Var originalInstance, uint32 index, Var * value, ScriptContext * requestContext) override;
        virtual PropertyQueryFlags GetItemReferenceQuery(Var originalInstance, uint32 index, Var * value, ScriptContext * requestContext) override;
        virtual BOOL SetItem(uint32 index, Var value, PropertyOperationFlags flags) override;
        virtual BOOL DeleteItem(uint32 index, PropertyOperationFlags flags) override;
        virtual BOOL GetEnumerator(JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext * requestContext, EnumeratorCache * enumeratorCache = nullptr) override;
        virtual BOOL Equals(__in Var other, __out BOOL* value, ScriptContext* requestContext) override;
        virtual BOOL StrictEquals(__in Var other, __out BOOL* value, ScriptContext* requestContext) override;

        virtual DynamicType* DuplicateType() override;
        virtual void SetPrototype(RecyclableObject* newPrototype) override;

        void PropertyIdFromInt(uint32 index, PropertyRecord const** propertyRecord);

        template <class GetPropertyNameFunc>
        BOOL SetPropertyTrap(Var receiver, SetPropertyTrapKind setPropertyTrapKind, GetPropertyNameFunc getPropertyName, Var newValue, ScriptContext * requestContext, PropertyOperationFlags propertyOperationFlags, BOOL skipPrototypeCheck = FALSE);
        BOOL SetPropertyTrap(Var receiver, SetPropertyTrapKind setPropertyTrapKind, JavascriptString * propertyString, Var newValue, ScriptContext * requestContext, PropertyOperationFlags propertyOperationFlags);

        JavascriptArray * PropertyKeysTrap(KeysTrapKind keysTrapKind, ScriptContext * requestContext);

        void * GetSlotData() const;
        void SetSlotData(void * data);
        int GetInlineSlotSize() const;
        void* GetInlineSlots() const;

    private:
        enum class SlotType {
            Inline,
            External
        };

        Field(SlotType) slotType;
        union
        {
            Field(void *) slot;
            Field(uint) inlineSlotSize;
        } u;

        Var GetValueFromDescriptor(Var instance, PropertyDescriptor propertyDescriptor, ScriptContext * requestContext);
        Var GetName(ScriptContext* requestContext, PropertyId propertyId, Var * isPropertyNameNumeric, Var * propertyNameNumericValue);

        BOOL GetPropertyDescriptorTrap(PropertyId propertyId, PropertyDescriptor * resultDescriptor, ScriptContext * requestContext);

        template <class Fn, class GetPropertyNameFunc>
        BOOL GetPropertyTrap(Var instance, PropertyDescriptor * propertyDescriptor, Fn fn, GetPropertyNameFunc getPropertyName, ScriptContext * requestContext);

        template <class Fn, class GetPropertyNameFunc>
        BOOL HasPropertyTrap(Fn fn, GetPropertyNameFunc getPropertyName);

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
AUTO_REGISTER_RECYCLER_OBJECT_DUMPER(Js::CustomExternalWrapperObject, &Js::RecyclableObject::DumpObjectFunction);

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//  Implements JavascriptProxy.
//----------------------------------------------------------------------------
#pragma once

namespace Js
{
    // Host should keep the same object in cross-site scenario.
    class JavascriptProxy : public DynamicObject
    {
        friend class JavascriptOperators;
    protected:
        DEFINE_VTABLE_CTOR(JavascriptProxy, DynamicObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptProxy);
    private:
        Field(RecyclableObject*) handler;
        Field(RecyclableObject*) target;

        void RevokeObject();
    public:
        static const uint32 MAX_STACK_CALL_ARGUMENT_COUNT = 20;
        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
            static FunctionInfo Revocable;
            static FunctionInfo Revoke;
        };
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

        enum IntegrityLevel {
            IntegrityLevel_sealed,
            IntegrityLevel_frozen
        };

        JavascriptProxy(DynamicType * type);
        JavascriptProxy(DynamicType * type, ScriptContext * scriptContext, RecyclableObject* target, RecyclableObject* handler);
        static BOOL Is(Var obj);
        static JavascriptProxy* FromVar(Var obj) { Assert(Is(obj)); return static_cast<JavascriptProxy*>(obj); }
#ifndef IsJsDiag
        RecyclableObject* GetTarget();
        RecyclableObject* GetHandler();
#else
        RecyclableObject* GetTarget() { return target; }
        RecyclableObject* GetHandler() { return handler; }
#endif
        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryRevocable(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryRevoke(RecyclableObject* function, CallInfo callInfo, ...);

        static Var FunctionCallTrap(RecyclableObject* function, CallInfo callInfo, ...);
        static JavascriptProxy* Create(ScriptContext* scriptContext, Arguments args);

        static BOOL GetOwnPropertyDescriptor(RecyclableObject* obj, PropertyId propertyId, ScriptContext* requestContext, PropertyDescriptor* propertyDescriptor);
        static BOOL DefineOwnPropertyDescriptor(RecyclableObject* obj, PropertyId propId, const PropertyDescriptor& descriptor, bool throwOnError, ScriptContext* requestContext);

        static DWORD GetOffsetOfTarget() { return offsetof(JavascriptProxy, target); }

        virtual PropertyQueryFlags HasPropertyQuery(PropertyId propertyId) override;
        virtual BOOL HasOwnProperty(PropertyId propertyId) override;
        virtual BOOL HasOwnPropertyNoHostObject(PropertyId propertyId) override;
        virtual BOOL HasOwnPropertyCheckNoRedecl(PropertyId propertyId) override;
        virtual BOOL UseDynamicObjectForNoHostObjectAccess() override;
        virtual DescriptorFlags GetSetter(PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual DescriptorFlags GetSetter(JavascriptString* propertyNameString, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL GetInternalProperty(Var instance, PropertyId internalPropertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL GetAccessors(PropertyId propertyId, Var* getter, Var* setter, ScriptContext * requestContext) override;
        virtual PropertyQueryFlags GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetInternalProperty(PropertyId internalPropertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags = PropertyOperation_None, PropertyValueInfo* info = NULL) override;
        virtual BOOL EnsureProperty(PropertyId propertyId) override;
        virtual BOOL EnsureNoRedeclProperty(PropertyId propertyId) override;
        virtual BOOL SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags = PropertyOperation_None, SideEffects possibleSideEffects = SideEffects_Any) override;
        virtual BOOL InitPropertyScoped(PropertyId propertyId, Var value) override;
        virtual BOOL InitFuncScoped(PropertyId propertyId, Var value) override;
        virtual BOOL DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags) override;
        virtual BOOL DeleteProperty(JavascriptString *propertyNameString, PropertyOperationFlags flags) override;
#if ENABLE_FIXED_FIELDS
        virtual BOOL IsFixedProperty(PropertyId propertyId) override;
#endif
        virtual PropertyQueryFlags HasItemQuery(uint32 index) override;
        virtual BOOL HasOwnItem(uint32 index) override;
        virtual PropertyQueryFlags GetItemQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override;
        virtual PropertyQueryFlags GetItemReferenceQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override;
        virtual DescriptorFlags GetItemSetter(uint32 index, Var* setterValue, ScriptContext* requestContext) override;
        virtual BOOL SetItem(uint32 index, Var value, PropertyOperationFlags flags) override;
        virtual BOOL DeleteItem(uint32 index, PropertyOperationFlags flags) override;
        virtual BOOL GetEnumerator(JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext* requestContext, ForInCache * forInCache = nullptr) override;
        virtual BOOL SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags = PropertyOperation_None) override;
        virtual BOOL Equals(__in Var other, __out BOOL* value, ScriptContext* requestContext) override;
        virtual BOOL StrictEquals(__in Var other, __out BOOL* value, ScriptContext* requestContext) override;
        virtual BOOL IsWritable(PropertyId propertyId) override;
        virtual BOOL IsConfigurable(PropertyId propertyId) override;
        virtual BOOL IsEnumerable(PropertyId propertyId) override;
        virtual BOOL IsExtensible() override;
        virtual BOOL PreventExtensions() override;
        virtual void ThrowIfCannotDefineProperty(PropertyId propId, const PropertyDescriptor& descriptor) { }
        virtual void ThrowIfCannotGetOwnPropertyDescriptor(PropertyId propId) {};
        virtual BOOL GetDefaultPropertyDescriptor(PropertyDescriptor& descriptor) override;
        virtual BOOL Seal() override;
        virtual BOOL Freeze() override;
        virtual BOOL IsSealed() override;
        virtual BOOL IsFrozen() override;
        virtual BOOL SetWritable(PropertyId propertyId, BOOL value) override;
        virtual BOOL SetConfigurable(PropertyId propertyId, BOOL value) override;
        virtual BOOL SetEnumerable(PropertyId propertyId, BOOL value) override;
        virtual BOOL SetAttributes(PropertyId propertyId, PropertyAttributes attributes) override;
        virtual BOOL GetSpecialPropertyName(uint32 index, JavascriptString ** propertyName, ScriptContext * requestContext) { return false; }
        virtual uint GetSpecialPropertyCount() const { return 0; }
        virtual PropertyId const * GetSpecialPropertyIds() const { return nullptr; }
        virtual BOOL HasInstance(Var instance, ScriptContext* scriptContext, IsInstInlineCache* inlineCache = NULL) override;
        // This is used for external object only; should not be called for proxy
        virtual RecyclableObject* GetConfigurablePrototype(ScriptContext * requestContext) override;
        virtual RecyclableObject* GetPrototypeSpecial() override;
        // for external object. don't need it here.
        virtual Js::JavascriptString* GetClassName(ScriptContext * requestContext) override;

#if DBG
        virtual bool CanStorePropertyValueDirectly(PropertyId propertyId, bool allowLetConst) { Assert(false); return false; };
#endif

        virtual void RemoveFromPrototype(ScriptContext * requestContext) override;
        virtual void AddToPrototype(ScriptContext * requestContext) override;
        virtual void SetPrototype(RecyclableObject* newPrototype) override;

        BOOL SetPrototypeTrap(RecyclableObject* newPrototype, bool showThrow, ScriptContext * requestContext);
        Var ToString(Js::ScriptContext* scriptContext);

        // proxy does not support IDispatch stuff.
        virtual Var GetNamespaceParent(Js::Var aChild) { AssertMsg(false, "Shouldn't call this implementation."); return nullptr; }
        virtual HRESULT QueryObjectInterface(REFIID riid, void **ppvObj) { AssertMsg(false, "Shouldn't call this implementation."); return E_NOTIMPL; }

        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;
        virtual RecyclableObject* ToObject(ScriptContext * requestContext) override;
        virtual Var GetTypeOfString(ScriptContext* requestContext) override;

        BOOL SetPropertyTrap(Var receiver, SetPropertyTrapKind setPropertyTrapKind, PropertyId propertyId, Var newValue, ScriptContext* requestContext, BOOL skipPrototypeCheck = FALSE);
        BOOL SetPropertyTrap(Var receiver, SetPropertyTrapKind setPropertyTrapKind, Js::JavascriptString * propertyString, Var newValue, ScriptContext* requestContext);

        void PropertyIdFromInt(uint32 index, PropertyRecord const** propertyRecord);

        JavascriptArray* PropertyKeysTrap(KeysTrapKind keysTrapKind, ScriptContext* requestContext);

        template <class Fn>
        void GetOwnPropertyKeysHelper(ScriptContext* scriptContext, RecyclableObject* trapResultArray, uint32 len, JavascriptArray* trapResult,
            JsUtil::BaseDictionary<Js::PropertyId, bool, ArenaAllocator>& targetToTrapResultMap, Fn fn)
        {
            Var element = nullptr;
            const PropertyRecord* propertyRecord;
            uint32 trapResultIndex = 0;
            PropertyId propertyId;
            for (uint32 i = 0; i < len; i++)
            {
                if (!JavascriptOperators::GetItem(trapResultArray, i, &element, scriptContext))
                    continue;

                if (!(JavascriptString::Is(element) || JavascriptSymbol::Is(element)))
                {
                    JavascriptError::ThrowTypeError(scriptContext, JSERR_InconsistentTrapResult, _u("ownKeys"));
                }

                JavascriptConversion::ToPropertyKey(element, scriptContext, &propertyRecord);
                propertyId = propertyRecord->GetPropertyId();

                if (!targetToTrapResultMap.ContainsKey(propertyId))
                {
                    if (propertyId != Constants::NoProperty)
                    {
                        targetToTrapResultMap.Add(propertyId, true);
                    }
                }

                // We explicitly allow duplicates in the results. A map is sufficient since the spec steps that remove entries
                // remove ALL of them at the same time.
                if (fn(propertyRecord))
                {
                    trapResult->DirectSetItemAt(trapResultIndex++, element);
                }
            }
        }

        Var ConstructorTrap(Arguments args, ScriptContext* scriptContext, const Js::AuxArray<uint32> *spreadIndices);


#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        static PropertyId EnsureHandlerPropertyId(ScriptContext* scriptContext);
        static RecyclableObject* AutoProxyWrapper(Var obj);
#endif

    private:
        JavascriptFunction* GetMethodHelper(PropertyId methodId, ScriptContext* requestContext);
        Var GetValueFromDescriptor(Var instance, PropertyDescriptor propertyDescriptor, ScriptContext* requestContext);
        static Var GetName(ScriptContext* requestContext, PropertyId propertyId);

        RecyclableObject* MarshalHandler(ScriptContext* requestContext) const
        {
            return static_cast<RecyclableObject*>(
                CrossSite::MarshalVar(requestContext, handler));
        }
        RecyclableObject* MarshalTarget(ScriptContext* requestContext) const
        {
            return static_cast<RecyclableObject*>(
                CrossSite::MarshalVar(requestContext, target));
        }

        static BOOL TestIntegrityLevel(IntegrityLevel integrityLevel, RecyclableObject* obj, ScriptContext* scriptContext);
        static BOOL SetIntegrityLevel(IntegrityLevel integrityLevel, RecyclableObject* obj, ScriptContext* scriptContext);

        template <class Fn, class GetPropertyIdFunc>
        BOOL HasPropertyTrap(Fn fn, GetPropertyIdFunc getPropertyId);

        template <class Fn, class GetPropertyIdFunc>
        BOOL GetPropertyTrap(Var instance, PropertyDescriptor* propertyDescriptor, Fn fn, GetPropertyIdFunc getPropertyId, ScriptContext* requestContext);

        template <class Fn, class GetPropertyIdFunc>
        BOOL GetPropertyDescriptorTrap(Var originalInstance, Fn fn, GetPropertyIdFunc getPropertyId, PropertyDescriptor* resultDescriptor, ScriptContext* requestContext);

#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };
}

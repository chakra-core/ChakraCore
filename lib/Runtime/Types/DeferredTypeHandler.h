//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class DeferredTypeHandlerBase : public DynamicTypeHandler
    {
    public:
        DEFINE_GETCPPNAME();

    protected:
        DeferredTypeHandlerBase(bool isPrototype, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots) :
            DynamicTypeHandler(0, inlineSlotCapacity, offsetOfInlineSlots, DefaultFlags | IsLockedFlag | MayBecomeSharedFlag | IsSharedFlag | (isPrototype ? IsPrototypeFlag : 0))
        {
            SetIsInlineSlotCapacityLocked();
            this->ClearHasOnlyWritableDataProperties(); // Until the type handler is initialized, we cannot
                                                        // be certain that the type has only writable data properties.
        }

        DeferredTypeHandlerBase(Recycler * recycler, DeferredTypeHandlerBase * base);

    public:
        void Convert(DynamicObject * instance, DynamicTypeHandler * handler);
        void Convert(DynamicObject * instance, DeferredInitializeMode mode, int initSlotCapacity,  BOOL hasAccessor = false);

        virtual void SetAllPropertiesToUndefined(DynamicObject* instance, bool invalidateFixedFields) override {};
        virtual void MarshalAllPropertiesToScriptContext(DynamicObject* instance, ScriptContext* targetScriptContext, bool invalidateFixedFields) override {};

        virtual BOOL IsDeferredTypeHandler() const override { return TRUE; }

        virtual void SetIsPrototype(DynamicObject* instance) override { Assert(false); }
#if DBG
        virtual bool SupportsPrototypeInstances() const { Assert(false); return false; }
        virtual bool RespectsIsolatePrototypes() const { return false; }
        virtual bool RespectsChangeTypeOnProto() const { return false; }
#endif

    private:
        template <typename T>
        T* ConvertToTypeHandler(DynamicObject* instance, int initSlotCapacity, BOOL isProto = FALSE);

        DictionaryTypeHandler * ConvertToDictionaryType(DynamicObject* instance, int initSlotCapacity, BOOL isProto);
        SimpleDictionaryTypeHandler * ConvertToSimpleDictionaryType(DynamicObject* instance, int initSlotCapacity, BOOL isProto);
        ES5ArrayTypeHandler * ConvertToES5ArrayType(DynamicObject* instance, int initSlotCapacity);

#if ENABLE_TTD
    public:
        virtual void MarkObjectSlots_TTD(TTD::SnapshotExtractor* extractor, DynamicObject* obj) const override
        {
            ;
        }

        virtual uint32 ExtractSlotInfo_TTD(TTD::NSSnapType::SnapHandlerPropertyEntry* entryInfo, ThreadContext* threadContext, TTD::SlabAllocator& alloc) const override
        {
            return 0;
        }
#endif
    public:
        virtual DynamicTypeHandler* ConvertToExternalDataSupport(Recycler* recycler) override { AssertMsg(false, "Why Base class?"); return nullptr; }
    };

    class DefaultDeferredTypeFilter
    {
    public:
        static bool HasFilter() { return false; }
        static bool HasProperty(PropertyId propertyId) { Assert(false); return false; }
    };


#define DEFERRED_TYPE_HANDLER_TEMPLATE template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter = DefaultDeferredTypeFilter, bool isPrototypeTemplate = false, uint16 _inlineSlotCapacity = 0, uint16 _offsetOfInlineSlots = 0>
    class DeferredTypeHandler : public DeferredTypeHandlerBase
    {
        friend class DynamicTypeHandler;

        DEFERRED_TYPE_HANDLER_TEMPLATE friend class DeferredTypeHandlerWithExternal;

    public:
        DEFINE_GETCPPNAME();

    private:
        DeferredTypeHandler() : DeferredTypeHandlerBase(isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots) { }
        DeferredTypeHandler(Recycler * recycler, DeferredTypeHandler* base):
        DeferredTypeHandlerBase(recycler, base) { }

    public:
        static DeferredTypeHandler *GetDefaultInstance() { return &defaultInstance; }

        virtual BOOL IsLockable() const override { return true; }
        virtual BOOL IsSharable() const override { return true; }
        virtual int GetPropertyCount() override;
        virtual PropertyId GetPropertyId(ScriptContext* scriptContext, PropertyIndex index) override;
        virtual PropertyId GetPropertyId(ScriptContext* scriptContext, BigPropertyIndex index) override;
        virtual BOOL FindNextProperty(ScriptContext* scriptContext, PropertyIndex& index, JavascriptString** propertyString,
            PropertyId* propertyId, PropertyAttributes* attributes, Type* type, DynamicType *typeToEnumerate, EnumeratorFlags flags, DynamicObject* instance, PropertyValueInfo* info) override;
        virtual PropertyIndex GetPropertyIndex(PropertyRecord const* propertyRecord) override;
        virtual bool GetPropertyEquivalenceInfo(PropertyRecord const* propertyRecord, PropertyEquivalenceInfo& info) override;
        virtual bool IsObjTypeSpecEquivalent(const Type* type, const TypeEquivalenceRecord& record, uint& failedPropertyIndex) override;
        virtual bool IsObjTypeSpecEquivalent(const Type* type, const EquivalentPropertyEntry* entry) override;
        virtual bool EnsureObjectReady(DynamicObject* instance) override;
        virtual BOOL HasProperty(DynamicObject* instance, PropertyId propertyId, __out_opt bool *noRedecl = nullptr) override;
        virtual BOOL HasProperty(DynamicObject* instance, JavascriptString* propertyNameString) override;
        virtual BOOL GetProperty(DynamicObject* instance, Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL GetProperty(DynamicObject* instance, Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL SetProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetProperty(DynamicObject* instance, JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetInternalProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyOperationFlags flags) override;
        virtual DescriptorFlags GetSetter(DynamicObject* instance, PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual DescriptorFlags GetSetter(DynamicObject* instance, JavascriptString* propertyNameString, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL DeleteProperty(DynamicObject* instance, PropertyId propertyId, PropertyOperationFlags flags) override;

        virtual BOOL HasItem(DynamicObject* instance, uint32 index);
        virtual BOOL SetItem(DynamicObject* instance, uint32 index, Var value, PropertyOperationFlags flags) override;
        virtual BOOL SetItemWithAttributes(DynamicObject* instance, uint32 index, Var value, PropertyAttributes attributes) override;
        virtual BOOL SetItemAttributes(DynamicObject* instance, uint32 index, PropertyAttributes attributes) override;
        virtual BOOL SetItemAccessors(DynamicObject* instance, uint32 index, Var getter, Var setter) override;
        virtual BOOL DeleteItem(DynamicObject* instance, uint32 index, PropertyOperationFlags flags) override;
        virtual BOOL GetItem(DynamicObject* instance, Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override;
        virtual DescriptorFlags GetItemSetter(DynamicObject* instance, uint32 index, Var* setterValue, ScriptContext* requestContext) override;

        virtual BOOL IsEnumerable(DynamicObject* instance, PropertyId propertyId) override;
        virtual BOOL IsWritable(DynamicObject* instance, PropertyId propertyId) override;
        virtual BOOL IsConfigurable(DynamicObject* instance, PropertyId propertyId) override;
        virtual BOOL IsFrozen(DynamicObject *instance) override;
        virtual BOOL IsSealed(DynamicObject *instance) override;
        virtual BOOL SetEnumerable(DynamicObject* instance, PropertyId propertyId, BOOL value) override;
        virtual BOOL SetWritable(DynamicObject* instance, PropertyId propertyId, BOOL value) override;
        virtual BOOL SetConfigurable(DynamicObject* instance, PropertyId propertyId, BOOL value) override;
        virtual BOOL SetAccessors(DynamicObject* instance, PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags = PropertyOperation_None) override;
        virtual BOOL GetAccessors(DynamicObject* instance, PropertyId propertyId, Var *getter, Var *setter) override;
        virtual BOOL PreventExtensions(DynamicObject *instance) override;
        virtual BOOL Seal(DynamicObject *instance) override;
        virtual BOOL SetPropertyWithAttributes(DynamicObject* instance, PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags = PropertyOperation_None, SideEffects possibleSideEffects = SideEffects_Any) override;
        virtual BOOL SetAttributes(DynamicObject* instance, PropertyId propertyId, PropertyAttributes attributes) override;
        virtual BOOL GetAttributesWithPropertyIndex(DynamicObject * instance, PropertyId propertyId, BigPropertyIndex index, PropertyAttributes * attributes) override;

        virtual DynamicTypeHandler* ConvertToTypeWithItemAttributes(DynamicObject* instance) override;

        virtual void SetIsPrototype(DynamicObject* instance) override;

#if DBG
        virtual bool SupportsPrototypeInstances() const { return isPrototypeTemplate; }
#endif

    private:
        static DeferredTypeHandler defaultInstance;
        bool EnsureObjectReady(DynamicObject* instance, DeferredInitializeMode mode);
        virtual BOOL FreezeImpl(DynamicObject *instance, bool isConvertedType) override;

    public:
        virtual DynamicTypeHandler* ConvertToExternalDataSupport(Recycler* recycler) override;
    };

#define DeferredTypeHandlerTypeDef DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>
#define DeferredTypeHandlerWithExternalTypeDef DeferredTypeHandlerWithExternal<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter = DefaultDeferredTypeFilter, bool isPrototypeTemplate = false, uint16 _inlineSlotCapacity = 0, uint16 _offsetOfInlineSlots = 0>
    class DeferredTypeHandlerWithExternal sealed : public DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>

    {
        typedef DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots> _DeferredTypeHandlerTypeDef;
        typedef DeferredTypeHandlerWithExternal<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots> _DeferredTypeHandlerWithExternalTypeDef;
    public:
        DEFINE_GETCPPNAME();

    private:
        DeferredTypeHandlerWithExternal(Recycler * recycler, _DeferredTypeHandlerTypeDef* sth):
          _DeferredTypeHandlerTypeDef(recycler, sth) { }

    public:
        DEFINE_HANDLERWITHEXTERNAL_INTERFACE(_DeferredTypeHandlerTypeDef, _DeferredTypeHandlerWithExternalTypeDef)
    };

    DEFERRED_TYPE_HANDLER_TEMPLATE
    DynamicTypeHandler* DeferredTypeHandlerTypeDef::ConvertToExternalDataSupport(Recycler* recycler)
    {
        return (DynamicTypeHandler*)DeferredTypeHandlerWithExternalTypeDef::New(recycler, this);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    DeferredTypeHandlerTypeDef DeferredTypeHandlerTypeDef::defaultInstance;

    DEFERRED_TYPE_HANDLER_TEMPLATE
    int DeferredTypeHandlerTypeDef::GetPropertyCount()
    {
        return 0;
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    PropertyId DeferredTypeHandlerTypeDef::GetPropertyId(ScriptContext* scriptContext, PropertyIndex index)
    {
        Assert(false);
        return Constants::NoProperty;
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    PropertyId DeferredTypeHandlerTypeDef::GetPropertyId(ScriptContext* scriptContext, BigPropertyIndex index)
    {
        Assert(false);
        return Constants::NoProperty;
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::FindNextProperty(ScriptContext* scriptContext, PropertyIndex& index,
        __out JavascriptString** propertyString, __out PropertyId* propertyId, __out_opt PropertyAttributes* attributes, Type* type, DynamicType *typeToEnumerate, EnumeratorFlags flags, DynamicObject* instance, PropertyValueInfo* info)
    {
        Assert(false);
        return FALSE;
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    PropertyIndex DeferredTypeHandlerTypeDef::GetPropertyIndex(PropertyRecord const* propertyRecord)
    {
        return Constants::NoSlot;
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    bool DeferredTypeHandlerTypeDef::GetPropertyEquivalenceInfo(PropertyRecord const* propertyRecord, PropertyEquivalenceInfo& info)
    {
        info.slotIndex = Constants::NoSlot;
        info.isAuxSlot = false;
        info.isWritable = false;
        return false;
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    bool DeferredTypeHandlerTypeDef::IsObjTypeSpecEquivalent(const Type* type,
      const TypeEquivalenceRecord& record, uint& failedPropertyIndex)
    {
        uint propertyCount = record.propertyCount;
        EquivalentPropertyEntry* properties = record.properties;
        for (uint pi = 0; pi < propertyCount; pi++)
        {
            const EquivalentPropertyEntry* refInfo = &properties[pi];
            if (!this->DeferredTypeHandlerTypeDef::IsObjTypeSpecEquivalent(type, refInfo))
            {
                failedPropertyIndex = pi;
                return false;
            }
        }
        return true;
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    bool DeferredTypeHandlerTypeDef::IsObjTypeSpecEquivalent(const Type* type,
      const EquivalentPropertyEntry* entry)
    {
        if (!DeferredTypeFilter::HasFilter())
        {
            return false;
        }

        if (entry->slotIndex != Constants::NoSlot || entry->mustBeWritable || DeferredTypeFilter::HasProperty(entry->propertyId))
        {
            return false;
        }

        return true;
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    bool DeferredTypeHandlerTypeDef::EnsureObjectReady(DynamicObject* instance)
    {
        return EnsureObjectReady(instance, DeferredInitializeMode_Default);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    bool DeferredTypeHandlerTypeDef::EnsureObjectReady(DynamicObject* instance,
      DeferredInitializeMode mode)
    {
        return initializer(instance, this, mode);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::HasProperty(DynamicObject* instance,
      PropertyId propertyId, __out_opt bool *noRedecl)
    {
        if (noRedecl != nullptr)
        {
            *noRedecl = false;
        }

        if (DeferredTypeFilter::HasFilter() && DeferredTypeFilter::HasProperty(propertyId))
        {
            return TRUE;
        }

        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->HasProperty(instance, propertyId, noRedecl);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::HasProperty(DynamicObject* instance, JavascriptString* propertyNameString)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->HasProperty(instance, propertyNameString);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::GetProperty(DynamicObject* instance, Var originalInstance,
        PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        if (DeferredTypeFilter::HasFilter() && !DeferredTypeFilter::HasProperty(propertyId))
        {
            *value = requestContext->GetMissingPropertyResult();
            return FALSE;
        }

        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            *value = requestContext->GetMissingPropertyResult();

            // Return true so that we stop walking the prototype
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->GetProperty(instance, originalInstance, propertyId, value, info, requestContext);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::GetProperty(DynamicObject* instance, Var originalInstance,
        JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            *value = requestContext->GetMissingPropertyResult();

            // Return true so that we stop walking the prototype
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->GetProperty(instance, originalInstance,
          propertyNameString, value, info, requestContext);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::SetProperty(DynamicObject* instance,
      PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Set))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetProperty(instance, propertyId, value, flags, info);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::SetProperty(DynamicObject* instance,
      JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Set))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetProperty(instance, propertyNameString, value, flags, info);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::SetInternalProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyOperationFlags flags)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Set))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetInternalProperty(instance, propertyId, value, flags);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    DescriptorFlags DeferredTypeHandlerTypeDef::GetSetter(DynamicObject* instance,
      PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        if (setterValue != nullptr)
        {
            *setterValue = nullptr;
        }

        if (DeferredTypeFilter::HasFilter() && !DeferredTypeFilter::HasProperty(propertyId))
        {
            return DescriptorFlags::None;
        }

        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return DescriptorFlags::None;
        }

        return GetCurrentTypeHandler(instance)->GetSetter(instance, propertyId, setterValue, info, requestContext);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    DescriptorFlags DeferredTypeHandlerTypeDef::GetSetter(DynamicObject* instance,
      JavascriptString* propertyNameString, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        if (setterValue != nullptr)
        {
            *setterValue = nullptr;
        }

        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return DescriptorFlags::None;
        }

        return GetCurrentTypeHandler(instance)->GetSetter(instance, propertyNameString, setterValue, info, requestContext);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::DeleteProperty(DynamicObject* instance,
      PropertyId propertyId, PropertyOperationFlags flags)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->DeleteProperty(instance, propertyId, flags);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::HasItem(DynamicObject* instance, uint32 index)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->HasItem(instance, index);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::SetItem(DynamicObject* instance, uint32 index,
      Var value, PropertyOperationFlags flags)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetItem(instance, index, value, flags);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::SetItemWithAttributes(DynamicObject* instance,
      uint32 index, Var value, PropertyAttributes attributes)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }
        return GetCurrentTypeHandler(instance)->SetItemWithAttributes(instance, index, value, attributes);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::SetItemAttributes(DynamicObject* instance, uint32 index, PropertyAttributes attributes)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetItemAttributes(instance, index, attributes);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::SetItemAccessors(DynamicObject* instance, uint32 index, Var getter, Var setter)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetItemAccessors(instance, index, getter, setter);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::DeleteItem(DynamicObject* instance, uint32 index, PropertyOperationFlags flags)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->DeleteItem(instance, index, flags);
    }
    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::GetItem(DynamicObject* instance, Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            *value = requestContext->GetMissingItemResult();
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->GetItem(instance, originalInstance, index, value, requestContext);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    DescriptorFlags DeferredTypeHandlerTypeDef::GetItemSetter(DynamicObject* instance, uint32 index, Var* setterValue, ScriptContext* requestContext)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return DescriptorFlags::None;
        }

        return GetCurrentTypeHandler(instance)->GetItemSetter(instance, index, setterValue, requestContext);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::IsEnumerable(DynamicObject* instance, PropertyId propertyId)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->IsEnumerable(instance, propertyId);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::IsWritable(DynamicObject* instance, PropertyId propertyId)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->IsWritable(instance, propertyId);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::IsConfigurable(DynamicObject* instance, PropertyId propertyId)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->IsConfigurable(instance, propertyId);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::SetEnumerable(DynamicObject* instance, PropertyId propertyId, BOOL value)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetEnumerable(instance, propertyId, value);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::SetWritable(DynamicObject* instance, PropertyId propertyId, BOOL value)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetWritable(instance, propertyId, value);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::SetConfigurable(DynamicObject* instance, PropertyId propertyId, BOOL value)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetConfigurable(instance, propertyId, value);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::SetAccessors(DynamicObject* instance, PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_SetAccessors))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetAccessors(instance, propertyId, getter, setter, flags);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::GetAccessors(DynamicObject* instance, PropertyId propertyId, Var *getter, Var *setter)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->GetAccessors(instance, propertyId, getter, setter);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::IsSealed(DynamicObject *instance)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->IsSealed(instance);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::IsFrozen(DynamicObject *instance)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->IsFrozen(instance);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::PreventExtensions(DynamicObject* instance)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Extensions))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->PreventExtensions(instance);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::Seal(DynamicObject* instance)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Extensions))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->Seal(instance);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::FreezeImpl(DynamicObject* instance, bool isConvertedType)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Extensions))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->Freeze(instance, true);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::SetPropertyWithAttributes(DynamicObject* instance, PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Set))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetPropertyWithAttributes(instance, propertyId, value, attributes, info, flags, possibleSideEffects);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::SetAttributes(DynamicObject* instance, PropertyId propertyId, PropertyAttributes attributes)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Set))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetAttributes(instance, propertyId, attributes);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    BOOL DeferredTypeHandlerTypeDef::GetAttributesWithPropertyIndex(DynamicObject * instance, PropertyId propertyId, BigPropertyIndex index, PropertyAttributes * attributes)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            if (attributes)
            {
                *attributes = PropertyDynamicTypeDefaults;
            }
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->GetAttributesWithPropertyIndex(instance, propertyId, index, attributes);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    DynamicTypeHandler* DeferredTypeHandlerTypeDef::ConvertToTypeWithItemAttributes(DynamicObject* instance)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Set))
        {
            AssertOrFailFastMsg(false, "Is this a valid scenario?");
            return nullptr;
        }

        return GetCurrentTypeHandler(instance)->ConvertToTypeWithItemAttributes(instance);
    }

    DEFERRED_TYPE_HANDLER_TEMPLATE
    void DeferredTypeHandlerTypeDef::SetIsPrototype(DynamicObject* instance)
    {
        if (!isPrototypeTemplate)
        {
            // We don't force a type transition even when ChangeTypeOnProto() == true, because objects with NullTypeHandlers don't
            // have any properties, so there is nothing to invalidate.  Types with NullTypeHandlers also aren't cached in typeWithoutProperty
            // caches, so there will be no fast property add path that could skip prototype cache invalidation.
            DeferredTypeHandlerBase* protoTypeHandler = DeferredTypeHandler<initializer, DeferredTypeFilter, true, _inlineSlotCapacity, _offsetOfInlineSlots>::GetDefaultInstance();
            AssertMsg(protoTypeHandler->GetFlags() == (GetFlags() | IsPrototypeFlag), "Why did we change the flags of a DeferredTypeHandler?");
            Assert(this->GetIsInlineSlotCapacityLocked() == protoTypeHandler->GetIsInlineSlotCapacityLocked());
            protoTypeHandler->SetPropertyTypes(PropertyTypesWritableDataOnly | PropertyTypesWritableDataOnlyDetection, GetPropertyTypes());
            SetInstanceTypeHandler(instance, protoTypeHandler);
        }
    }

}

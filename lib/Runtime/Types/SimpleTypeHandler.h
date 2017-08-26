//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    template<size_t size>
    class SimpleTypeHandler sealed: public DynamicTypeHandler
    {
        friend class NullTypeHandlerBase;
    private:
        Field(int) propertyCount;
        Field(SimplePropertyDescriptor) descriptors[size];

    public:
        DEFINE_GETCPPNAME();

    private:
        SimpleTypeHandler(Recycler*);        // only used by NullTypeHandler
        SimpleTypeHandler(SimpleTypeHandler<size> * typeHandler);
        DEFINE_VTABLE_CTOR_NO_REGISTER(SimpleTypeHandler, DynamicTypeHandler);

    public:
        SimpleTypeHandler(NO_WRITE_BARRIER_TAG_TYPE(const PropertyRecord* id), PropertyAttributes attributes = PropertyNone, PropertyTypes propertyTypes = PropertyTypesNone, uint16 inlineSlotCapacity = 0, uint16 offsetOfInlineSlots = 0);
        // Constructor of a shared typed handler

        SimpleTypeHandler(NO_WRITE_BARRIER_TAG_TYPE(SimplePropertyDescriptor const (&SharedFunctionPropertyDescriptors)[size]), PropertyTypes propertyTypes = PropertyTypesNone, uint16 inlineSlotCapacity = 0, uint16 offsetOfInlineSlots = 0);

        virtual BOOL IsLockable() const override { return true; }
        virtual BOOL IsSharable() const override { return true; }
        virtual int GetPropertyCount() override;
        virtual PropertyId GetPropertyId(ScriptContext* scriptContext, PropertyIndex index) override;
        virtual PropertyId GetPropertyId(ScriptContext* scriptContext, BigPropertyIndex index) override;
        virtual BOOL FindNextProperty(ScriptContext* scriptContext, PropertyIndex& index, JavascriptString** propertyString,
            PropertyId* propertyId, PropertyAttributes* attributes, Type* type, DynamicType *typeToEnumerate, EnumeratorFlags flags, DynamicObject* instance, PropertyValueInfo* info) override;
        virtual PropertyIndex GetPropertyIndex(PropertyRecord const* propertyRecord) override;
#if ENABLE_NATIVE_CODEGEN
        virtual bool GetPropertyEquivalenceInfo(PropertyRecord const* propertyRecord, PropertyEquivalenceInfo& info) override;
        virtual bool IsObjTypeSpecEquivalent(const Type* type, const TypeEquivalenceRecord& record, uint& failedPropertyIndex) override;
        virtual bool IsObjTypeSpecEquivalent(const Type* type, const EquivalentPropertyEntry* entry) override;
#endif
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
        virtual BOOL IsEnumerable(DynamicObject* instance, PropertyId propertyId) override;
        virtual BOOL IsWritable(DynamicObject* instance, PropertyId propertyId) override;
        virtual BOOL IsConfigurable(DynamicObject* instance, PropertyId propertyId) override;
        virtual BOOL SetEnumerable(DynamicObject* instance, PropertyId propertyId, BOOL value) override;
        virtual BOOL SetWritable(DynamicObject* instance, PropertyId propertyId, BOOL value) override;
        virtual BOOL SetConfigurable(DynamicObject* instance, PropertyId propertyId, BOOL value) override;
        virtual BOOL SetAccessors(DynamicObject* instance, PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags = PropertyOperation_None) override;
        virtual BOOL PreventExtensions(DynamicObject *instance) override;
        virtual BOOL Seal(DynamicObject* instance) override;
        virtual BOOL SetPropertyWithAttributes(DynamicObject* instance, PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags = PropertyOperation_None, SideEffects possibleSideEffects = SideEffects_Any) override;
        virtual BOOL SetAttributes(DynamicObject* instance, PropertyId propertyId, PropertyAttributes attributes) override;
        virtual BOOL GetAttributesWithPropertyIndex(DynamicObject * instance, PropertyId propertyId, BigPropertyIndex index, PropertyAttributes * attributes) override;
        virtual void SetAllPropertiesToUndefined(DynamicObject* instance, bool invalidateFixedFields) override;
        virtual void MarshalAllPropertiesToScriptContext(DynamicObject* instance, ScriptContext* targetScriptContext, bool invalidateFixedFields) override;
        virtual DynamicTypeHandler* ConvertToTypeWithItemAttributes(DynamicObject* instance) override;

        virtual void SetIsPrototype(DynamicObject* instance) override;

#if DBG
        virtual bool SupportsPrototypeInstances() const { return !ChangeTypeOnProto() && !(GetIsOrMayBecomeShared() && IsolatePrototypes()); }
        virtual bool CanStorePropertyValueDirectly(const DynamicObject* instance, PropertyId propertyId, bool allowLetConst) override;
#if ENABLE_FIXED_FIELDS        
        bool HasSingletonInstanceOnlyIfNeeded() const
        {
            // If we add support for fixed fields to this type handler we will have to update this implementation.
            return true;
        }
#endif
#endif

    private:
        template <typename T>
        T* ConvertToTypeHandler(DynamicObject* instance);

        DictionaryTypeHandler* ConvertToDictionaryType(DynamicObject* instance);
        SimpleDictionaryTypeHandler* ConvertToSimpleDictionaryType(DynamicObject* instance);
        ES5ArrayTypeHandler* ConvertToES5ArrayType(DynamicObject* instance);
        SimpleTypeHandler<size>* ConvertToNonSharedSimpleType(DynamicObject * instance);

        BOOL GetDescriptor(PropertyId propertyId, int * index);
        BOOL SetAttribute(DynamicObject* instance, int index, PropertyAttributes attribute);
        BOOL ClearAttribute(DynamicObject* instance, int index, PropertyAttributes attribute);
        BOOL AddProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects);
        virtual BOOL FreezeImpl(DynamicObject* instance, bool isConvertedType) override;

#if ENABLE_TTD
    public:
        virtual void MarkObjectSlots_TTD(TTD::SnapshotExtractor* extractor, DynamicObject* obj) const override;

        virtual uint32 ExtractSlotInfo_TTD(TTD::NSSnapType::SnapHandlerPropertyEntry* entryInfo, ThreadContext* threadContext, TTD::SlabAllocator& alloc) const override;

        virtual Js::BigPropertyIndex GetPropertyIndex_EnumerateTTD(const Js::PropertyRecord* pRecord) override;
#endif
    };

    typedef SimpleTypeHandler<1> SimpleTypeHandlerSize1;
    typedef SimpleTypeHandler<2> SimpleTypeHandlerSize2;

}

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    typedef uint8 PathTypeSetterSlotIndex;
    static const PathTypeSetterSlotIndex NoSetterSlot = (PathTypeSetterSlotIndex)-1;

    class PathTypeSuccessorKey
    {
    private:
        Field(PropertyId) propertyId;
        Field(ObjectSlotAttributes) attributes;

    public:
        PathTypeSuccessorKey();
        PathTypeSuccessorKey(const PropertyId propertyId, const ObjectSlotAttributes attributes);

    public:
        bool HasInfo() const;
        void Clear();
        PropertyId GetPropertyId() const;
        ObjectSlotAttributes GetAttributes() const;

    public:
        bool operator ==(const PathTypeSuccessorKey &other) const;
        bool operator !=(const PathTypeSuccessorKey &other) const;
        hash_t GetHashCode() const;
        operator hash_t() const { return GetHashCode(); }
    };

    class PathTypeSuccessorInfo
    {
    public:
        bool IsSingleSuccessor() const { return kind == PathTypeSuccessorKindSingle; }
        bool IsMultiSuccessor() const { return !IsSingleSuccessor(); }
        bool GetSuccessor(const PathTypeSuccessorKey successorKey, RecyclerWeakReference<DynamicType> ** typeWeakRef) const;
        void SetSuccessor(DynamicType * type, const PathTypeSuccessorKey successorKey, RecyclerWeakReference<DynamicType> * typeWeakRef, ScriptContext * scriptContext);
        void ReplaceSuccessor(DynamicType * type, PathTypeSuccessorKey successorKey, RecyclerWeakReference<DynamicType> * typeWeakRef);

        template<class Fn> void MapSuccessors(Fn fn);
        template<class Fn> void MapSuccessorsUntil(Fn fn);

    protected:
        enum PathTypeSuccessorKind : byte
        {
            PathTypeSuccessorKindSingle,
            PathTypeSuccessorKindMulti
        };
        Field(PathTypeSuccessorKind) kind;
    };

    class PathTypeSingleSuccessorInfo : public PathTypeSuccessorInfo
    {
    public:
        static PathTypeSingleSuccessorInfo * New(const PathTypeSuccessorKey successorKey, RecyclerWeakReference<DynamicType> * typeWeakRef, ScriptContext * scriptContext);

        PathTypeSingleSuccessorInfo(const PathTypeSuccessorKey successorKey, RecyclerWeakReference<DynamicType> * typeWeakRef);

        bool GetSuccessor(const PathTypeSuccessorKey successorKey, RecyclerWeakReference<DynamicType> ** typeWeakRef) const;
        void SetSuccessor(DynamicType * type, const PathTypeSuccessorKey successorKey, RecyclerWeakReference<DynamicType> * typeWeakRef, ScriptContext * scriptContext);
        void ReplaceSuccessor(DynamicType * type, PathTypeSuccessorKey successorKey, RecyclerWeakReference<DynamicType> * typeWeakRef);

        template<class Fn> void MapSingleSuccessor(Fn fn);

    private:
        Field(PathTypeSuccessorKey) successorKey;
        Field(RecyclerWeakReference<DynamicType> *) successorTypeWeakRef;
    };

    class PathTypeMultiSuccessorInfo : public PathTypeSuccessorInfo
    {
    public:
        static PathTypeMultiSuccessorInfo * New(const PathTypeSuccessorKey successorKey, RecyclerWeakReference<DynamicType> * typeWeakRef, ScriptContext * scriptContext);

        PathTypeMultiSuccessorInfo(Recycler * recycler, const PathTypeSuccessorKey successorKey, RecyclerWeakReference<DynamicType> * typeWeakRef);

        bool GetSuccessor(const PathTypeSuccessorKey successorKey, RecyclerWeakReference<DynamicType> ** typeWeakRef) const;
        void SetSuccessor(DynamicType * type, const PathTypeSuccessorKey successorKey, RecyclerWeakReference<DynamicType> * typeWeakRef, ScriptContext * scriptContext);
        void ReplaceSuccessor(DynamicType * type, PathTypeSuccessorKey successorKey, RecyclerWeakReference<DynamicType> * typeWeakRef);

        template<class Fn> void MapMultiSuccessors(Fn fn);
        template<class Fn> void MapMultiSuccessorsUntil(Fn fn);

    private:
        typedef JsUtil::WeakReferenceDictionary<PathTypeSuccessorKey, DynamicType, DictionarySizePolicy<PowerOf2Policy, 1>> PropertySuccessorsMap;
        Field(PropertySuccessorsMap *) propertySuccessors;
    };

    class PathTypeHandlerBase : public DynamicTypeHandler
    {
        template<size_t size>
        friend class SimpleTypeHandler;
        friend class PathTypeHandlerNoAttr;
        friend class PathTypeHandlerWithAttr;
        friend class DynamicObject;

    protected:
        Field(DynamicType*) predecessorType; // Strong reference to predecessor type so that predecessor types remain in the cache even though they might not be used
        Field(TypePath*) typePath;
        Field(PathTypeSuccessorInfo*) successorInfo;

    public:
        DEFINE_GETCPPNAME();
        typedef JsUtil::WeaklyReferencedKeyDictionary<DynamicType, DynamicType*> TypeTransitionMap;

    protected:
        PathTypeHandlerBase(TypePath* typePath, uint16 pathLength, const PropertyIndex slotCapacity, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots, bool isLocked = false, bool isShared = false, DynamicType* predecessorType = nullptr);

        DEFINE_VTABLE_CTOR_INIT_NO_REGISTER(PathTypeHandlerBase, DynamicTypeHandler, predecessorType(nullptr), typePath(nullptr), successorInfo(nullptr));

    public:
        virtual BOOL IsLockable() const override { return true; }
        virtual BOOL IsSharable() const override { return true; }

        template<class Fn> void MapSuccessors(Fn fn);
        template<class Fn> void MapSuccessorsUntil(Fn fn);
        PathTypeSuccessorInfo * GetSuccessorInfo() const { return successorInfo; }
        void SetSuccessorInfo(PathTypeSuccessorInfo * info) { successorInfo = info; }
        void ReplaceSuccessor(DynamicType * type, PathTypeSuccessorKey key, RecyclerWeakReference<DynamicType> * typeWeakRef) { return successorInfo->ReplaceSuccessor(type, key, typeWeakRef); }

        static PropertyAttributes ObjectSlotAttributesToPropertyAttributes(const ObjectSlotAttributes attr) { return attr & ObjectSlotAttr_PropertyAttributesMask; }
        static ObjectSlotAttributes PropertyAttributesToObjectSlotAttributes(const PropertyAttributes attr) { return (ObjectSlotAttributes)(attr & ObjectSlotAttr_PropertyAttributesMask); }
        static bool ObjectSlotAttributesContains(const PropertyAttributes attr) { return attr == (attr & ObjectSlotAttr_PropertyAttributesMask); }
        static bool UsePathTypeHandlerForObjectLiteral(const PropertyIdArray *const propIds, bool *const check__proto__Ref = nullptr);
        static DynamicType* CreateTypeForNewScObject(ScriptContext* scriptContext, DynamicType* type, const Js::PropertyIdArray *propIds, bool shareType);
        template <bool skipLetAttrForArguments> static DynamicType* CreateNewScopeObject(ScriptContext* scriptContext, DynamicType* type, const Js::PropertyIdArray *propIds, PropertyAttributes extraAttributes = PropertyNone, uint extraAttributesSlotCount = UINT_MAX);

        static PathTypeHandlerBase * FromTypeHandler(DynamicTypeHandler * const typeHandler) { Assert(typeHandler->IsPathTypeHandler()); return static_cast<PathTypeHandlerBase*>(typeHandler); }

        virtual int GetPropertyCount() override;
        virtual PropertyId GetPropertyId(ScriptContext* scriptContext, PropertyIndex index) override;
        virtual PropertyId GetPropertyId(ScriptContext* scriptContext, BigPropertyIndex index) override;
        virtual PropertyIndex GetPropertyIndex(const PropertyRecord* propertyRecord) override;
#if ENABLE_NATIVE_CODEGEN
        virtual bool GetPropertyEquivalenceInfo(PropertyRecord const* propertyRecord, PropertyEquivalenceInfo& info) override;
        virtual bool IsObjTypeSpecEquivalent(const Type* type, const TypeEquivalenceRecord& record, uint& failedPropertyIndex) override;
        virtual bool IsObjTypeSpecEquivalent(const Type* type, const EquivalentPropertyEntry* entry) override;
#endif
        virtual BOOL HasProperty(DynamicObject* instance, PropertyId propertyId, __out_opt bool *noRedecl = nullptr, _Inout_opt_ PropertyValueInfo* info = nullptr) override;
        virtual BOOL HasProperty(DynamicObject* instance, JavascriptString* propertyNameString) override;
        virtual BOOL GetProperty(DynamicObject* instance, Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL GetProperty(DynamicObject* instance, Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL SetProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetProperty(DynamicObject* instance, JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL InitProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL DeleteProperty(DynamicObject* instance, PropertyId propertyId, PropertyOperationFlags flags) override;

        virtual BOOL FindNextProperty(ScriptContext* scriptContext, PropertyIndex& index, JavascriptString** propertyString,
            PropertyId* propertyId, PropertyAttributes* attributes, Type* type, DynamicType *typeToEnumerate, EnumeratorFlags flags, DynamicObject* instance, PropertyValueInfo* info) override
        {
            return FindNextPropertyHelper(scriptContext, nullptr, index, propertyString, propertyId, attributes, type, typeToEnumerate, flags, instance, info);
        }

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

        virtual void ResetTypeHandler(DynamicObject * instance) override;
        virtual void SetAllPropertiesToUndefined(DynamicObject* instance, bool invalidateFixedFields) override;
        virtual void MarshalAllPropertiesToScriptContext(DynamicObject* instance, ScriptContext* targetScriptContext, bool invalidateFixedFields) override;
        virtual DynamicTypeHandler* ConvertToTypeWithItemAttributes(DynamicObject* instance) override;
        virtual BOOL AllPropertiesAreEnumerable() override { return true; }
        virtual BOOL IsPathTypeHandler() const { return TRUE; }
#if DBG
        virtual bool IsPathTypeHandlerWithAttr() const { return false; }
#endif

        virtual void ShrinkSlotAndInlineSlotCapacity() override;
        virtual void LockInlineSlotCapacity() override;
        virtual void EnsureInlineSlotCapacityIsLocked() override;
        virtual void VerifyInlineSlotCapacityIsLocked() override;
        void EnsureInlineSlotCapacityIsLocked(bool startFromRoot);
        void VerifyInlineSlotCapacityIsLocked(bool startFromRoot);
        PathTypeHandlerBase *DeoptimizeObjectHeaderInlining(JavascriptLibrary *const library);
        virtual void SetPrototype(DynamicObject* instance, RecyclableObject* newPrototype) override;

        virtual void SetIsPrototype(DynamicObject* instance) override;

        BOOL FindNextPropertyHelper(ScriptContext* scriptContext, ObjectSlotAttributes * objectAttributes, PropertyIndex& index, JavascriptString** propertyString,
            PropertyId* propertyId, PropertyAttributes* attributes, Type* type, DynamicType *typeToEnumerate, EnumeratorFlags flags, DynamicObject* instance, PropertyValueInfo* info);
        BOOL SetAttributesAtIndex(DynamicObject* instance, PropertyId propertyId, PropertyIndex index, PropertyAttributes attributes);
        BOOL SetAttributesHelper(DynamicObject* instance, PropertyId propertyId, PropertyIndex propertyIndex, ObjectSlotAttributes * instanceAttributes, ObjectSlotAttributes propertyAttributes, bool setAllAttributes = false);
        BOOL SetAccessorsHelper(DynamicObject* instance, PropertyId propertyId, ObjectSlotAttributes * attributes, PathTypeSetterSlotIndex * setters, Var getter, Var setter, PropertyOperationFlags flags);

#if ENABLE_NATIVE_CODEGEN
        bool IsObjTypeSpecEquivalentHelper(const Type* type, const ObjectSlotAttributes * attributes, const TypeEquivalenceRecord& record, uint& failedPropertyIndex);
        bool IsObjTypeSpecEquivalentHelper(const Type* type, const ObjectSlotAttributes * attributes, const EquivalentPropertyEntry* entry);
#endif

#if DBG
        virtual bool SupportsPrototypeInstances() const { return !IsolatePrototypes(); }
        virtual bool CanStorePropertyValueDirectly(const DynamicObject* instance, PropertyId propertyId, bool allowLetConst) override;
#endif

        DynamicTypeHandler* ConvertToNonShareableTypeHandler(DynamicObject* instance);

#if ENABLE_FIXED_FIELDS
        virtual void DoShareTypeHandler(ScriptContext* scriptContext) override;
        virtual BOOL IsFixedProperty(const DynamicObject* instance, PropertyId propertyId) override;
        virtual bool HasSingletonInstance() const override sealed;
        virtual bool TryUseFixedProperty(PropertyRecord const * propertyRecord, Var * pProperty, FixedPropertyKind propertyType, ScriptContext * requestContext) override;
        virtual bool TryUseFixedAccessor(PropertyRecord const * propertyRecord, Var * pAccessor, FixedPropertyKind propertyType, bool getter, ScriptContext * requestContext) override;

#if DBG
        bool HasOnlyInitializedNonFixedProperties();
        virtual bool CheckFixedProperty(PropertyRecord const * propertyRecord, Var * pProperty, ScriptContext * requestContext) override;
        virtual bool HasAnyFixedProperties() const override;
#endif

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        virtual void DumpFixedFields() const override;
        static void TraceFixedFieldsBeforeTypeHandlerChange(
            const char16* conversionName, const char16* oldTypeHandlerName, const char16* newTypeHandlerName,
            DynamicObject* instance, DynamicTypeHandler* oldTypeHandler, DynamicType* oldType, RecyclerWeakReference<DynamicObject>* oldSingletonInstanceBefore);
        static void TraceFixedFieldsAfterTypeHandlerChange(
            DynamicObject* instance, DynamicTypeHandler* oldTypeHandler, DynamicTypeHandler* newTypeHandler,
            DynamicType* oldType, DynamicType* newType, RecyclerWeakReference<DynamicObject>* oldSingletonInstanceBefore);
        static void TraceFixedFieldsBeforeSetIsProto(
            DynamicObject* instance, DynamicTypeHandler* oldTypeHandler, DynamicType* oldType, RecyclerWeakReference<DynamicObject>* oldSingletonInstanceBefore);
        static void TraceFixedFieldsAfterSetIsProto(
            DynamicObject* instance, DynamicTypeHandler* oldTypeHandler, DynamicTypeHandler* newTypeHandler,
            DynamicType* oldType, DynamicType* newType, RecyclerWeakReference<DynamicObject>* oldSingletonInstanceBefore);
#endif
    private:
        static bool FixPropsOnPathTypes()
        {
#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
            return CONFIG_FLAG(FixPropsOnPathTypes);
#else
            return false;
#endif
        }

        template <bool allowNonExistent, bool markAsUsed>
        bool TryGetFixedProperty(PropertyRecord const * propertyRecord, Var * pProperty, Js::FixedPropertyKind propertyType, ScriptContext * requestContext);

    public:
        virtual RecyclerWeakReference<DynamicObject>* GetSingletonInstance() const override sealed { return HasSingletonInstance() ? this->typePath->GetSingletonInstance() : nullptr; }

        virtual void SetSingletonInstanceUnchecked(RecyclerWeakReference<DynamicObject>* instance) override
        {
            Assert(!GetIsShared());
            this->typePath->SetSingletonInstance(instance, GetPathLength());
        }

        virtual void ClearSingletonInstance() { Assert(false); }

#if DBG
        bool HasSingletonInstanceOnlyIfNeeded() const
        {
#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
            return AreSingletonInstancesNeeded() || !this->GetTypePath()->HasSingletonInstance();
#else
            return true;
#endif
        }
#endif

    private:
        template <bool invalidateFixedFields> void DoShareTypeHandlerInternal(ScriptContext* scriptContext);

        void InvalidateFixedFieldAt(Js::PropertyId propertyId, Js::PropertyIndex index, ScriptContext* scriptContext);
        void AddBlankFieldAt(Js::PropertyId propertyId, Js::PropertyIndex index, ScriptContext* scriptContext);
        bool ProcessFixedFieldChange(DynamicObject* instance, PropertyId propertyId, PropertyIndex slotIndex, Var value, bool isNonFixed, const PropertyRecord * propertyRecord = nullptr);
#endif // ENABLE_FIXED_FIELDS

    private:
        template <typename T>
        T* ConvertToTypeHandler(DynamicObject* instance);

        DynamicType* PromoteType(DynamicObject* instance, const PathTypeSuccessorKey key, PropertyIndex* propertyIndex);

        DictionaryTypeHandler* ConvertToDictionaryType(DynamicObject* instance);
        ES5ArrayTypeHandler* ConvertToES5ArrayType(DynamicObject* instance);

        template <typename T> DynamicTypeHandler*
        TryConvertToSimpleDictionaryType(DynamicObject* instance, int propertyCapacity, bool mayBecomeShared = false);

        DynamicTypeHandler* TryConvertToSimpleDictionaryType(DynamicObject* instance, int propertyCapacity, bool mayBecomeShared = false)
        {
            return TryConvertToSimpleDictionaryType<SimpleDictionaryTypeHandler>(instance, propertyCapacity, mayBecomeShared);
        }

        template <typename T> T*
        ConvertToSimpleDictionaryType(DynamicObject* instance, int propertyCapacity, bool mayBecomeShared = false);

        SimpleDictionaryTypeHandler* ConvertToSimpleDictionaryType(DynamicObject* instance, int propertyCapacity, bool mayBecomeShared = false)
        {
            return ConvertToSimpleDictionaryType<SimpleDictionaryTypeHandler>(instance, propertyCapacity, mayBecomeShared);
        }

        BOOL AddPropertyInternal(DynamicObject * instance, PropertyId propertyId, Js::Var value, ObjectSlotAttributes attr, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects);
        BOOL AddProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects);
        template<bool setAttributes> BOOL SetPropertyInternal(DynamicObject* instance, PropertyId propertyId, PropertyIndex propertyIndex, Var value, ObjectSlotAttributes attr, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects, bool isInit = false);
        virtual BOOL FreezeImpl(DynamicObject* instance, bool isConvertedType) override;

        // Checks whether conversion to shared type is needed and performs it, then calls actual operation on the shared type.
        // Template method used for PreventExtensions, Seal, Freeze.
        // FType is functor/lambda to perform actual forced operation (such as PreventExtensionsInternal) on the shared type.
        template<typename FType>
        BOOL ConvertToSharedNonExtensibleTypeIfNeededAndCallOperation(DynamicObject* instance, const PropertyRecord* operationInternalPropertyId, FType operation);

        template <bool isObjectLiteral>
        DynamicType* PromoteType(DynamicType* type, const PathTypeSuccessorKey key, bool shareType, ScriptContext* scriptContext, DynamicObject* object = nullptr, PropertyIndex* propertyIndex = nullptr);
        ObjectSlotAttributes * UpdateAttributes(Recycler * recycler, ObjectSlotAttributes * oldAttributes, uint8 oldPathSize, uint8 newTypePathSize);
        PathTypeSetterSlotIndex * UpdateSetterSlots(Recycler * recycler, PathTypeSetterSlotIndex * oldSetters, uint8 oldPathSize, uint8 newTypePathSize);

        PropertyIndex GetPropertyIndex(PropertyId propertyId);

        void SetSlotAndCache(DynamicObject* instance, PropertyId propertyId, PropertyRecord const * record, PropertyIndex index, Var value, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects);

    protected:
        bool GetSuccessor(const PathTypeSuccessorKey successorKey, RecyclerWeakReference<DynamicType> ** typeWeakRef) const;
        void SetSuccessor(DynamicType * type, const PathTypeSuccessorKey successorKey, RecyclerWeakReference<DynamicType> * typeWeakRef, ScriptContext * scriptContext);

        uint16 GetPathLength() const { return GetUnusedBytesValue(); }
        TypePath * GetTypePath() const { return typePath; }
        virtual void SetTypePath(TypePath *typePath) { this->typePath = typePath; }
        DynamicType * GetPredecessorType() const { return predecessorType; }
        PathTypeHandlerBase* GetRootPathTypeHandler();
        bool HasAccessors() const { return this->GetSetterSlots() != nullptr; }
        bool CanConvertToSimpleDictionaryType() const { return !HasAccessors(); }

        virtual ObjectSlotAttributes * GetAttributeArray() const { return nullptr; }
        virtual ObjectSlotAttributes GetAttributes(const PropertyIndex index) const { return ObjectSlotAttr_Default; }
        virtual void SetAttributeArray(ObjectSlotAttributes * attributes) { Assert(false); }
        virtual void SetAttributes(PropertyIndex propertyIndex, ObjectSlotAttributes attr) { Assert(false); }
        virtual PathTypeSetterSlotIndex * GetSetterSlots() const { return nullptr; }
        virtual PathTypeSetterSlotIndex GetSetterSlotIndex(const PropertyIndex index) const { return NoSetterSlot; }
        virtual PathTypeSetterSlotIndex GetSetterCount() const { return 0; }
        virtual void SetSetterSlots(PathTypeSetterSlotIndex * setters) { Assert(false); }
        virtual void SetSetterSlot(PropertyIndex propertyIndex, PathTypeSetterSlotIndex setterSlot) { Assert(false); }

        void SetPropertyValueInfo(PropertyValueInfo* info, RecyclableObject* instance, PropertyIndex index, ObjectSlotAttributes attributes = ObjectSlotAttr_Writable);

#if ENABLE_FIXED_FIELDS
#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
        template<class FMarkAsFixed> void InitializePath(DynamicObject *const object, const PropertyIndex slotIndex, const PropertyIndex objectSlotCount, ScriptContext *const scriptContext, const FMarkAsFixed MarkAsFixed);
        template<class FMarkAsFixed> void InitializeNewPath(DynamicObject *const object, const PropertyIndex slotIndex, const PropertyIndex objectSlotCount, const FMarkAsFixed MarkAsFixed);
        void InitializeExistingPath(const PropertyIndex slotIndex, const PropertyIndex objectSlotCount, ScriptContext *const scriptContext);
#endif
#endif

    public:
        void ShrinkSlotAndInlineSlotCapacity(uint16 newInlineSlotCapacity);
        bool GetMaxPathLength(uint16 * maxPathLength);
        void MoveAuxSlotsToObjectHeader(DynamicObject *const object);
        BOOL DeleteLastProperty(DynamicObject *const object);

#if ENABLE_TTD
    public:
        virtual void MarkObjectSlots_TTD(TTD::SnapshotExtractor* extractor, DynamicObject* obj) const override;

        virtual uint32 ExtractSlotInfo_TTD(TTD::NSSnapType::SnapHandlerPropertyEntry* entryInfo, ThreadContext* threadContext, TTD::SlabAllocator& alloc) const override;

        virtual Js::BigPropertyIndex GetPropertyIndex_EnumerateTTD(const Js::PropertyRecord* pRecord) override;

        virtual bool IsResetableForTTD(uint32 snapMaxIndex) const override;
#endif
    };

#if ENABLE_FIXED_FIELDS
#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES

    template<class FMarkAsFixed>
    void PathTypeHandlerBase::InitializePath(
        DynamicObject *const object,
        const PropertyIndex slotIndex,
        const PropertyIndex objectSlotCount,
        ScriptContext *const scriptContext,
        const FMarkAsFixed MarkAsFixed)
    {
        Assert(slotIndex < objectSlotCount);
        Assert(objectSlotCount == slotIndex + 1);

        if(!FixPropsOnPathTypes())
        {
            return;
        }

        if(objectSlotCount <= GetTypePath()->GetMaxInitializedLength())
        {
            InitializeExistingPath(slotIndex, objectSlotCount, scriptContext);
            return;
        }
        InitializeNewPath(object, slotIndex, objectSlotCount, MarkAsFixed);
    }

    template<class FMarkAsFixed>
    void PathTypeHandlerBase::InitializeNewPath(
        DynamicObject *const object,
        const PropertyIndex slotIndex,
        const PropertyIndex objectSlotCount,
        const FMarkAsFixed MarkAsFixed)
    {
        TypePath *const typePath = GetTypePath();
        Assert(slotIndex == typePath->GetMaxInitializedLength());
        Assert(objectSlotCount > typePath->GetMaxInitializedLength());

        // We are a adding a property where no instance property has been set before.  We rely on properties being
        // added in order of indexes to be sure that we don't leave any uninitialized properties interspersed with
        // initialized ones, which could lead to incorrect behavior.  See comment in TypePath::Branch.

        if(!object)
        {
            typePath->AddBlankFieldAt(slotIndex, objectSlotCount);
            return;
        }

        // Consider: It would be nice to assert the slot is actually null.  However, we sometimes pre-initialize to
        // undefined or even some other special illegal value (for let or const, currently == null)
        // Assert(object->GetSlot(index) == null);

        if(ShouldFixAnyProperties() && CanBeSingletonInstance(object))
        {
            typePath->AddSingletonInstanceFieldAt(object, slotIndex, MarkAsFixed(), objectSlotCount);
            return;
        }

        typePath->AddSingletonInstanceFieldAt(slotIndex, objectSlotCount);
    }

#endif
#endif

    typedef SimpleDictionaryTypeHandlerBase<PropertyIndex, const PropertyRecord*, true> SimpleDictionaryTypeHandlerWithNonExtensibleSupport;

    class PathTypeHandlerNoAttr : public PathTypeHandlerBase
    {
        friend class PathTypeHandlerBase;

    public:
        DEFINE_GETCPPNAME();

    protected:
        PathTypeHandlerNoAttr(TypePath *typePath, uint16 pathLength, const PropertyIndex slotCapacity, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots, bool isLocked = false, bool isShared = false, DynamicType* predecessorType = nullptr);

        DEFINE_VTABLE_CTOR_NO_REGISTER(PathTypeHandlerNoAttr, PathTypeHandlerBase);

    public:
        static PathTypeHandlerNoAttr * New(ScriptContext * scriptContext, TypePath* typePath, uint16 pathLength, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots, bool isLocked = false, bool isShared = false, DynamicType* predecessorType = nullptr);
        static PathTypeHandlerNoAttr * New(ScriptContext * scriptContext, TypePath* typePath, uint16 pathLength, const PropertyIndex slotCapacity, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots, bool isLocked = false, bool isShared = false, DynamicType* predecessorType = nullptr);
        static PathTypeHandlerNoAttr * New(ScriptContext * scriptContext, PathTypeHandlerNoAttr * typeHandler, bool isLocked, bool isShared);
    };

    class PathTypeHandlerWithAttr : public PathTypeHandlerNoAttr
    {
        friend class PathTypeHandlerBase;

    private:
        Field(ObjectSlotAttributes *) attributes;
        Field(PathTypeSetterSlotIndex *) setters;
        Field(PathTypeSetterSlotIndex) setterCount;

    public:
        DEFINE_GETCPPNAME();

    protected:
        PathTypeHandlerWithAttr(TypePath* typePath, ObjectSlotAttributes * attributes, PathTypeSetterSlotIndex * setters, PathTypeSetterSlotIndex setterCount, uint16 pathLength, const PropertyIndex slotCapacity, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots, bool isLocked = false, bool isShared = false, DynamicType* predecessorType = nullptr);

        DEFINE_VTABLE_CTOR_INIT_NO_REGISTER(PathTypeHandlerWithAttr, PathTypeHandlerNoAttr, attributes(nullptr), setters(nullptr), setterCount(0));

    protected:
        virtual ObjectSlotAttributes * GetAttributeArray() const override { return attributes; }
        virtual ObjectSlotAttributes GetAttributes(const PropertyIndex index) const override { Assert(index < GetPathLength()); return attributes[index]; }
        virtual void SetAttributeArray(ObjectSlotAttributes * attributes) override { this->attributes = attributes; }
        virtual void SetAttributes(PropertyIndex propertyIndex, ObjectSlotAttributes attr) override { Assert(propertyIndex < GetPathLength()); this->attributes[propertyIndex] = attr; }
        virtual PathTypeSetterSlotIndex * GetSetterSlots() const override { return setters; }
        virtual PathTypeSetterSlotIndex GetSetterSlotIndex(const PropertyIndex index) const override { Assert(index < GetPathLength()); return setters[index]; }
        virtual PathTypeSetterSlotIndex GetSetterCount() const override { return setterCount; }
        virtual void SetSetterSlots(PathTypeSetterSlotIndex * setters) override { this->setters = setters; }
        virtual void SetSetterSlot(PropertyIndex propertyIndex, PathTypeSetterSlotIndex setterSlot) { Assert(attributes[propertyIndex] & ObjectSlotAttr_Accessor); setters[propertyIndex] = setterSlot; }
      
    public:
        static PathTypeHandlerWithAttr * New(ScriptContext * scriptContext, TypePath * typePath, ObjectSlotAttributes * attributes, PathTypeSetterSlotIndex * setters, PathTypeSetterSlotIndex setterCount, uint16 pathLength, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots, bool isLocked = false, bool isShared = false, DynamicType* predecessorType = nullptr);
        static PathTypeHandlerWithAttr * New(ScriptContext * scriptContext, TypePath * typePath, ObjectSlotAttributes * attributes, PathTypeSetterSlotIndex * setters, PathTypeSetterSlotIndex setterCount, uint16 pathLength, const PropertyIndex slotCapacity, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots, bool isLocked = false, bool isShared = false, DynamicType* predecessorType = nullptr);
        static PathTypeHandlerWithAttr * New(ScriptContext * scriptContext, PathTypeHandlerWithAttr * typeHandler, bool isLocked, bool isShared);

        static PathTypeHandlerWithAttr * FromPathTypeHandler(PathTypeHandlerBase * typeHandler)
        {
            Assert(typeHandler->IsPathTypeHandlerWithAttr());
            return static_cast<PathTypeHandlerWithAttr*>(typeHandler);
        }

#if DBG
        virtual bool IsPathTypeHandlerWithAttr() const override { return true; }
#endif

        virtual BOOL IsEnumerable(DynamicObject* instance, PropertyId propertyId) override;
        virtual BOOL IsWritable(DynamicObject* instance, PropertyId propertyId) override;
        virtual BOOL IsConfigurable(DynamicObject* instance, PropertyId propertyId) override;
        virtual BOOL SetEnumerable(DynamicObject* instance, PropertyId propertyId, BOOL value) override;
        virtual BOOL SetWritable(DynamicObject* instance, PropertyId propertyId, BOOL value) override;
        virtual BOOL SetConfigurable(DynamicObject* instance, PropertyId propertyId, BOOL value) override;

        virtual int GetPropertyCountForEnum() override;
        virtual BOOL HasProperty(DynamicObject* instance, PropertyId propertyId, __out_opt bool *noRedecl, _Inout_opt_ PropertyValueInfo* info) override;
        virtual BOOL GetProperty(DynamicObject* instance, Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL GetProperty(DynamicObject* instance, Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL GetAttributesWithPropertyIndex(DynamicObject * instance, PropertyId propertyId, BigPropertyIndex index, PropertyAttributes * attributes) override;

        virtual BOOL SetProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetProperty(DynamicObject* instance, JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;

        _Check_return_ _Success_(return) virtual BOOL GetAccessors(DynamicObject* instance, PropertyId propertyId, _Outptr_result_maybenull_ Var* getter, _Outptr_result_maybenull_ Var* setter) override;
        virtual BOOL SetAccessors(DynamicObject* instance, PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags = PropertyOperation_None) override;
        virtual DescriptorFlags GetSetter(DynamicObject* instance, PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual DescriptorFlags GetSetter(DynamicObject* instance, JavascriptString* propertyNameString, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;

        virtual BOOL FindNextProperty(ScriptContext* scriptContext, PropertyIndex& index, JavascriptString** propertyString,
            PropertyId* propertyId, PropertyAttributes* attributes, Type* type, DynamicType *typeToEnumerate, EnumeratorFlags flags, DynamicObject* instance, PropertyValueInfo* info) override
        {
            return FindNextPropertyHelper(scriptContext, this->attributes, index, propertyString, propertyId, attributes, type, typeToEnumerate, flags, instance, info);
        }
        virtual BOOL AllPropertiesAreEnumerable() sealed override { return false; }
#if ENABLE_NATIVE_CODEGEN
        virtual bool IsObjTypeSpecEquivalent(const Type* type, const TypeEquivalenceRecord& record, uint& failedPropertyIndex) override;
        virtual bool IsObjTypeSpecEquivalent(const Type* type, const EquivalentPropertyEntry* entry) override;
#endif

        virtual BigPropertyIndex PropertyIndexToPropertyEnumeration(BigPropertyIndex index) const override { return index - setterCount; }
    };

}

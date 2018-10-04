//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
class ScriptSite;
namespace Js
{

#if ENABLE_TTD
#define DEFINE_TTD_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(T) \
    virtual void MarshalCrossSite_TTDInflate() \
    { \
        AssertMsg(VirtualTableInfo<T>::HasVirtualTable(this), "Derived class need to define marshal"); \
        VirtualTableInfo<Js::CrossSiteObject<T>>::SetVirtualTable(this); \
    }
#else
#define DEFINE_TTD_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(T)
#endif

#if !defined(USED_IN_STATIC_LIB)
#define DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(T) \
    friend class Js::CrossSiteObject<T>; \
    virtual void MarshalToScriptContext(Js::ScriptContext * scriptContext) \
    { \
        Assert(this->GetScriptContext() != scriptContext); \
        AssertMsg(VirtualTableInfo<T>::HasVirtualTable(this), "Derived class need to define marshal to script context"); \
        VirtualTableInfo<Js::CrossSiteObject<T>>::SetVirtualTable(this); \
    }\
    DEFINE_TTD_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(T)
#else
#define DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(T)  \
        virtual void MarshalToScriptContext(Js::ScriptContext * scriptContext)  {Assert(FALSE);}
#endif

#if DBG
#define SetSlotArguments(propertyId, slotIndex, value) propertyId, false, slotIndex, value
#define SetSlotArgumentsRoot(propertyId, allowLetConst, slotIndex, value) propertyId, allowLetConst, slotIndex, value
#else
#define SetSlotArguments(propertyId, slotIndex, value) slotIndex, value
#define SetSlotArgumentsRoot(propertyId, allowLetConst, slotIndex, value) slotIndex, value
#endif

    enum class DynamicObjectFlags : uint16
    {
        None = 0u,
        ObjectArrayFlagsTag = 1u << 0,       // Tag bit used to indicate the objectArrayOrFlags field is used as flags as opposed to object array pointer.
        HasSegmentMap = 1u << 1,
        HasNoMissingValues = 1u << 2,        // The head segment of a JavascriptArray has no missing values.

        InitialArrayValue = ObjectArrayFlagsTag | HasNoMissingValues,

        AllArrayFlags = HasNoMissingValues | HasSegmentMap,
        AllFlags = ObjectArrayFlagsTag | HasNoMissingValues | HasSegmentMap
    };
    ENUM_CLASS_HELPERS(DynamicObjectFlags, uint16);

    class DynamicObject : public RecyclableObject
    {
        friend class CrossSite;
        friend class DynamicTypeHandler;
        friend class ModuleNamespace;
        friend class DynamicObjectEnumerator;
        friend class RecyclableObject;
        friend struct InlineCache;
        friend class ForInObjectEnumerator; // for cache enumerator

        friend class JavascriptArray; // for xplat offsetof field access
        friend class JavascriptNativeArray; // for xplat offsetof field access
        friend class JavascriptOperators;
        friend class JavascriptLibrary;
        friend class ModuleNamespace; // for slot setting.

#if ENABLE_OBJECT_SOURCE_TRACKING
    public:
        //Field for tracking object allocation
        TTD::DiagnosticOrigin TTDDiagOriginInfo;
#endif

    private:
        // Memory layout of DynamicObject can be one of the following:
        //        (#1)                (#2)                (#3)
        //  +--------------+    +--------------+    +--------------+
        //  | vtable, etc. |    | vtable, etc. |    | vtable, etc. |
        //  |--------------|    |--------------|    |--------------|
        //  | auxSlots     |    | auxSlots     |    | inline slots |
        //  | union        |    | union        |    |              |
        //  +--------------+    |--------------|    |              |
        //                      | inline slots |    |              |
        //                      +--------------+    +--------------+
        // The allocation size of inline slots is variable and dependent on profile data for the
        // object. The offset of the inline slots is managed by DynamicTypeHandler.
        // More details for the layout scenarios below.

        Field(Field(Var)*) auxSlots;

        // The objectArrayOrFlags field can store one of three things:
        //   a) a pointer to the object array holding numeric properties of this object (#1, #2), or
        //   b) a bitfield of flags (#1, #2), or
        //   c) inline slot data (#3)
        // Because object arrays are not commonly used, the storage space can be reused to carry information that
        // can improve performance for typical objects. To indicate the bitfield usage we set the least significant bit to 1.
        // Object array pointer always trumps the flags, such that when the first numeric property is added to an
        // object, its flags will be wiped out.  Hence flags can only be used as a form of cache to improve performance.
        // For functional correctness, some other fallback mechanism must exist to convey the information contained in flags.
        // This fields always starts off initialized to null.  Currently, only JavascriptArray overrides it to store flags, the
        // bits it uses are DynamicObjectFlags::AllArrayFlags.
        // Regarding c) above, inline slots can be stored within the allocation of sizeof(DynamicObject) (#3) or after
        // sizeof(DynamicObject) (#2). This is indicated by GetTypeHandler()->IsObjectHeaderInlinedTypeHandler(); when true, the
        // inline slots are within the object, and thus the union members *and* auxSlots actually contain inline slot data.

        union
        {
            Field(ArrayObject *) objectArray;       // Only if !IsAnyArray
            struct                                  // Only if IsAnyArray
            {
                Field(DynamicObjectFlags) arrayFlags;
                Field(ProfileId) arrayCallSiteIndex;
            };
        };

        CompileAssert(sizeof(ProfileId) == 2);
        CompileAssert(static_cast<intptr_t>(DynamicObjectFlags::ObjectArrayFlagsTag) != 0);

        void InitSlots(DynamicObject * instance, ScriptContext * scriptContext);
        void SetTypeHandler(DynamicTypeHandler * typeHandler, bool hasChanged);

        Field(Var)* GetInlineSlots() const;

    protected:
        DEFINE_VTABLE_CTOR(DynamicObject, RecyclableObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(DynamicObject);

        DynamicObject(DynamicType * type, const bool initSlots = true);
        DynamicObject(DynamicType * type, ScriptContext * scriptContext);

        // For boxing stack instance
        DynamicObject(DynamicObject * instance, bool deepCopy);

        uint16 GetOffsetOfInlineSlots() const;

        template <class T>
        static T* NewObject(Recycler * recycler, DynamicType * type);

    public:
        static DynamicObject * New(Recycler * recycler, DynamicType * type);

        // Return whether the type is exactly DynamicObject, not some subclass (for more inclusive check use VarIs)
        static bool IsBaseDynamicObject(Var aValue);

        // Returns the object if it is exactly DynamicObject, not some subclass. Otherwise returns null.
        static DynamicObject* TryVarToBaseDynamicObject(Var aValue);

        void EnsureSlots(int oldCount, int newCount, ScriptContext * scriptContext, DynamicTypeHandler * newTypeHandler = nullptr);
        void EnsureSlots(int newCount, ScriptContext *scriptContext);
        void ReplaceType(DynamicType * type);
        virtual void ReplaceTypeWithPredecessorType(DynamicType * previousType);

        DynamicTypeHandler * GetTypeHandler() const;

        Var GetSlot(int index);
        Var GetInlineSlot(int index);
        Var GetAuxSlot(int index);
#if DBG
        void SetSlot(PropertyId propertyId, bool allowLetConst, int index, Var value);
        void SetInlineSlot(PropertyId propertyId, bool allowLetConst, int index, Var value);
        void SetAuxSlot(PropertyId propertyId, bool allowLetConst, int index, Var value);
#else
        void SetSlot(int index, Var value);
        void SetInlineSlot(int index, Var value);
        void SetAuxSlot(int index, Var value);
#endif

    private:
        bool IsCompatibleForCopy(DynamicObject* from) const;

        bool IsObjectHeaderInlinedTypeHandlerUnchecked() const;
    public:
        bool IsObjectHeaderInlinedTypeHandler() const;
        bool DeoptimizeObjectHeaderInlining();

    public:
        bool HasNonEmptyObjectArray() const;
        DynamicType * GetDynamicType() const { return (DynamicType *)this->GetType(); }

        // Check if a typeId is of any array type (JavascriptArray or ES5Array).
        static bool IsAnyArrayTypeId(TypeId typeId);

        // Check if a Var is either a JavascriptArray* or ES5Array*.
        static bool IsAnyArray(const Var aValue);
        static bool IsAnyArray(DynamicObject* obj);

        // Check if a Var is a typedarray.
        static bool IsAnyTypedArray(const Var aValue);

        bool UsesObjectArrayOrFlagsAsFlags() const
        {
            return !!(arrayFlags & DynamicObjectFlags::ObjectArrayFlagsTag);
        }

        ArrayObject* GetObjectArray() const
        {
            return HasObjectArray() ? GetObjectArrayOrFlagsAsArray() : nullptr;
        }

        bool HasObjectArray() const
        {
            // Only JavascriptArray uses the objectArrayOrFlags as flags.
            Assert(DynamicObject::IsAnyArray((Var)this) || !UsesObjectArrayOrFlagsAsFlags() || IsObjectHeaderInlinedTypeHandler());
            return ((objectArray != nullptr) && !UsesObjectArrayOrFlagsAsFlags() && !IsObjectHeaderInlinedTypeHandler());
        }

        ArrayObject* GetObjectArrayUnchecked() const
        {
            return HasObjectArrayUnchecked() ? GetObjectArrayOrFlagsAsArray() : nullptr;
        }

        bool HasObjectArrayUnchecked() const
        {
            return ((objectArray != nullptr) && !UsesObjectArrayOrFlagsAsFlags() && !IsObjectHeaderInlinedTypeHandlerUnchecked());
        }

        BOOL HasObjectArrayItem(uint32 index);
        BOOL DeleteObjectArrayItem(uint32 index, PropertyOperationFlags flags);
        BOOL GetObjectArrayItem(Var originalInstance, uint32 index, Var* value, ScriptContext* requestContext);
        DescriptorFlags GetObjectArrayItemSetter(uint32 index, Var* setterValue, ScriptContext* requestContext);
        BOOL SetObjectArrayItem(uint32 index, Var value, PropertyOperationFlags flags);
        BOOL SetObjectArrayItemWithAttributes(uint32 index, Var value, PropertyAttributes attributes);
        BOOL SetObjectArrayItemAttributes(uint32 index, PropertyAttributes attributes);
        BOOL SetObjectArrayItemWritable(PropertyId propertyId, BOOL writable);
        BOOL SetObjectArrayItemAccessors(uint32 index, Var getter, Var setter);
        void InvalidateHasOnlyWritableDataPropertiesInPrototypeChainCacheIfPrototype();
        void ResetObject(DynamicType* type, BOOL keepProperties);

        bool TryCopy(DynamicObject* from);

        virtual void SetIsPrototype();

        bool HasLockedType() const;
        bool HasSharedType() const;
        bool HasSharedTypeHandler() const;
        bool LockType();
        bool ShareType();
        bool GetIsExtensible() const;
        bool GetHasNoEnumerableProperties();
        bool SetHasNoEnumerableProperties(bool value);
        virtual bool HasReadOnlyPropertiesInvisibleToTypeHandler() { return false; }

        void InitSlots(DynamicObject* instance);
        virtual int GetPropertyCount() override;
        int GetPropertyCountForEnum();
        virtual PropertyId GetPropertyId(PropertyIndex index) override;
        virtual PropertyId GetPropertyId(BigPropertyIndex index) override;
        PropertyIndex GetPropertyIndex(PropertyId propertyId) sealed;
        virtual PropertyQueryFlags HasPropertyQuery(PropertyId propertyId, _Inout_opt_ PropertyValueInfo* info) override;
        virtual BOOL HasOwnProperty(PropertyId propertyId) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL GetInternalProperty(Var instance, PropertyId internalPropertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetInternalProperty(PropertyId internalPropertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual DescriptorFlags GetSetter(PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual DescriptorFlags GetSetter(JavascriptString* propertyNameString, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags = PropertyOperation_None, PropertyValueInfo* info = nullptr) override;
        virtual BOOL SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags = PropertyOperation_None, SideEffects possibleSideEffects = SideEffects_Any) override;
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
        virtual BOOL ToPrimitive(JavascriptHint hint, Var* result, ScriptContext * requestContext) override;
        virtual BOOL GetEnumerator(JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext * scriptContext, EnumeratorCache * enumeratorCache = nullptr) override;
        virtual BOOL SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags = PropertyOperation_None) override;
        _Check_return_ _Success_(return) virtual BOOL GetAccessors(PropertyId propertyId, _Outptr_result_maybenull_ Var* getter, _Outptr_result_maybenull_ Var* setter, ScriptContext* requestContext) override;
        virtual BOOL IsWritable(PropertyId propertyId) override;
        virtual BOOL IsConfigurable(PropertyId propertyId) override;
        virtual BOOL IsEnumerable(PropertyId propertyId) override;
        virtual BOOL SetEnumerable(PropertyId propertyId, BOOL value) override;
        virtual BOOL SetWritable(PropertyId propertyId, BOOL value) override;
        virtual BOOL SetConfigurable(PropertyId propertyId, BOOL value) override;
        virtual BOOL SetAttributes(PropertyId propertyId, PropertyAttributes attributes) override;
        virtual BOOL IsExtensible() override { return GetIsExtensible(); };
        virtual BOOL PreventExtensions() override;
        virtual BOOL Seal() override;
        virtual BOOL Freeze() override;
        virtual BOOL IsSealed() override;
        virtual BOOL IsFrozen() override;
        virtual BOOL GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;
        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;
        virtual Var GetTypeOfString(ScriptContext * requestContext) override;

#if DBG
        virtual bool CanStorePropertyValueDirectly(PropertyId propertyId, bool allowLetConst) override;
#endif

        virtual void RemoveFromPrototype(ScriptContext * requestContext, bool * allProtoCachesInvalidated) override;
        virtual void AddToPrototype(ScriptContext * requestContext, bool * allProtoCachesInvalidated) override;
        virtual bool ClearProtoCachesWereInvalidated() override;
        virtual void SetPrototype(RecyclableObject* newPrototype) override;

        virtual BOOL IsCrossSiteObject() const { return FALSE; }

        virtual DynamicType* DuplicateType();
        virtual void PrepareForConversionToNonPathType();
        static bool IsTypeHandlerCompatibleForObjectHeaderInlining(DynamicTypeHandler * oldTypeHandler, DynamicTypeHandler * newTypeHandler);

        void ChangeType();

        void ChangeTypeIf(const Type* oldType);

        BOOL FindNextProperty(BigPropertyIndex& index, JavascriptString** propertyString, PropertyId* propertyId, PropertyAttributes* attributes,
            DynamicType *typeToEnumerate, EnumeratorFlags flags, ScriptContext * requestContext, PropertyValueInfo * info);

        virtual BOOL HasDeferredTypeHandler() const sealed;
        static DWORD GetOffsetOfAuxSlots();
        static DWORD GetOffsetOfObjectArray();
        static DWORD GetOffsetOfType();

        Js::BigPropertyIndex GetPropertyIndexFromInlineSlotIndex(uint inlineSlotIndex);
        Js::BigPropertyIndex GetPropertyIndexFromAuxSlotIndex(uint auxIndex);
        BOOL GetAttributesWithPropertyIndex(PropertyId propertyId, BigPropertyIndex index, PropertyAttributes * attributes);

        RecyclerWeakReference<DynamicObject>* CreateWeakReferenceToSelf();

        void SetObjectArray(ArrayObject* objectArray);
    protected:
        BOOL GetEnumeratorWithPrefix(JavascriptEnumerator * prefixEnumerator, JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext * scriptContext, EnumeratorCache * enumeratorCache);

        // These are only call for arrays
        void InitArrayFlags(DynamicObjectFlags flags);
        DynamicObjectFlags GetArrayFlags() const;
        DynamicObjectFlags GetArrayFlags_Unchecked() const; // do not use except in extreme circumstances
        void SetArrayFlags(const DynamicObjectFlags flags);

        ProfileId GetArrayCallSiteIndex() const;
        void SetArrayCallSiteIndex(ProfileId profileId);

        static DynamicObject * BoxStackInstance(DynamicObject * instance, bool deepCopy);

    private:
        ArrayObject* EnsureObjectArray();
        ArrayObject* GetObjectArrayOrFlagsAsArray() const { return objectArray; }

        template <PropertyId propertyId>
        BOOL ToPrimitiveImpl(Var* result, ScriptContext * requestContext);
        BOOL CallToPrimitiveFunction(Var toPrimitiveFunction, PropertyId propertyId, Var* result, ScriptContext * requestContext);
#if DBG
    public:
        virtual bool DbgIsDynamicObject() const override { return true; }
#endif

#ifdef RECYCLER_STRESS
    public:
        virtual void Finalize(bool isShutdown) override;
        virtual void Dispose(bool isShutdown) override;
        virtual void Mark(Recycler *recycler) override;
#endif

#if ENABLE_TTD
    public:
        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;

        Field(Js::Var) const* GetInlineSlots_TTD() const;
        Js::Var const* GetAuxSlots_TTD() const;

#if ENABLE_OBJECT_SOURCE_TRACKING
        void SetDiagOriginInfoAsNeeded();
#endif

#endif

    public:
        virtual VTableValue DummyVirtualFunctionToHinderLinkerICF()
        {
            // This virtual function hinders linker to do ICF vtable of this class with other classes.
            // ICF vtable causes unexpected behavior in type check code. Objects uses vtable as identify should
            // override this function and return a unique value.
            return VTableValue::VtableDynamicObject;
        }

    };

    template <> bool VarIsImpl<DynamicObject>(RecyclableObject* obj);
} // namespace Js

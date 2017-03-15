//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#define InitialObjTypeSpecFldInfoFlagValue 0x01

// Union with uint16 flags for fast default initialization
union ObjTypeSpecFldInfoFlags
{
    struct
    {
        Field(bool) falseReferencePreventionBit : 1;
        Field(bool) isPolymorphic : 1;
        Field(bool) isRootObjectNonConfigurableField : 1;
        Field(bool) isRootObjectNonConfigurableFieldLoad : 1;
        Field(bool) usesAuxSlot : 1;
        Field(bool) isLocal : 1;
        Field(bool) isLoadedFromProto : 1;
        Field(bool) usesAccessor : 1;
        Field(bool) hasFixedValue : 1;
        Field(bool) keepFieldValue : 1;
        Field(bool) isBeingStored : 1;
        Field(bool) isBeingAdded : 1;
        Field(bool) doesntHaveEquivalence : 1;
        Field(bool) isBuiltIn : 1;
    };
    struct
    {
        Field(uint16) flags;
    };
    ObjTypeSpecFldInfoFlags(uint16 flags) : flags(flags) { }
};

class ObjTypeSpecFldInfo
{
public:
    ObjTypeSpecFldInfo()
    {
        m_data.id = 0;
        m_data.typeId = Js::TypeIds_Limit;
        m_data.typeSet = nullptr;
        m_data.initialType = nullptr;
        m_data.flags = InitialObjTypeSpecFldInfoFlagValue;
        m_data.slotIndex = Js::Constants::NoSlot;
        m_data.propertyId = Js::Constants::NoProperty;
        m_data.protoObjectAddr = nullptr;
        m_data.propertyGuardValueAddr = nullptr;
        m_data.ctorCache = nullptr;
        m_data.fixedFieldCount = 0;
        m_data.fixedFieldInfoArraySize = 0;
        m_data.fixedFieldInfoArray = nullptr;
    }

    ObjTypeSpecFldInfo(uint id, Js::TypeId typeId, JITType* initialType,
        bool usesAuxSlot, bool isLoadedFromProto, bool usesAccessor, bool isFieldValueFixed, bool keepFieldValue, bool isBuiltIn,
        uint16 slotIndex, Js::PropertyId propertyId, Js::DynamicObject* protoObject, Js::PropertyGuard* propertyGuard,
        JITTimeConstructorCache* ctorCache, FixedFieldInfo* fixedFieldInfoArray)
    {
        ObjTypeSpecFldInfoFlags flags(InitialObjTypeSpecFldInfoFlagValue);
        flags.isPolymorphic = false;
        flags.usesAuxSlot = usesAuxSlot;
        flags.isLocal = !isLoadedFromProto && !usesAccessor;
        flags.isLoadedFromProto = isLoadedFromProto;
        flags.usesAccessor = usesAccessor;
        flags.hasFixedValue = isFieldValueFixed;
        flags.keepFieldValue = keepFieldValue;
        flags.isBeingAdded = initialType != nullptr;
        flags.doesntHaveEquivalence = true; // doesn't mean anything for data from a monomorphic cache
        flags.isBuiltIn = isBuiltIn;
        m_data.flags = flags.flags;

        m_data.id = id;
        m_data.typeId = typeId;
        m_data.typeSet = nullptr;
        m_data.initialType = initialType->GetData();
        m_data.slotIndex = slotIndex;
        m_data.propertyId = propertyId;
        m_data.protoObjectAddr = protoObject;
        m_data.propertyGuardValueAddr = (void*)propertyGuard->GetAddressOfValue();
        m_data.ctorCache = ctorCache->GetData();
        m_data.fixedFieldCount = 1;
        m_data.fixedFieldInfoArraySize = 1;
        m_data.fixedFieldInfoArray = fixedFieldInfoArray->GetRaw();
    }

    ObjTypeSpecFldInfo(uint id, Js::TypeId typeId, JITType* initialType, Js::EquivalentTypeSet* typeSet,
        bool usesAuxSlot, bool isLoadedFromProto, bool usesAccessor, bool isFieldValueFixed, bool keepFieldValue, bool doesntHaveEquivalence, bool isPolymorphic,
        uint16 slotIndex, Js::PropertyId propertyId, Js::DynamicObject* protoObject, Js::PropertyGuard* propertyGuard,
        JITTimeConstructorCache* ctorCache, FixedFieldInfo* fixedFieldInfoArray, uint16 fixedFieldCount)
    {
        ObjTypeSpecFldInfoFlags flags(InitialObjTypeSpecFldInfoFlagValue);
        flags.isPolymorphic = isPolymorphic;
        flags.usesAuxSlot = usesAuxSlot;
        flags.isLocal = !isLoadedFromProto && !usesAccessor;
        flags.isLoadedFromProto = isLoadedFromProto;
        flags.usesAccessor = usesAccessor;
        flags.hasFixedValue = isFieldValueFixed;
        flags.keepFieldValue = keepFieldValue;
        flags.isBeingAdded = initialType != nullptr;
        flags.doesntHaveEquivalence = doesntHaveEquivalence;
        flags.isBuiltIn = false;
        m_data.flags = flags.flags;

        m_data.id = id;
        m_data.typeId = typeId;
        m_data.typeSet = (EquivalentTypeSetIDL*)typeSet;
        m_data.initialType = initialType->GetData();
        m_data.slotIndex = slotIndex;
        m_data.propertyId = propertyId;
        m_data.protoObjectAddr = protoObject;
        m_data.propertyGuardValueAddr = (void*)propertyGuard->GetAddressOfValue();
        m_data.ctorCache = ctorCache->GetData();
        m_data.fixedFieldCount = fixedFieldCount;
        m_data.fixedFieldInfoArraySize = fixedFieldCount > 0 ? fixedFieldCount : 1;
        m_data.fixedFieldInfoArray = fixedFieldInfoArray->GetRaw();
    }

    bool UsesAuxSlot() const;
    bool UsesAccessor() const;
    bool IsRootObjectNonConfigurableFieldLoad() const;
    bool HasEquivalentTypeSet() const;
    bool DoesntHaveEquivalence() const;
    bool IsPoly() const;
    bool IsMono() const;
    bool IsBuiltIn() const;
    bool IsLoadedFromProto() const;
    bool HasFixedValue() const;
    bool IsBeingStored() const;
    bool IsBeingAdded() const;
    bool IsRootObjectNonConfigurableField() const;
    bool HasInitialType() const;
    bool IsMonoObjTypeSpecCandidate() const;
    bool IsPolyObjTypeSpecCandidate() const;

    bool GetKeepFieldValue() const
    {
        return GetFlags().keepFieldValue;
    }

    void SetFieldValue(Js::Var value)
    {
        Assert(IsMono() || (IsPoly() && !DoesntHaveEquivalence()));
        m_data.fixedFieldInfoArray[0].fieldValue = value;
    }

    bool IsBuiltin() const
    {
        return GetFlags().isBuiltIn;
    }

    void SetRootObjectNonConfigurableField(bool isLoad)
    {
        GetFlagsPtr()->isRootObjectNonConfigurableField = true;
        GetFlagsPtr()->isRootObjectNonConfigurableFieldLoad = isLoad;
    }

    Js::JavascriptFunction* GetFieldValueAsFunctionIfAvailable() const
    {
        Assert(!JITManager::GetJITManager()->IsJITServer());
        Assert(IsMono() || (IsPoly() && !DoesntHaveEquivalence()));

        if (PHASE_OFF1(Js::ObjTypeSpecPhase)) return nullptr; // TODO: (lei)remove this after obj type spec for OOPJIT implemented

        return m_data.fixedFieldInfoArray[0].fieldValue != 0 && Js::JavascriptFunction::Is((Js::Var)m_data.fixedFieldInfoArray[0].fieldValue) ?
            Js::JavascriptFunction::FromVar((Js::Var)m_data.fixedFieldInfoArray[0].fieldValue) : nullptr;
    }

    Js::TypeId GetTypeId() const;
    Js::TypeId GetTypeId(uint i) const;

    Js::PropertyId GetPropertyId() const;
    uint16 GetSlotIndex() const;
    uint16 GetFixedFieldCount() const;

    uint GetObjTypeSpecFldId() const;

    intptr_t GetProtoObject() const;
    intptr_t GetFieldValue(uint i) const;
    intptr_t GetPropertyGuardValueAddr() const;
    intptr_t GetFieldValueAsFixedDataIfAvailable() const;

    JITTimeConstructorCache * GetCtorCache() const;

    Js::EquivalentTypeSet * GetEquivalentTypeSet() const;
    JITTypeHolder GetType() const;
    JITTypeHolder GetType(uint i) const;
    JITTypeHolder GetInitialType() const;
    JITTypeHolder GetFirstEquivalentType() const;

    FixedFieldInfo * GetFixedFieldIfAvailableAsFixedFunction();
    FixedFieldInfo * GetFixedFieldIfAvailableAsFixedFunction(uint i);
    FixedFieldInfo * GetFixedFieldInfoArray() const;

    void SetIsBeingStored(bool value); // REVIEW: this doesn't flow out of JIT, should it?

    ObjTypeSpecFldIDL * GetRaw() { return &m_data; }

    static ObjTypeSpecFldInfo* CreateFrom(uint id, Js::InlineCache* cache, uint cacheId,
        Js::EntryPointInfo *entryPoint, Js::FunctionBody* const topFunctionBody, Js::FunctionBody *const functionBody, Js::FieldAccessStatsPtr inlineCacheStats);

    static ObjTypeSpecFldInfo* CreateFrom(uint id, Js::PolymorphicInlineCache* cache, uint cacheId,
        Js::EntryPointInfo *entryPoint, Js::FunctionBody* const topFunctionBody, Js::FunctionBody *const functionBody, Js::FieldAccessStatsPtr inlineCacheStats);

    // TODO: OOP JIT, implement this
    char16* GetCacheLayoutString() { __debugbreak(); return nullptr; }

private:
    ObjTypeSpecFldInfoFlags GetFlags() const;
    ObjTypeSpecFldInfoFlags * GetFlagsPtr();

    Field(ObjTypeSpecFldIDL) m_data;
};

class ObjTypeSpecFldInfoArray
{
private:
    Field(Field(ObjTypeSpecFldInfo*)*) infoArray;
#if DBG
    Field(uint) infoCount;
#endif
public:
    ObjTypeSpecFldInfoArray();

private:
    void EnsureArray(Recycler *const recycler, Js::FunctionBody *const functionBody);

public:
    ObjTypeSpecFldInfo* GetInfo(Js::FunctionBody *const functionBody, const uint index) const;
    ObjTypeSpecFldInfo* GetInfo(const uint index) const;
    Field(ObjTypeSpecFldInfo*)* GetInfoArray() const { return infoArray; }

    void SetInfo(Recycler *const recycler, Js::FunctionBody *const functionBody,
        const uint index, ObjTypeSpecFldInfo* info);

    void Reset();

    template <class Fn>
    void Map(Fn fn, uint count) const
    {
        if (this->infoArray != nullptr)
        {
            for (uint i = 0; i < count; i++)
            {
                ObjTypeSpecFldInfo* info = this->infoArray[i];

                if (info != nullptr)
                {
                    fn(info);
                }
            }
        }
    };

    PREVENT_COPY(ObjTypeSpecFldInfoArray)
};

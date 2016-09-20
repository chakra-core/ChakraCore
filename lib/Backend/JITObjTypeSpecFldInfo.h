//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class JITObjTypeSpecFldInfo
{
public:
    JITObjTypeSpecFldInfo(ObjTypeSpecFldIDL * data);
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

    JITTimeFixedField * GetFixedFieldIfAvailableAsFixedFunction();
    JITTimeFixedField * GetFixedFieldIfAvailableAsFixedFunction(uint i);
    JITTimeFixedField * GetFixedFieldInfoArray();

    void SetIsBeingStored(bool value); // REVIEW: this doesn't flow out of JIT, should it?

    static void BuildObjTypeSpecFldInfoArray(
        __in ArenaAllocator * alloc,
        __in Js::ObjTypeSpecFldInfo ** objTypeSpecInfo,
        __in uint arrayLength,
        _Inout_updates_(arrayLength) ObjTypeSpecFldIDL * jitData);

    // TODO: OOP JIT, implement this
    wchar_t* GetCacheLayoutString() { __debugbreak(); return nullptr; }

private:
    Js::ObjTypeSpecFldInfoFlags GetFlags() const;

    ObjTypeSpecFldIDL m_data;
};

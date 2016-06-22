//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

JITObjTypeSpecFldInfo::JITObjTypeSpecFldInfo(ObjTypeSpecFldIDL * data) :
    m_data(*data)
{
    CompileAssert(sizeof(ObjTypeSpecFldIDL) == sizeof(JITObjTypeSpecFldInfo));
}

bool
JITObjTypeSpecFldInfo::UsesAuxSlot() const
{
    return GetFlags().usesAuxSlot;
}

bool
JITObjTypeSpecFldInfo::UsesAccessor() const
{
    return GetFlags().usesAccessor;
}

bool
JITObjTypeSpecFldInfo::IsRootObjectNonConfigurableFieldLoad() const
{
    return GetFlags().isRootObjectNonConfigurableFieldLoad;
}

bool
JITObjTypeSpecFldInfo::HasEquivalentTypeSet() const
{
    return m_data.typeSet != nullptr;
}

bool
JITObjTypeSpecFldInfo::DoesntHaveEquivalence() const
{
    return GetFlags().doesntHaveEquivalence;
}

bool
JITObjTypeSpecFldInfo::IsPoly() const
{
    return GetFlags().isPolymorphic;
}

bool
JITObjTypeSpecFldInfo::IsMono() const
{
    return !IsPoly();
}

bool
JITObjTypeSpecFldInfo::IsBuiltIn() const
{
    return GetFlags().isBuiltIn;
}

bool
JITObjTypeSpecFldInfo::IsLoadedFromProto() const
{
    return GetFlags().isLoadedFromProto;
}

bool
JITObjTypeSpecFldInfo::HasFixedValue() const
{
    return GetFlags().hasFixedValue;
}

bool
JITObjTypeSpecFldInfo::IsBeingStored() const
{
    return GetFlags().isBeingStored;
}

bool
JITObjTypeSpecFldInfo::IsBeingAdded() const
{
    return GetFlags().isBeingAdded;
}

bool
JITObjTypeSpecFldInfo::IsRootObjectNonConfigurableField() const
{
    return GetFlags().isRootObjectNonConfigurableField;
}

bool
JITObjTypeSpecFldInfo::HasInitialType() const
{
    return IsMono() && !IsLoadedFromProto() && m_data.initialType != nullptr;
}

bool
JITObjTypeSpecFldInfo::IsMonoObjTypeSpecCandidate() const
{
    return IsMono();
}

bool
JITObjTypeSpecFldInfo::IsPolyObjTypeSpecCandidate() const
{
    return IsPoly();
}

Js::TypeId
JITObjTypeSpecFldInfo::GetTypeId() const
{
    Assert(m_data.typeId != Js::TypeIds_Limit);
    return (Js::TypeId)m_data.typeId;
}

Js::TypeId
JITObjTypeSpecFldInfo::GetTypeId(uint i) const
{
    Assert(IsPoly());
    return (Js::TypeId)m_data.fixedFieldInfoArray[i].type.typeId;
}

Js::PropertyId
JITObjTypeSpecFldInfo::GetPropertyId() const
{
    return (Js::PropertyId)m_data.propertyId;
}

uint16
JITObjTypeSpecFldInfo::GetSlotIndex() const
{
    return m_data.slotIndex;
}

uint16
JITObjTypeSpecFldInfo::GetFixedFieldCount() const
{
    return m_data.fixedFieldCount;
}

uint
JITObjTypeSpecFldInfo::GetObjTypeSpecFldId() const
{
    return m_data.id;
}

intptr_t
JITObjTypeSpecFldInfo::GetProtoObject() const
{
    return m_data.protoObjectAddr;
}

intptr_t
JITObjTypeSpecFldInfo::GetFieldValue(uint i) const
{
    Assert(IsPoly());
    return m_data.fixedFieldInfoArray[i].fieldValue;
}

intptr_t
JITObjTypeSpecFldInfo::GetPropertyGuardValueAddr() const
{
    return m_data.propertyGuardValueAddr;
}

intptr_t
JITObjTypeSpecFldInfo::GetFieldValueAsFixedDataIfAvailable() const
{
    Assert(HasFixedValue() && GetFixedFieldCount() == 1);

    return m_data.fixedFieldInfoArray[0].fieldValue;
}

JITTimeConstructorCache *
JITObjTypeSpecFldInfo::GetCtorCache() const
{
    return (JITTimeConstructorCache*)m_data.ctorCache;
}

Js::EquivalentTypeSet *
JITObjTypeSpecFldInfo::GetEquivalentTypeSet() const
{
    return (Js::EquivalentTypeSet *)m_data.typeSet;
}

JITType *
JITObjTypeSpecFldInfo::GetType() const
{
    Assert(IsMono());
    return (JITType *)&m_data.fixedFieldInfoArray[0].type;
}

JITType *
JITObjTypeSpecFldInfo::GetType(uint i) const
{
    Assert(IsPoly());
    return (JITType *)&m_data.fixedFieldInfoArray[i].type;
}

JITType *
JITObjTypeSpecFldInfo::GetInitialType() const
{
    return (JITType *)m_data.initialType;
}

JITType *
JITObjTypeSpecFldInfo::GetFirstEquivalentType() const
{
    Assert(HasEquivalentTypeSet());
    return GetEquivalentTypeSet()->GetFirstType();
}

void
JITObjTypeSpecFldInfo::SetIsBeingStored(bool value)
{
    ((Js::ObjTypeSpecFldInfoFlags*)&m_data.flags)->isBeingStored = true;
}

Js::ObjTypeSpecFldInfoFlags
JITObjTypeSpecFldInfo::GetFlags() const
{
    return (Js::ObjTypeSpecFldInfoFlags)m_data.flags;
}

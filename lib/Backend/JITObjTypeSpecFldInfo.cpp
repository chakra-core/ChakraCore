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
    return (Js::TypeId)m_data.fixedFieldInfoArray[i].type->typeId;
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

JITTypeHolder
JITObjTypeSpecFldInfo::GetType() const
{
    Assert(IsMono());
    return JITTypeHolder((JITType *)m_data.fixedFieldInfoArray[0].type);
}

JITTypeHolder
JITObjTypeSpecFldInfo::GetType(uint i) const
{
    Assert(IsPoly());
    return JITTypeHolder((JITType *)m_data.fixedFieldInfoArray[i].type);
}

JITTypeHolder
JITObjTypeSpecFldInfo::GetInitialType() const
{
    return JITTypeHolder((JITType *)m_data.initialType);
}

JITTypeHolder
JITObjTypeSpecFldInfo::GetFirstEquivalentType() const
{
    Assert(HasEquivalentTypeSet());
    return JITTypeHolder(GetEquivalentTypeSet()->GetFirstType());
}

void
JITObjTypeSpecFldInfo::SetIsBeingStored(bool value)
{
    ((Js::ObjTypeSpecFldInfoFlags*)&m_data.flags)->isBeingStored = value;
}

JITTimeFixedField *
JITObjTypeSpecFldInfo::GetFixedFieldIfAvailableAsFixedFunction()
{
    Assert(HasFixedValue());
    Assert(IsMono() || (IsPoly() && !DoesntHaveEquivalence()));
    Assert(m_data.fixedFieldInfoArray);
    if (m_data.fixedFieldInfoArray[0].funcInfoAddr != 0)
    {
        return (JITTimeFixedField *)&m_data.fixedFieldInfoArray[0];
    }
    return nullptr;
}

JITTimeFixedField *
JITObjTypeSpecFldInfo::GetFixedFieldIfAvailableAsFixedFunction(uint i)
{
    Assert(HasFixedValue());
    Assert(IsPoly());
    if (m_data.fixedFieldCount > 0 && m_data.fixedFieldInfoArray[i].funcInfoAddr != 0)
    {
        return (JITTimeFixedField *)&m_data.fixedFieldInfoArray[i];
    }
    return nullptr;
}

JITTimeFixedField *
JITObjTypeSpecFldInfo::GetFixedFieldInfoArray()
{
    return (JITTimeFixedField*)m_data.fixedFieldInfoArray;
}

/* static */
void
JITObjTypeSpecFldInfo::BuildObjTypeSpecFldInfoArray(
    __in ArenaAllocator * alloc,
    __in Js::ObjTypeSpecFldInfo ** objTypeSpecInfo,
    __in uint arrayLength,
    __out ObjTypeSpecFldIDL * jitData)
{
    for (uint i = 0; i < arrayLength; ++i)
    {
        if (objTypeSpecInfo[i] == nullptr)
        {
            continue;
        }
        if (objTypeSpecInfo[i]->IsLoadedFromProto())
        {
            jitData[i].protoObjectAddr = (intptr_t)objTypeSpecInfo[i]->GetProtoObject();
        }
        jitData[i].propertyGuardValueAddr = (intptr_t)objTypeSpecInfo[i]->GetPropertyGuard()->GetAddressOfValue();
        jitData[i].propertyId = objTypeSpecInfo[i]->GetPropertyId();
        jitData[i].typeId = objTypeSpecInfo[i]->GetTypeId();
        jitData[i].id = objTypeSpecInfo[i]->GetObjTypeSpecFldId();
        jitData[i].flags = objTypeSpecInfo[i]->GetFlags();
        jitData[i].slotIndex = objTypeSpecInfo[i]->GetSlotIndex();
        jitData[i].fixedFieldCount = objTypeSpecInfo[i]->GetFixedFieldCount();

        if (objTypeSpecInfo[i]->HasInitialType())
        {
            jitData[i].initialType = AnewStructZ(alloc, TypeIDL);
            JITType::BuildFromJsType(objTypeSpecInfo[i]->GetInitialType(), (JITType*)jitData[i].initialType);
        }

        if (objTypeSpecInfo[i]->GetCtorCache() != nullptr)
        {
            jitData[i].ctorCache = objTypeSpecInfo[i]->GetCtorCache()->GetData();
        }

        CompileAssert(sizeof(Js::EquivalentTypeSet) == sizeof(EquivalentTypeSetIDL));
        Js::EquivalentTypeSet * equivTypeSet = objTypeSpecInfo[i]->GetEquivalentTypeSet();
        if (equivTypeSet != nullptr)
        {
            jitData[i].typeSet = (EquivalentTypeSetIDL*)equivTypeSet;
        }

        jitData[i].fixedFieldInfoArraySize = jitData[i].fixedFieldCount;
        if (jitData[i].fixedFieldInfoArraySize == 0)
        {
            jitData[i].fixedFieldInfoArraySize = 1;
        }
        jitData[i].fixedFieldInfoArray = AnewArrayZ(alloc, FixedFieldIDL, jitData[i].fixedFieldInfoArraySize);
        Js::FixedFieldInfo * ffInfo = objTypeSpecInfo[i]->GetFixedFieldInfoArray();
        for (uint16 j = 0; j < jitData[i].fixedFieldInfoArraySize; ++j)
        {
            jitData[i].fixedFieldInfoArray[j].fieldValue = (intptr_t)ffInfo[j].fieldValue;
            jitData[i].fixedFieldInfoArray[j].nextHasSameFixedField = ffInfo[j].nextHasSameFixedField;
            if (ffInfo[j].fieldValue != nullptr && Js::JavascriptFunction::Is(ffInfo[j].fieldValue))
            {
                Js::JavascriptFunction * funcObj = Js::JavascriptFunction::FromVar(ffInfo[j].fieldValue);
                jitData[i].fixedFieldInfoArray[j].valueType = ValueType::FromObject(funcObj).GetRawData();
                jitData[i].fixedFieldInfoArray[j].funcInfoAddr = (intptr_t)funcObj->GetFunctionInfo();
                jitData[i].fixedFieldInfoArray[j].localFuncId = (intptr_t)funcObj->GetFunctionInfo()->GetLocalFunctionId();
                if (Js::ScriptFunction::Is(ffInfo[j].fieldValue))
                {
                    jitData[i].fixedFieldInfoArray[j].environmentAddr = (intptr_t)Js::ScriptFunction::FromVar(funcObj)->GetEnvironment();
                }
            }
            if (ffInfo[j].type != nullptr)
            {
                jitData[i].fixedFieldInfoArray[j].type = AnewStructZ(alloc, TypeIDL);
                // TODO: OOP JIT, maybe type should be out of line? might not save anything on x64 though
                JITType::BuildFromJsType(ffInfo[j].type, (JITType*)jitData[i].fixedFieldInfoArray[j].type);
            }
        }
    }
}

Js::ObjTypeSpecFldInfoFlags
JITObjTypeSpecFldInfo::GetFlags() const
{
    return (Js::ObjTypeSpecFldInfoFlags)m_data.flags;
}

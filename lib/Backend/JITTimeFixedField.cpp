//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

CompileAssert(sizeof(FixedFieldIDL) == sizeof(JITTimeFixedField));

void
JITTimeFixedField::SetNextHasSameFixedField()
{
    m_data.nextHasSameFixedField = TRUE;
}

bool
JITTimeFixedField::IsClassCtor() const
{
    return m_data.isClassCtor != FALSE;
}

bool
JITTimeFixedField::NextHasSameFixedField() const
{
    return m_data.nextHasSameFixedField != FALSE;
}

uint
JITTimeFixedField::GetLocalFuncId() const
{
    return m_data.localFuncId;
}

ValueType
JITTimeFixedField::GetValueType() const
{
    CompileAssert(sizeof(ValueType) == sizeof(uint16));
    return *(ValueType*)&m_data.valueType;
}

intptr_t
JITTimeFixedField::GetFieldValue() const
{
    return m_data.fieldValue;
}

intptr_t
JITTimeFixedField::GetFuncInfoAddr() const
{
    return m_data.funcInfoAddr;
}

intptr_t
JITTimeFixedField::GetEnvironmentAddr() const
{
    return m_data.environmentAddr;
}

JITType *
JITTimeFixedField::GetType() const
{
    return (JITType*)m_data.type;
}

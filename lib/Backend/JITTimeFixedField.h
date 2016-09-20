//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class JITTimeFixedField
{
public:
    void SetNextHasSameFixedField();

    bool IsClassCtor() const;
    bool NextHasSameFixedField() const;
    ValueType GetValueType() const;
    uint GetLocalFuncId() const;
    intptr_t GetFuncInfoAddr() const;
    intptr_t GetEnvironmentAddr() const;
    intptr_t GetFieldValue() const;
    JITType * GetType() const;
private:
    JITTimeFixedField();
    FixedFieldIDL m_data;
};

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class FixedFieldInfo
{
public:
    FixedFieldInfo() : m_data() { }
    static void PopulateFixedField(_In_opt_ Js::Type * type, _In_opt_ Js::Var var, _Out_ FixedFieldInfo * fixed);

    void SetNextHasSameFixedField();

    bool IsClassCtor() const;
    bool NextHasSameFixedField() const;
    ValueType GetValueType() const;
    Js::LocalFunctionId GetLocalFuncId() const;
    intptr_t GetFuncInfoAddr() const;
    intptr_t GetEnvironmentAddr() const;
    intptr_t GetFieldValue() const;
    JITType * GetType() const;
    FixedFieldIDL * GetRaw();
private:
    Field(FixedFieldIDL) m_data;
};

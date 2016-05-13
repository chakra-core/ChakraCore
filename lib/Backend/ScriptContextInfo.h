//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class ScriptContextInfo
{
public:
    ScriptContextInfo(ScriptContextData * contextData);
    intptr_t GetNullAddr() const;
    intptr_t GetUndefinedAddr() const;
    intptr_t GetTrueAddr() const;
    intptr_t GetFalseAddr() const;
    intptr_t GetUndeclBlockVarAddr() const;
    intptr_t GetEmptyStringAddr() const;
    intptr_t GetNegativeZeroAddr() const;
    intptr_t GetNumberTypeStaticAddr() const;
    intptr_t GetStringTypeStaticAddr() const;
    intptr_t GetObjectTypeAddr() const;
    intptr_t GetObjectHeaderInlinedTypeAddr() const;
    intptr_t GetRegexTypeAddr() const;
    intptr_t GetArrayConstructorAddr() const;
    intptr_t GetCharStringCacheAddr() const;
    intptr_t GetAddr() const;

    intptr_t GetVTableAddress(VTableValue vtableType) const;


private:
    ScriptContextData m_contextData;
};

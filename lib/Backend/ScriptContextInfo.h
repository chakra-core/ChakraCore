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
    intptr_t GetAddr() const;


private:
    ScriptContextData * m_contextData;
};

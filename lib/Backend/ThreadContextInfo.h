//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

// TODO michhol (OOP JIT): rename this
class ThreadContextInfo
{
public:
    ThreadContextInfo(ThreadContextData * data);

    intptr_t GetNullFrameDisplayAddr() const;
    intptr_t GetStrictNullFrameDisplayAddr() const;

private:
    ThreadContextData * m_threadContextData;
};

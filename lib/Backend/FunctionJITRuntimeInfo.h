//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class FunctionJITRuntimeInfo
{
public:
    FunctionJITRuntimeInfo(FunctionJITRuntimeData * data);

    intptr_t GetClonedInlineCache(uint index) const;
    bool HasClonedInlineCaches() const;

private:
    FunctionJITRuntimeData m_data;
};

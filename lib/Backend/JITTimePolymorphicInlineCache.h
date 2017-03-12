//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class JITTimePolymorphicInlineCache
{
public:
    JITTimePolymorphicInlineCache();

    intptr_t GetAddr() const;
    intptr_t GetInlineCachesAddr() const;
    uint16 GetSize() const;

private:
    PolymorphicInlineCacheIDL m_data;
};

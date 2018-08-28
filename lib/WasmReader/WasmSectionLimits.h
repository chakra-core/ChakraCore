//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
#ifdef ENABLE_WASM
namespace Wasm
{
struct SectionLimits
{
    enum Flags : uint32
    {
        HAS_MAXIMUM = 1 << 0,
    };
    bool HasMaximum() const { return (flags & HAS_MAXIMUM) != 0; }

    Flags flags;
    uint32 initial;
    uint32 maximum;
};

struct MemorySectionLimits : public SectionLimits
{
    enum Flags : uint32
    {
        IS_SHARED = 1 << 1,
    };
    bool IsShared() const { return (flags & IS_SHARED) != 0; }
};

struct TableSectionLimits : public SectionLimits
{
    // Nothing specific to table section yet
};
}
#endif

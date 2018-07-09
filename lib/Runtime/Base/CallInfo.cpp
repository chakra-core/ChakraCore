//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeBasePch.h"

namespace Js
{
    const ushort CallInfo::ksizeofCount =  24;
    const ushort CallInfo::ksizeofCallFlags = 8;
    const uint CallInfo::kMaxCountArgs = (1 << ksizeofCount) - 1 ;

    // For Eval calls the FrameDisplay is passed in as an extra argument.
    // This is not counted in Info.Count. Use this API to get the updated count.
    ArgSlot CallInfo::GetArgCountWithExtraArgs(CallFlags flags, uint count)
    {
        AssertOrFailFastMsg(count < Constants::UShortMaxValue - 1, "ArgList too large");
        ArgSlot argSlotCount = (ArgSlot)count;
        if (CallInfo::HasExtraArg(flags))
        {
            argSlotCount++;
        }
        return argSlotCount;
    }

    uint CallInfo::GetLargeArgCountWithExtraArgs(CallFlags flags, uint count)
    {
        if (CallInfo::HasExtraArg(flags))
        {
            UInt32Math::Inc(count);
        }
        return count;
    }

    ArgSlot CallInfo::GetArgCountWithoutExtraArgs(CallFlags flags, ArgSlot count)
    {
        ArgSlot newCount = count;
        if (CallInfo::HasExtraArg(flags))
        {
            if (count == 0)
            {
                ::Math::DefaultOverflowPolicy();
            }
            newCount = count - 1;
        }
        return newCount;
    }
}

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

#if DBG && defined(RECYCLER_VERIFY_MARK)

#include "ByteCode/ScopeInfo.h"

bool IsLikelyRuntimeFalseReference(char* objectStartAddress, size_t offset,
    const char* typeName)
{
    // Js::DynamicProfileInfo allocate with non-Leaf in test/chk build
    // TODO: (leish)(swb) find a way to set barrier for the Js::DynamicProfileInfo plus allocation
    if (strstr(typeName, "Js::DynamicProfileInfo"))
    {
        return true;
    }

    // the fields on Js::DateImplementation can easily form a false positive
    // TODO: (leish)(swb) find a way to tag these
    if (VirtualTableInfo<Js::JavascriptDate>::HasVirtualTable(objectStartAddress) ||
        VirtualTableInfo<Js::CrossSiteObject<Js::JavascriptDate>>::HasVirtualTable(objectStartAddress))
    {
        return offset >= offsetof(Js::JavascriptDate, m_date);
    }

    // symbol array at the end of scopeInfo, can point to arena allocated propertyRecord
    if (offset >= offsetof(Js::ScopeInfo, symbols) && strstr(typeName, "Js::ScopeInfo"))
    {
        return true;
    }

    // Js::Type::entryPoint may contain outdated data uncleared, and reused by recycler
    // Most often occurs with script function Type
    if (offset == Js::Type::GetOffsetOfEntryPoint() && strstr(typeName, "Js::ScriptFunctionType"))
    {
        return true;
    }

    if (strstr(typeName, "Js::SparseArraySegment"))
    {
        // Js::SparseArraySegmentBase left, length and size can easily form a false positive
        // TODO: (leish)(swb) find a way to tag these fields
        if (offset < Js::SparseArraySegmentBase::GetOffsetOfNext())  // left, length, size
        {
            return true;
        }

        // Native array elements may form false positives
        if (offset > Js::SparseArraySegmentBase::GetOffsetOfNext() &&  // elements
            (strstr(typeName, "<double>") || strstr(typeName, "<int>")))
        {
            return true;
        }
    }

    return false;
}

#endif  // DBG && defined(RECYCLER_VERIFY_MARK)

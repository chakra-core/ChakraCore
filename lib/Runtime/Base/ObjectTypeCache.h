//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Js
{

class ObjectTypeCache
{
public:
    static const uint MaxCachedTypes = 32;

private:
    struct CacheEntry
    {
        Field(RecyclerWeakReference<RecyclableObject>*) prototype;
        Field(RecyclerWeakReference<DynamicType>*) type;
    };

    CacheEntry cache[MaxCachedTypes];

public:
    DynamicType* FindOrCreateType(_In_ RecyclableObject* prototype);
};

} // namespace Js

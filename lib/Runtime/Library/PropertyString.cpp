//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    DEFINE_RECYCLER_TRACKER_PERF_COUNTER(PropertyString);
    DEFINE_RECYCLER_TRACKER_WEAKREF_PERF_COUNTER(PropertyString);

    PropertyString::PropertyString(StaticType* type, const Js::PropertyRecord* propertyRecord) :
        JavascriptString(type, propertyRecord->GetLength(), propertyRecord->GetBuffer()),
        propertyRecord(propertyRecord),
        ldElemInlineCache(nullptr),
        stElemInlineCache(nullptr),
        hitRate(0)
    {
    }

    PropertyString* PropertyString::New(StaticType* type, const Js::PropertyRecord* propertyRecord, Recycler *recycler)
    {
        PropertyString * propertyString = RecyclerNewZ(recycler, PropertyString, type, propertyRecord);
        // TODO: in future, might be worth putting these inline to avoid extra allocations. PIC copy API needs to be updated to support this though
        propertyString->ldElemInlineCache = ScriptContextPolymorphicInlineCache::New(MinPropertyStringInlineCacheSize, type->GetLibrary());
        propertyString->stElemInlineCache = ScriptContextPolymorphicInlineCache::New(MinPropertyStringInlineCacheSize, type->GetLibrary());
        return propertyString;
    }

    PolymorphicInlineCache * PropertyString::GetLdElemInlineCache() const
    {
        return this->ldElemInlineCache;
    }

    PolymorphicInlineCache * PropertyString::GetStElemInlineCache() const
    {
        return this->stElemInlineCache;
    }

    void const * PropertyString::GetOriginalStringReference()
    {
        // Property record is the allocation containing the string buffer
        return this->propertyRecord;
    }

    bool PropertyString::ShouldUseCache() const
    {
        return this->hitRate > (int)CONFIG_FLAG(StringCacheMissThreshold);
    }

    void PropertyString::LogCacheMiss()
    {
        this->hitRate -= (int)CONFIG_FLAG(StringCacheMissPenalty);
        if (this->hitRate < (int)CONFIG_FLAG(StringCacheMissReset))
        {
            this->hitRate = 0;
        }
    }

    RecyclableObject * PropertyString::CloneToScriptContext(ScriptContext* requestContext)
    {
        return requestContext->GetLibrary()->CreatePropertyString(this->propertyRecord);
    }

    PolymorphicInlineCache * PropertyString::CreateBiggerPolymorphicInlineCache(bool isLdElem)
    {
        PolymorphicInlineCache * polymorphicInlineCache = isLdElem ? GetLdElemInlineCache() : GetStElemInlineCache();
        Assert(polymorphicInlineCache && polymorphicInlineCache->CanAllocateBigger());
        uint16 polymorphicInlineCacheSize = polymorphicInlineCache->GetSize();
        uint16 newPolymorphicInlineCacheSize = PolymorphicInlineCache::GetNextSize(polymorphicInlineCacheSize);
        Assert(newPolymorphicInlineCacheSize > polymorphicInlineCacheSize);
        PolymorphicInlineCache * newPolymorphicInlineCache = ScriptContextPolymorphicInlineCache::New(newPolymorphicInlineCacheSize, GetLibrary());
        polymorphicInlineCache->CopyTo(this->propertyRecord->GetPropertyId(), GetScriptContext(), newPolymorphicInlineCache);
        if (isLdElem)
        {
            this->ldElemInlineCache = newPolymorphicInlineCache;
        }
        else
        {
            this->stElemInlineCache = newPolymorphicInlineCache;
        }
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        if (PHASE_VERBOSE_TRACE1(Js::PolymorphicInlineCachePhase) || PHASE_TRACE1(PropertyStringCachePhase))
        {
            Output::Print(_u("PropertyString '%s' : Bigger PIC, oldSize = %d, newSize = %d\n"), GetString(), polymorphicInlineCacheSize, newPolymorphicInlineCacheSize);
        }
#endif
        return newPolymorphicInlineCache;
    }
}

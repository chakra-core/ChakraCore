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
        propertyRecordUsageCache(type, propertyRecord)
    {
    }

    PropertyString* PropertyString::New(StaticType* type, const Js::PropertyRecord* propertyRecord, Recycler *recycler)
    {
        return RecyclerNewZ(recycler, PropertyString, type, propertyRecord);
    }

    PolymorphicInlineCache * PropertyString::GetLdElemInlineCache() const
    {
        return this->propertyRecordUsageCache.GetLdElemInlineCache();
    }

    PolymorphicInlineCache * PropertyString::GetStElemInlineCache() const
    {
        return this->propertyRecordUsageCache.GetStElemInlineCache();
    }

    PropertyRecordUsageCache * PropertyString::GetPropertyRecordUsageCache()
    {
        return &this->propertyRecordUsageCache;
    }

    template <> bool VarIsImpl<PropertyString>(RecyclableObject * obj)
    {
        return VirtualTableInfo<Js::PropertyString>::HasVirtualTable(obj);
    }

    const void * PropertyString::GetOriginalStringReference()
    {
        // Property record is the allocation containing the string buffer
        return this->propertyRecordUsageCache.GetPropertyRecord();
    }

    bool PropertyString::TrySetPropertyFromCache(
        _In_ RecyclableObject *const object,
        _In_ Var propertyValue,
        _In_ ScriptContext *const requestContext,
        const PropertyOperationFlags propertyOperationFlags,
        _Inout_ PropertyValueInfo *const propertyValueInfo)
    {
        return this->propertyRecordUsageCache.TrySetPropertyFromCache<false /* ReturnOperationInfo */>(object, propertyValue, requestContext, propertyOperationFlags, propertyValueInfo, this, nullptr);
    }

    RecyclableObject * PropertyString::CloneToScriptContext(ScriptContext* requestContext)
    {
        return requestContext->GetPropertyString(this->propertyRecordUsageCache.GetPropertyRecord());
    }
}

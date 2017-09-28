//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeBasePch.h"
#include "Types/PathTypeHandler.h"

namespace Js
{

DynamicType* ObjectTypeCache::FindOrCreateType(_In_ RecyclableObject* prototype)
{
    // For fast lookup, cache index is a function of prototype address
    const uint index = (uint)(((uintptr_t)prototype) >> PolymorphicInlineCacheShift) & (ObjectTypeCache::MaxCachedTypes - 1);

    RecyclerWeakReference<RecyclableObject>* cachedWeakProto = cache[index].prototype;

    // Check for nullptr, because the cache will not have a weak ref yet if this is first access
    if (cachedWeakProto != nullptr)
    {
        Assert(cache[index].type != nullptr);

        DynamicType* cachedType = cache[index].type->Get();
        RecyclableObject* cachedProto = cachedWeakProto->Get();

        // We can use cache if the prototypes match and the Type hasn't been collected
        if (cachedProto == prototype && cachedType != nullptr)
        {
            return cachedType;
        }
    }

    // If we didn't already have a Type in our cache, we need to create a new one

    JavascriptLibrary * library = prototype->GetLibrary();
    ScriptContext * scriptContext = library->GetScriptContext();

    SimplePathTypeHandler* typeHandler = SimplePathTypeHandler::New(scriptContext, library->GetRootPath(), 0, 0, 0, true, true);
    DynamicType* newType = DynamicType::New(scriptContext, TypeIds_Object, prototype, nullptr, typeHandler, true, true);

    // Store the Type and prototype as weak references to avoid unnecessarily extending their lifetimes
    RecyclerWeakReference<RecyclableObject> *  newWeakProtoHandle = prototype->GetRecycler()->CreateWeakReferenceHandle(prototype);
    RecyclerWeakReference<DynamicType> * newWeakTypeHandle = prototype->GetRecycler()->CreateWeakReferenceHandle(newType);

    cache[index].prototype = newWeakProtoHandle;
    cache[index].type = newWeakTypeHandle;
    return newType;
}

} // namespace Js

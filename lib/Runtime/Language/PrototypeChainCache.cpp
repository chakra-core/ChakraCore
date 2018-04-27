//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

using namespace Js;

template <typename T>
ThreadCacheRegistry<T>::ThreadCacheRegistry(_In_ PageAllocator* pageAllocator) :
    arena(_u("PrototypeChainCacheRegistry"), pageAllocator, Throw::OutOfMemory)
{
}

template <typename T>
ScriptCacheRegistry<T>**
ThreadCacheRegistry<T>::Register(_In_ ScriptCacheRegistry<T>* cache)
{
    return this->scriptRegistrations.PrependNode(&this->arena, cache);
}

template <typename T>
void
ThreadCacheRegistry<T>::Unregister(_In_ ScriptCacheRegistry<T>** registration)
{
    this->scriptRegistrations.RemoveElement(&this->arena, registration);
}

template <typename T>
void
ThreadCacheRegistry<T>::Reset()
{
    this->scriptRegistrations.Reset();
}

template <typename T>
void
ThreadCacheRegistry<T>::Clear()
{
    bool hasItem = false;
    FOREACH_DLISTBASE_ENTRY(ScriptCacheRegistry<T>*, registration, &this->scriptRegistrations)
    {
        registration->Clear(true /* isThreadClear */);
        hasItem = true;
    }
    NEXT_DLISTBASE_ENTRY;

    if (!hasItem)
    {
        return;
    }

    this->scriptRegistrations.Reset();
    this->arena.Reset();
}

template <typename T>
ScriptCacheRegistry<T>::ScriptCacheRegistry(_In_ ScriptContext* scriptContext, _In_ ThreadCacheRegistry<T>* threadRegistry) :
    threadRegistry(threadRegistry),
    scriptContext(scriptContext),
    cache(nullptr),
    registration(nullptr)
{
}

template <typename T>
void
AssignCache(_In_ T* cache)
{
    Assert(this->cache == nullptr);
    this->cache = cache;
}

template <typename T>
void
ScriptCacheRegistry<T>::Register()
{
    Assert(this->cache != nullptr);
    if (this->registration == nullptr)
    {
        this->registration = this->threadRegistry->Register(this);
    }
}

template <typename T>
void
ScriptCacheRegistry<T>::Clear(bool isThreadClear)
{
    if (this->registration != nullptr)
    {
        Assert(this->cache);
        if (!this->scriptContext->IsFinalized())
        {
            // If ScriptContext is finalized, then cache may be freed so we shouldn't clear it
            this->cache->Clear();
        }
        // Thread clear will reset the registry, so we don't need to call unregister
        if (!isThreadClear)
        {
            this->threadRegistry->Unregister(this->registration);
        }
        this->registration = nullptr;
    }
}

template <typename T>
void
ScriptCacheRegistry<T>::AssignCache(_In_ T* cache)
{
    Assert(this->cache == nullptr);
    this->cache = cache;
}

template <typename T>
PrototypeChainCache<T>::PrototypeChainCache(_In_ Recycler * recycler) :
    scriptRegistry(nullptr),
    types(recycler)
{
}

template <typename T>
void
PrototypeChainCache<T>::Initialize(_In_ ScriptCacheRegistry<PrototypeChainCache<T>>* scriptRegistry)
{
    this->scriptRegistry = scriptRegistry;
    scriptRegistry->AssignCache(this);
}

template <typename T>
void
PrototypeChainCache<T>::Register(_In_ Type* type)
{
    Assert(type);
    Assert(type->GetScriptContext() == scriptRegistry->GetScriptContext());
    Assert(!scriptRegistry->GetScriptContext()->IsClosed());
#if DBG
    T::RegistrationAssert(type);
#endif

    if (this->types.Count() == 0)
    {
        this->scriptRegistry->Register();
    }
    this->types.Add(type);
}

template <typename T>
bool
PrototypeChainCache<T>::Check(_In_ RecyclableObject* object)
{
    Assert(object);
    if (object->GetType()->HasSpecialPrototype())
    {
        TypeId typeId = object->GetTypeId();
        if (typeId == TypeIds_Null)
        {
            return true;
        }
        if (typeId == TypeIds_Proxy)
        {
            return false;
        }
    }

    if (!T::CheckObject(object))
    {
        return false;
    }

    return CheckProtoChain(object->GetPrototype());
}

template <typename T>
bool
PrototypeChainCache<T>::CheckProtoChain(_In_ RecyclableObject* prototype)
{
    Assert(prototype);

    if (T::IsCached(prototype))
    {
        Assert(CheckProtoChainInternal(prototype));
        return true;
    }
    return CheckProtoChainInternal(prototype);
}

template <typename T>
bool
PrototypeChainCache<T>::CheckProtoChainInternal(RecyclableObject* prototype)
{
    Assert(prototype);

    Type *const originalType = prototype->GetType();
    ScriptContext *const scriptContext = prototype->GetScriptContext();

    bool onlyOneScriptContext = true;
    TypeId typeId;
    for (; (typeId = prototype->GetTypeId()) != TypeIds_Null; prototype = prototype->GetPrototype())
    {
        if (typeId == TypeIds_Proxy)
        {
            return false;
        }
        if (prototype->GetScriptContext() != scriptContext)
        {
            onlyOneScriptContext = false;
        }
        if (!T::CheckObject(prototype))
        {
            return false;
        }
    }

    if (onlyOneScriptContext)
    {
        // Technically, we could register all prototypes in the chain but this is good enough for now
        T::Cache(originalType);
    }

    return true;
}

template <typename T>
void
PrototypeChainCache<T>::Clear()
{
    for (int i = 0; i < this->types.Count(); ++i)
    {
        T::ClearType(this->types.Item(i));
    }
    this->types.ClearAndZero();
}

void
NoSpecialPropertyCache::ClearType(_In_ Type* type)
{
    type->SetThisAndPrototypesHaveNoSpecialProperties(false);
}

bool
NoSpecialPropertyCache::IsSpecialProperty(PropertyId propertyId)
{
    return propertyId == PropertyIds::_symbolToStringTag
        || propertyId == PropertyIds::_symbolToPrimitive
        || propertyId == PropertyIds::toString
        || propertyId == PropertyIds::valueOf;
}

bool
NoSpecialPropertyCache::IsSpecialProperty(_In_ PropertyRecord const* propertyRecord)
{
    return IsSpecialProperty(propertyRecord->GetPropertyId());
}

bool
NoSpecialPropertyCache::IsSpecialProperty(_In_ JavascriptString* propertyString)
{
    return BuiltInPropertyRecords::valueOf.Equals(propertyString)
        || BuiltInPropertyRecords::toString.Equals(propertyString);
}

// We can handle default initialization of toString/valueOf for Object.prototype,
// so check if it's that's what is happening
bool
NoSpecialPropertyCache::IsDefaultSpecialProperty(
    _In_ DynamicObject* instance,
    _In_ JavascriptLibrary* library,
    PropertyId propertyId)
{
    return instance == library->GetObjectPrototype()
        && ((propertyId == PropertyIds::toString && !library->GetObjectToStringFunction())
            || (propertyId == PropertyIds::valueOf && !library->GetObjectValueOfFunction()));
}

bool
NoSpecialPropertyCache::IsDefaultHandledSpecialProperty(PropertyId propertyId)
{
    return propertyId == PropertyIds::toString || propertyId == PropertyIds::valueOf;
}

bool
NoSpecialPropertyCache::IsDefaultHandledSpecialProperty(_In_ PropertyRecord const* propertyRecord)
{
    return IsDefaultHandledSpecialProperty(propertyRecord->GetPropertyId());
}

bool
NoSpecialPropertyCache::IsDefaultHandledSpecialProperty(_In_ JavascriptString* propertyString)
{
    return IsSpecialProperty(propertyString);
}

bool
NoSpecialPropertyCache::IsCached(_In_ RecyclableObject* prototype)
{
    return prototype->GetType()->ThisAndPrototypesHaveNoSpecialProperties();
}

void
NoSpecialPropertyCache::Cache(_In_ Type* type)
{
    type->SetThisAndPrototypesHaveNoSpecialProperties(true);
}

bool
NoSpecialPropertyCache::CheckObject(_In_ RecyclableObject* object)
{
    return !object->HasAnySpecialProperties() && !object->HasDeferredTypeHandler();
}

#if DBG
void
NoSpecialPropertyCache::RegistrationAssert(_In_ Type* type)
{
    Assert(type->ThisAndPrototypesHaveNoSpecialProperties());
}
#endif

void
OnlyWritablePropertyCache::ClearType(_In_ Type* type)
{
    type->SetAreThisAndPrototypesEnsuredToHaveOnlyWritableDataProperties(false);
}

bool
OnlyWritablePropertyCache::IsCached(_In_ RecyclableObject* prototype)
{
    return prototype->GetType()->AreThisAndPrototypesEnsuredToHaveOnlyWritableDataProperties();
}

void
OnlyWritablePropertyCache::Cache(_In_ Type* type)
{
    type->SetAreThisAndPrototypesEnsuredToHaveOnlyWritableDataProperties(true);
}

bool
OnlyWritablePropertyCache::CheckObject(_In_ RecyclableObject* object)
{
    return object->HasOnlyWritableDataProperties();
}

#if DBG
void
OnlyWritablePropertyCache::RegistrationAssert(_In_ Type* type)
{
    Assert(type->AreThisAndPrototypesEnsuredToHaveOnlyWritableDataProperties());
}
#endif

template class Js::PrototypeChainCache<NoSpecialPropertyCache>;
template class Js::PrototypeChainCache<OnlyWritablePropertyCache>;

template class Js::ScriptCacheRegistry<NoSpecialPropertyProtoChainCache>;
template class Js::ScriptCacheRegistry<OnlyWritablePropertyProtoChainCache>;

template class Js::ThreadCacheRegistry<NoSpecialPropertyProtoChainCache>;
template class Js::ThreadCacheRegistry<OnlyWritablePropertyProtoChainCache>;

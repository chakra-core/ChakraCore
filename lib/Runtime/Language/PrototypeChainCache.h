//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Js
{
template <typename T> class PrototypeChainCache;
template <typename T> class ScriptCacheRegistry;

template <typename T>
class ThreadCacheRegistry
{
public:
    ThreadCacheRegistry(_In_ PageAllocator* pageAllocator);
    ScriptCacheRegistry<T>** Register(_In_ ScriptCacheRegistry<T>* cache);
    void Unregister(_In_ ScriptCacheRegistry<T>** cache);
    void Clear();
    void Reset();

private:
    ArenaAllocator arena;
    DListBase<ScriptCacheRegistry<T>*> scriptRegistrations;
};

template <typename T>
class ScriptCacheRegistry
{
public:
    ScriptCacheRegistry(_In_ ScriptContext* scriptContext, _In_ ThreadCacheRegistry<T>* threadRegistry);
    void Register();
    void Clear(bool isThreadClear);
    void AssignCache(_In_ T* cache);

    ScriptContext * GetScriptContext() const { return scriptContext; }
private:
    T* cache;
    ScriptCacheRegistry<T>** registration;
    ThreadCacheRegistry<T>* threadRegistry;
    ScriptContext* scriptContext;
};

template <typename T>
class PrototypeChainCache
{
public:
    PrototypeChainCache(_In_ Recycler * recycler);
    void Initialize(_In_ ScriptCacheRegistry<PrototypeChainCache<T>>* scriptRegistry);
    void Register(_In_ Type* type);
    bool Check(_In_ RecyclableObject* object);
    bool CheckProtoChain(_In_ RecyclableObject* prototype);
    void Clear();

    template <typename KeyType>
    void ProcessProperty(
        _In_ DynamicTypeHandler* typeHandler,
        PropertyAttributes attributes,
        _In_ KeyType propertyKey,
        ScriptContext * scriptContext);

private:
    bool CheckProtoChainInternal(_In_ RecyclableObject* prototype);

    Field(JsUtil::List<Type*>) types;
    Field(ScriptCacheRegistry<PrototypeChainCache<T>>*) scriptRegistry;
};

// This cache contains types ensured to have no @@toPrimitive, @@toStringTag, toString, or valueOf properties in it
// and in all objects in its prototype chain.
// The default values of Object.prototype.toString and Object.prototype.valueOf are also permitted.
class NoSpecialPropertyCache
{
public:
    template <typename KeyType>
    static bool ProcessProperty(
        _In_ DynamicTypeHandler* typeHandler,
        PropertyAttributes attributes,
        _In_ KeyType propertyKey,
        ScriptContext * scriptContext);
    static bool CheckObject(_In_ RecyclableObject* object);
    static bool IsCached(_In_ RecyclableObject* prototype);

    static void ClearType(_In_ Type* type);

    static void Cache(_In_ Type* type);

    static bool IsSpecialProperty(PropertyId propertyId);
    static bool IsSpecialProperty(_In_ PropertyRecord const* propertyRecord);
    static bool IsSpecialProperty(_In_ JavascriptString* propertyString);

    static bool IsDefaultSpecialProperty(
        _In_ DynamicObject* instance,
        _In_ JavascriptLibrary* library,
        PropertyId propertyId);

    static bool IsDefaultHandledSpecialProperty(PropertyId propertyId);
    static bool IsDefaultHandledSpecialProperty(_In_ PropertyRecord const* propertyRecord);
    static bool IsDefaultHandledSpecialProperty(_In_ JavascriptString* propertyString);
#if DBG
    static void RegistrationAssert(_In_ Type* type);
#endif
};

// This cache contains types ensured to have only writable data properties in it and all objects in its prototype chain
// (i.e., no readonly properties or accessors). Only prototype objects' types are stored in the cache. When something
// in the script context adds a readonly property or accessor to an object that is used as a prototype object, this
// list is cleared. The list is also cleared before garbage collection so that it does not keep growing, and so, it can
// hold strong references to the types.
//
// The cache is used by the type-without-property local inline cache. When setting a property on a type that doesn't
// have the property, to determine whether to promote the object like an object of that type was last promoted, we need
// to ensure that objects in the prototype chain have not acquired a readonly property or setter (ideally, only for that
// property ID, but we just check for any such property). This cache is used to avoid doing this many times, especially
// when the prototype chain is not short.
//
// This cache is only used to invalidate the status of types. The type itself contains a boolean indicating whether it
// and prototypes contain only writable data properties, which is reset upon invalidating the status.
class OnlyWritablePropertyCache
{
public:
    template <typename KeyType>
    static bool ProcessProperty(
        _In_ DynamicTypeHandler* typeHandler,
        PropertyAttributes attributes,
        _In_ KeyType propertyKey,
        ScriptContext * scriptContext);
    static bool CheckObject(_In_ RecyclableObject* object);
    static bool IsCached(_In_ RecyclableObject* prototype);

    static void ClearType(_In_ Type* type);

    static void Cache(_In_ Type* type);
#if DBG
    static void RegistrationAssert(_In_ Type* type);
#endif
};

typedef PrototypeChainCache<OnlyWritablePropertyCache> OnlyWritablePropertyProtoChainCache;
typedef PrototypeChainCache<NoSpecialPropertyCache> NoSpecialPropertyProtoChainCache;

typedef ScriptCacheRegistry<NoSpecialPropertyProtoChainCache> NoSpecialPropertyScriptRegistry;
typedef ScriptCacheRegistry<OnlyWritablePropertyProtoChainCache> OnlyWritablePropertyScriptRegistry;

typedef ThreadCacheRegistry<NoSpecialPropertyProtoChainCache> NoSpecialPropertyThreadRegistry;
typedef ThreadCacheRegistry<OnlyWritablePropertyProtoChainCache> OnlyWritablePropertyThreadRegistry;

template <typename T>
template <typename KeyType>
inline void
PrototypeChainCache<T>::ProcessProperty(
    _In_ DynamicTypeHandler* typeHandler,
    PropertyAttributes attributes,
    _In_ KeyType propertyKey,
    ScriptContext * scriptContext)
{
    if (T::ProcessProperty(typeHandler, attributes, propertyKey, scriptContext))
    {
        this->Clear();
    }
}

template <typename KeyType>
inline bool
NoSpecialPropertyCache::ProcessProperty(
    _In_ DynamicTypeHandler* typeHandler,
    PropertyAttributes attributes,
    _In_ KeyType propertyKey, 
    ScriptContext * scriptContext)
{
    if (IsSpecialProperty(propertyKey))
    {
        typeHandler->SetHasSpecialProperties();
        if (typeHandler->GetFlags() & DynamicTypeHandler::IsPrototypeFlag)
        {
            return true;
        }
    }
    return false;
}

template <typename KeyType>
inline bool
OnlyWritablePropertyCache::ProcessProperty(
    _In_ DynamicTypeHandler* typeHandler,
    PropertyAttributes attributes,
    _In_ KeyType propertyKey,
    ScriptContext * scriptContext)
{
    if (!(attributes & PropertyWritable))
    {
        typeHandler->ClearHasOnlyWritableDataProperties();
        if (typeHandler->GetFlags() & DynamicTypeHandler::IsPrototypeFlag)
        {
            PropertyId propertyId = DynamicTypeHandler::TMapKey_GetPropertyId(scriptContext, propertyKey);
            scriptContext->InvalidateStoreFieldCaches(propertyId);
            return true;
        }
    }
    return false;
}

}

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
class PropertyString : public JavascriptString
{
protected:
    Field(int) hitRate;
    Field(PolymorphicInlineCache*) ldElemInlineCache;
    Field(PolymorphicInlineCache*) stElemInlineCache;
    Field(const Js::PropertyRecord*) propertyRecord;

    DEFINE_VTABLE_CTOR(PropertyString, JavascriptString);

    PropertyString(StaticType* type, const Js::PropertyRecord* propertyRecord);
public:
    virtual Js::PropertyRecord const * GetPropertyRecord(bool dontLookupFromDictionary = false) override
    {
        return this->propertyRecord;
    }

    PolymorphicInlineCache * GetLdElemInlineCache() const;
    PolymorphicInlineCache * GetStElemInlineCache() const;
    PolymorphicInlineCache * CreateBiggerPolymorphicInlineCache(bool isLdElem);
    void RegisterCacheMiss();
    int GetHitRate() const { return this->hitRate; };
    void RegisterCacheHit() { ++this->hitRate; };
    bool ShouldUseCache() const;

    bool TrySetPropertyFromCache(
        _In_ RecyclableObject *const object,
        _In_ Var propertyValue,
        _In_ ScriptContext *const requestContext,
        const PropertyOperationFlags propertyOperationFlags,
        _Inout_ PropertyValueInfo *const propertyValueInfo);


    template <bool OwnPropertyOnly>
    bool TryGetPropertyFromCache(
        Var const instance,
        RecyclableObject *const object,
        Var *const propertyValue,
        ScriptContext *const requestContext,
        PropertyValueInfo *const propertyValueInfo);

    static PropertyString* New(StaticType* type, const Js::PropertyRecord* propertyRecord, Recycler *recycler);

    virtual void const * GetOriginalStringReference() override;
    virtual RecyclableObject * CloneToScriptContext(ScriptContext* requestContext) override;

    static uint32 GetOffsetOfLdElemInlineCache() { return offsetof(PropertyString, ldElemInlineCache); }
    static uint32 GetOffsetOfStElemInlineCache() { return offsetof(PropertyString, stElemInlineCache); }
    static uint32 GetOffsetOfHitRate() { return offsetof(PropertyString, hitRate); }
    static bool Is(Var var);
    static bool Is(RecyclableObject * var);

    template <typename T> static PropertyString* TryFromVar(T var);

#if ENABLE_DEBUG_CONFIG_OPTIONS
    void DumpCache(bool ldElemCache)
    {
        PolymorphicInlineCache * cache = ldElemCache ? GetLdElemInlineCache() : GetStElemInlineCache();
        Output::Print(_u("PropertyCache HitRate: %i; types: "), this->hitRate);
        for (uint i = 0; i < cache->GetSize(); ++i)
        {
            Output::Print(_u("%p,"), cache->GetInlineCaches()[i].GetType());
        }
        Output::Print(_u("\n"));
    }
#endif
#if ENABLE_TTD
    //Get the associated property id for this string if there is on (e.g. it is a propertystring otherwise return Js::PropertyIds::_none)
    virtual Js::PropertyId TryGetAssociatedPropertyId() const override { return this->propertyRecord->GetPropertyId(); }
#endif
public:
    virtual VTableValue DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtablePropertyString;
    }
};

// Templated so that the Is call dispatchs to different function depending
// on if argument is already a RecyclableObject* or only known to be a Var
//
// In case it is known to be a RecyclableObject*, the Is call skips that check
template <typename T> inline
PropertyString * PropertyString::TryFromVar(T var)
{
    return PropertyString::Is(var)
        ? reinterpret_cast<PropertyString*>(var)
        : nullptr;
}

template <bool OwnPropertyOnly> inline
bool PropertyString::TryGetPropertyFromCache(
    Var const instance,
    RecyclableObject *const object,
    Var *const propertyValue,
    ScriptContext *const requestContext,
    PropertyValueInfo *const propertyValueInfo)
{
    if (ShouldUseCache())
    {
        PropertyValueInfo::SetCacheInfo(propertyValueInfo, this, GetLdElemInlineCache(), true /* allowResizing */);

        // Some caches will look at prototype, so GetOwnProperty lookups must not check these
        bool found = CacheOperators::TryGetProperty<
            true,               // CheckLocal
            !OwnPropertyOnly,   // CheckProto
            !OwnPropertyOnly,   // CheckAccessor
            !OwnPropertyOnly,   // CheckMissing
            true,               // CheckPolymorphicInlineCache
            !OwnPropertyOnly,   // CheckTypePropertyCache
            false,              // IsInlineCacheAvailable
            true,               // IsPolymorphicInlineCacheAvailable
            false>              // ReturnOperationInfo
            (instance,
                false, // isRoot
                object,
                this->propertyRecord->GetPropertyId(),
                propertyValue,
                requestContext,
                nullptr, // operationInfo
                propertyValueInfo);

        if (found)
        {
            RegisterCacheHit();
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
            if (PHASE_TRACE1(PropertyStringCachePhase))
            {
                Output::Print(_u("PropertyCache: GetElem cache hit for '%s': type %p\n"), GetString(), object->GetType());
            }
#endif
            return true;
        }
    }

    RegisterCacheMiss();
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    if (PHASE_TRACE1(PropertyStringCachePhase))
    {
        Output::Print(_u("PropertyCache: GetElem cache miss for '%s': type %p, index %d\n"),
            GetString(),
            object->GetType(),
            GetLdElemInlineCache()->GetInlineCacheIndexForType(object->GetType()));
        DumpCache(true);
    }
#endif
    return false;
}

} // namespace Js

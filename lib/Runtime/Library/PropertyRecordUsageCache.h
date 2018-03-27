//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    // Keeps track of inline caches for loads and stores on the contained PropertyRecord.
    // Used in PropertyString and JavascriptSymbol.
    class PropertyRecordUsageCache sealed
    {
    private:
        Field(const PropertyRecord*) propertyRecord;
        Field(PolymorphicInlineCache*) ldElemInlineCache;
        Field(PolymorphicInlineCache*) stElemInlineCache;
        Field(int) hitRate;

    public:
        PropertyRecordUsageCache();
        PropertyRecordUsageCache(StaticType* type, const PropertyRecord* propertyRecord);

        const PropertyRecord* GetPropertyRecord() const { return propertyRecord; }

        PolymorphicInlineCache* GetLdElemInlineCache() const;
        PolymorphicInlineCache* GetStElemInlineCache() const;
        PolymorphicInlineCache* CreateBiggerPolymorphicInlineCache(bool isLdElem);
        void RegisterCacheMiss();
        int GetHitRate() const { return this->hitRate; };
        void RegisterCacheHit() { ++this->hitRate; };
        bool ShouldUseCache() const;

        static uint32 GetOffsetOfLdElemInlineCache() { return offsetof(PropertyRecordUsageCache, ldElemInlineCache); }
        static uint32 GetOffsetOfStElemInlineCache() { return offsetof(PropertyRecordUsageCache, stElemInlineCache); }
        static uint32 GetOffsetOfHitRate() { return offsetof(PropertyRecordUsageCache, hitRate); }

        bool TrySetPropertyFromCache(
            _In_ RecyclableObject *const object,
            _In_ Var propertyValue,
            _In_ ScriptContext *const requestContext,
            const PropertyOperationFlags propertyOperationFlags,
            _Inout_ PropertyValueInfo *const propertyValueInfo,
            RecyclableObject *const owner /* Object that this usage cache is part of */);


        template <bool OwnPropertyOnly>
        bool TryGetPropertyFromCache(
            Var const instance,
            RecyclableObject *const object,
            Var *const propertyValue,
            ScriptContext *const requestContext,
            PropertyValueInfo *const propertyValueInfo,
            RecyclableObject *const owner /* Object that this usage cache is part of */);

#if ENABLE_DEBUG_CONFIG_OPTIONS

        const char16* GetString()
        {
            return propertyRecord->GetBuffer();
        }

    private:
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
    };

    template <bool OwnPropertyOnly> inline
    bool PropertyRecordUsageCache::TryGetPropertyFromCache(
        Var const instance,
        RecyclableObject *const object,
        Var *const propertyValue,
        ScriptContext *const requestContext,
        PropertyValueInfo *const propertyValueInfo,
        RecyclableObject *const owner /* Object that this usage cache is part of */)
    {
        if (ShouldUseCache())
        {
            PropertyValueInfo::SetCacheInfo(propertyValueInfo, owner, this, GetLdElemInlineCache(), true /* allowResizing */);

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
                if (PHASE_TRACE1(PropertyCachePhase))
                {
                    Output::Print(_u("PropertyCache: GetElem cache hit for '%s': type %p\n"),
                        GetString(),
                        object->GetType());
                }
#endif
                return true;
            }
        }

        RegisterCacheMiss();
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        if (PHASE_TRACE1(PropertyCachePhase))
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
}

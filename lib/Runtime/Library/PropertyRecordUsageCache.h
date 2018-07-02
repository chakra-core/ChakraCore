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

        bool ShouldDisableWriteCache() const { return propertyRecord && propertyRecord->ShouldDisableWriteCache(); }

        static uint32 GetOffsetOfLdElemInlineCache() { return offsetof(PropertyRecordUsageCache, ldElemInlineCache); }
        static uint32 GetOffsetOfStElemInlineCache() { return offsetof(PropertyRecordUsageCache, stElemInlineCache); }
        static uint32 GetOffsetOfHitRate() { return offsetof(PropertyRecordUsageCache, hitRate); }

        template <bool ReturnOperationInfo>
        _Success_(return) bool TrySetPropertyFromCache(
            _In_ RecyclableObject *const object,
            _In_ Var propertyValue,
            _In_ ScriptContext *const requestContext,
            const PropertyOperationFlags propertyOperationFlags,
            _Out_ PropertyValueInfo *const propertyValueInfo,
            _In_ RecyclableObject *const owner, // Object that this usage cache is part of
            _Out_opt_ PropertyCacheOperationInfo* operationInfo);


        template <
            bool OwnPropertyOnly,
            bool OutputExistence, // When set, propertyValue represents whether the property exists on the instance, not its actual value
            bool ReturnOperationInfo>
        _Success_(return) bool TryGetPropertyFromCache(
            _In_ Var const instance,
            _In_ RecyclableObject *const object,
            _Out_ Var* const propertyValue,
            _In_ ScriptContext* const requestContext,
            _Out_ PropertyValueInfo* const propertyValueInfo,
            _In_ RecyclableObject* const owner, // Object that this usage cache is part of
            _Out_opt_ PropertyCacheOperationInfo* operationInfo)
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
                    ReturnOperationInfo,// ReturnOperationInfo
                    OutputExistence>    // OutputExistence
                        (instance,
                        false, // isRoot
                        object,
                        this->propertyRecord->GetPropertyId(),
                        propertyValue,
                        requestContext,
                        operationInfo,
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


    template <bool ReturnOperationInfo>
    _Success_(return) inline bool PropertyRecordUsageCache::TrySetPropertyFromCache(
        _In_ RecyclableObject *const object,
        _In_ Var propertyValue,
        _In_ ScriptContext *const requestContext,
        const PropertyOperationFlags propertyOperationFlags,
        _Out_ PropertyValueInfo *const propertyValueInfo,
        _In_ RecyclableObject *const owner, // Object that this usage cache is part of
        _Out_opt_ PropertyCacheOperationInfo* operationInfo)
    {
        if (ShouldUseCache())
        {
            PropertyValueInfo::SetCacheInfo(propertyValueInfo, owner, this, GetStElemInlineCache(), true /* allowResizing */);
            bool found = CacheOperators::TrySetProperty<
                true,   // CheckLocal
                true,   // CheckLocalTypeWithoutProperty
                true,   // CheckAccessor
                true,   // CheckPolymorphicInlineCache
                true,   // CheckTypePropertyCache
                false,  // IsInlineCacheAvailable
                true,   // IsPolymorphicInlineCacheAvailable
                ReturnOperationInfo>
                (object,
                    false, // isRoot
                    this->propertyRecord->GetPropertyId(),
                    propertyValue,
                    requestContext,
                    propertyOperationFlags,
                    operationInfo,
                    propertyValueInfo);

            if (found)
            {
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
                if (PHASE_TRACE1(PropertyCachePhase))
                {
                    Output::Print(_u("PropertyCache: SetElem cache hit for '%s': type %p\n"), GetString(), object->GetType());
                }
#endif
                RegisterCacheHit();
                return true;
            }
        }
        RegisterCacheMiss();
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        if (PHASE_TRACE1(PropertyCachePhase))
        {
            Output::Print(_u("PropertyCache: SetElem cache miss for '%s': type %p, index %d\n"),
                GetString(),
                object->GetType(),
                GetStElemInlineCache()->GetInlineCacheIndexForType(object->GetType()));
            DumpCache(false);
        }
#endif
        return false;
    }

}


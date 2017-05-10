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
        DECLARE_CONCRETE_STRING_CLASS;

        PropertyString(StaticType* type, const Js::PropertyRecord* propertyRecord);
    public:
        PolymorphicInlineCache * GetLdElemInlineCache() const;
        PolymorphicInlineCache * GetStElemInlineCache() const;
        Js::PropertyRecord const * GetPropertyRecord() const { return this->propertyRecord; }
        PolymorphicInlineCache * CreateBiggerPolymorphicInlineCache(bool isLdElem);
        void LogCacheMiss();
        int GetHitRate() const { return this->hitRate; };
        void LogCacheHit() { ++this->hitRate; };
        bool ShouldUseCache() const;

        static PropertyString* New(StaticType* type, const Js::PropertyRecord* propertyRecord, Recycler *recycler);

        virtual void const * GetOriginalStringReference() override;
        virtual RecyclableObject * CloneToScriptContext(ScriptContext* requestContext) override;

        static uint32 GetOffsetOfLdElemInlineCache() { return offsetof(PropertyString, ldElemInlineCache); }
        static uint32 GetOffsetOfStElemInlineCache() { return offsetof(PropertyString, stElemInlineCache); }
        static uint32 GetOffsetOfHitRate() { return offsetof(PropertyString, hitRate); }

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
}

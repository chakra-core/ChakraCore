//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    struct PropertyCache
    {
        Type * type;
        union
        {
            struct
            {
                uint16 preventdataSlotIndexFalseRef;
                uint16 dataSlotIndex;
            };
            intptr_t ptrSlot1;
        };
        union
        {
            struct
            {
                uint16 preventFlagsFalseRef;
                bool isInlineSlot;
                bool isStoreFieldEnabled;
            };
            intptr_t ptrSlot2;
        };
        intptr_t blank;
    };

    CompileAssert(sizeof(PropertyCache) == sizeof(InlineCacheAllocator::CacheLayout));
    CompileAssert(offsetof(PropertyCache, blank) == offsetof(InlineCacheAllocator::CacheLayout, strongRef));

    class PropertyString : public JavascriptString
    {
    protected:
        PropertyCache* propCache;
        const Js::PropertyRecord* m_propertyRecord;
        DEFINE_VTABLE_CTOR(PropertyString, JavascriptString);
        DECLARE_CONCRETE_STRING_CLASS;

        PropertyString(StaticType* type, const Js::PropertyRecord* propertyRecord);
    public:
        PropertyCache const * GetPropertyCache() const;
        void ClearPropertyCache();
        Js::PropertyRecord const * GetPropertyRecord() const { return m_propertyRecord; }
        static PropertyString* New(StaticType* type, const Js::PropertyRecord* propertyRecord, Recycler *recycler);
        static PropertyString* New(StaticType* type, const Js::PropertyRecord* propertyRecord, ArenaAllocator *arena);
        void UpdateCache(Type * type, uint16 dataSlotIndex, bool isInlineSlot, bool isStoreFieldEnabled);
        void ClearCache() { propCache->type = nullptr; }

        virtual void const * GetOriginalStringReference() override;
        virtual RecyclableObject * CloneToScriptContext(ScriptContext* requestContext) override;
        virtual bool IsArenaAllocPropertyString() { return false; }

        static uint32 GetOffsetOfPropertyCache() { return offsetof(PropertyString, propCache); }

#if ENABLE_TTD
        //Get the associated property id for this string if there is on (e.g. it is a propertystring otherwise return Js::PropertyIds::_none)
        virtual Js::PropertyId TryGetAssociatedPropertyId() const override { return this->m_propertyRecord->GetPropertyId(); }
#endif
    };

    class ArenaAllocPropertyString sealed : public PropertyString
    {
        friend PropertyString;
    protected:
        ArenaAllocPropertyString(StaticType* type, const Js::PropertyRecord* propertyRecord);
    public:
        virtual bool IsArenaAllocPropertyString() override { return true; }
    };
}

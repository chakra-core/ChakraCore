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
    Field(PropertyRecordUsageCache) propertyRecordUsageCache;

    DEFINE_VTABLE_CTOR(PropertyString, JavascriptString);

    PropertyString(StaticType* type, const Js::PropertyRecord* propertyRecord);
public:
    virtual void GetPropertyRecord(_Out_ PropertyRecord const** propertyRecord, bool dontLookupFromDictionary = false) override
    {
        *propertyRecord = this->propertyRecordUsageCache.GetPropertyRecord();
    }

    Js::PropertyId GetPropertyId()
    {
        return this->propertyRecordUsageCache.GetPropertyRecord()->GetPropertyId();
    }

    PolymorphicInlineCache * GetLdElemInlineCache() const;
    PolymorphicInlineCache * GetStElemInlineCache() const;
    PropertyRecordUsageCache * GetPropertyRecordUsageCache();

    bool TrySetPropertyFromCache(
        _In_ RecyclableObject *const object,
        _In_ Var propertyValue,
        _In_ ScriptContext *const requestContext,
        const PropertyOperationFlags propertyOperationFlags,
        _Inout_ PropertyValueInfo *const propertyValueInfo);


    template <
        bool OwnPropertyOnly,
        bool OutputExistence /*When set, propertyValue represents whether the property exists on the instance, not its actual value*/>
    bool TryGetPropertyFromCache(
        Var const instance,
        RecyclableObject *const object,
        Var *const propertyValue,
        ScriptContext *const requestContext,
        PropertyValueInfo *const propertyValueInfo)
    {
        return this->propertyRecordUsageCache.TryGetPropertyFromCache<OwnPropertyOnly, OutputExistence, false /* ReturnOperationInfo */>(instance, object, propertyValue, requestContext, propertyValueInfo, this, nullptr);
    }

    static PropertyString* New(StaticType* type, const Js::PropertyRecord* propertyRecord, Recycler *recycler);

    virtual void const * GetOriginalStringReference() override;
    virtual RecyclableObject * CloneToScriptContext(ScriptContext* requestContext) override;

    static uint32 GetOffsetOfLdElemInlineCache() { return offsetof(PropertyString, propertyRecordUsageCache) + PropertyRecordUsageCache::GetOffsetOfLdElemInlineCache(); }
    static uint32 GetOffsetOfStElemInlineCache() { return offsetof(PropertyString, propertyRecordUsageCache) + PropertyRecordUsageCache::GetOffsetOfStElemInlineCache(); }
    static uint32 GetOffsetOfHitRate() { return offsetof(PropertyString, propertyRecordUsageCache) + PropertyRecordUsageCache::GetOffsetOfHitRate(); }

#if ENABLE_TTD
    //Get the associated property id for this string if there is on (e.g. it is a propertystring otherwise return Js::PropertyIds::_none)
    virtual Js::PropertyId TryGetAssociatedPropertyId() const override { return this->propertyRecordUsageCache.GetPropertyRecord()->GetPropertyId(); }
#endif
public:
    virtual VTableValue DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtablePropertyString;
    }
};

template <> bool VarIsImpl<PropertyString>(RecyclableObject * obj);

} // namespace Js

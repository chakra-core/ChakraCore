//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeTypePch.h"

namespace Js
{
    // -------------------------------------------------------------------------------------------------------------------------
    // TypePropertyCacheElement
    // -------------------------------------------------------------------------------------------------------------------------

    TypePropertyCacheElement::TypePropertyCacheElement()
        : id(Constants::NoProperty), tag(1), index(0), prototypeObjectWithProperty(nullptr)
    {
    }

    PropertyId TypePropertyCacheElement::Id() const
    {
        return id;
    }

    PropertyIndex TypePropertyCacheElement::Index() const
    {
        return index;
    }

    bool TypePropertyCacheElement::IsInlineSlot() const
    {
        return isInlineSlot;
    }

    bool TypePropertyCacheElement::IsSetPropertyAllowed() const
    {
        return isSetPropertyAllowed;
    }

    bool TypePropertyCacheElement::IsMissing() const
    {
        return isMissing;
    }

    DynamicObject *TypePropertyCacheElement::PrototypeObjectWithProperty() const
    {
        return prototypeObjectWithProperty;
    }

    void TypePropertyCacheElement::Cache(
        const PropertyId id,
        const PropertyIndex index,
        const bool isInlineSlot,
        const bool isSetPropertyAllowed)
    {
        Assert(id != Constants::NoProperty);
        Assert(index != Constants::NoSlot);

        this->id = id;
        this->index = index;
        this->isInlineSlot = isInlineSlot;
        this->isSetPropertyAllowed = isSetPropertyAllowed;
        this->isMissing = false;
        this->prototypeObjectWithProperty = nullptr;
    }

    void TypePropertyCacheElement::Cache(
        const PropertyId id,
        const PropertyIndex index,
        const bool isInlineSlot,
        const bool isSetPropertyAllowed,
        const bool isMissing,
        DynamicObject *const prototypeObjectWithProperty,
        Type *const myParentType)
    {
        Assert(id != Constants::NoProperty);
        Assert(index != Constants::NoSlot);
        Assert(prototypeObjectWithProperty);
        Assert(myParentType);

        if(this->id != id || !this->prototypeObjectWithProperty)
            myParentType->GetScriptContext()->GetThreadContext()->RegisterTypeWithProtoPropertyCache(id, myParentType);

        this->id = id;
        this->index = index;
        this->isInlineSlot = isInlineSlot;
        this->isSetPropertyAllowed = isSetPropertyAllowed;
        this->isMissing = isMissing;
        this->prototypeObjectWithProperty = prototypeObjectWithProperty;
        Assert(this->isMissing == (uint16)(this->prototypeObjectWithProperty == this->prototypeObjectWithProperty->GetLibrary()->GetMissingPropertyHolder()));
    }

    void TypePropertyCacheElement::Clear()
    {
        id = Constants::NoProperty;
    }

    // -------------------------------------------------------------------------------------------------------------------------
    // TypePropertyCache
    // -------------------------------------------------------------------------------------------------------------------------

    size_t TypePropertyCache::ElementIndex(const PropertyId id)
    {
        Assert(id != Constants::NoProperty);
        Assert((TypePropertyCache_NumElements & TypePropertyCache_NumElements - 1) == 0);

        return id & TypePropertyCache_NumElements - 1;
    }

    inline bool TypePropertyCache::TryGetIndexForLoad(
        const bool checkMissing,
        const PropertyId id,
        PropertyIndex *const index,
        bool *const isInlineSlot,
        bool *const isMissing,
        DynamicObject * *const prototypeObjectWithProperty) const
    {
        Assert(index);
        Assert(isInlineSlot);
        Assert(isMissing);
        Assert(prototypeObjectWithProperty);

        const TypePropertyCacheElement &element = elements[ElementIndex(id)];
        if(element.Id() != id || (!checkMissing && element.IsMissing()))
            return false;

        *index = element.Index();
        *isInlineSlot = element.IsInlineSlot();
        *isMissing = checkMissing ? element.IsMissing() : false;
        *prototypeObjectWithProperty = element.PrototypeObjectWithProperty();
        return true;
    }

    inline bool TypePropertyCache::TryGetIndexForStore(
        const PropertyId id,
        PropertyIndex *const index,
        bool *const isInlineSlot) const
    {
        Assert(index);
        Assert(isInlineSlot);

        const TypePropertyCacheElement &element = elements[ElementIndex(id)];
        if(element.Id() != id ||
            !element.IsSetPropertyAllowed() ||
            element.PrototypeObjectWithProperty())
        {
            return false;
        }

        Assert(!element.IsMissing());
        *index = element.Index();
        *isInlineSlot = element.IsInlineSlot();
        return true;
    }

    template <bool OutputExistence /*When set, propertyValue represents whether the property exists on the instance, not its actual value*/>
    bool TypePropertyCache::TryGetProperty(
        const bool checkMissing,
        RecyclableObject *const propertyObject,
        const PropertyId propertyId,
        Var *const propertyValue,
        ScriptContext *const requestContext,
        PropertyCacheOperationInfo *const operationInfo,
        PropertyValueInfo *const propertyValueInfo)
    {
        Assert(propertyValueInfo);
        Assert(propertyValueInfo->GetInlineCache() || propertyValueInfo->GetPolymorphicInlineCache());

        PropertyIndex propertyIndex;
        DynamicObject *prototypeObjectWithProperty;
        bool isInlineSlot, isMissing;
        if(!TryGetIndexForLoad(
                checkMissing,
                propertyId,
                &propertyIndex,
                &isInlineSlot,
                &isMissing,
                &prototypeObjectWithProperty))
        {
        #if DBG_DUMP
            if(PHASE_TRACE1(TypePropertyCachePhase))
            {
                CacheOperators::TraceCache(
                    static_cast<InlineCache *>(nullptr),
                    _u("TypePropertyCache get miss"),
                    propertyId,
                    requestContext,
                    propertyObject);
            }
        #endif
            return false;
        }

        if(!prototypeObjectWithProperty)
        {
        #if DBG_DUMP
            if(PHASE_TRACE1(TypePropertyCachePhase))
            {
                CacheOperators::TraceCache(
                    static_cast<InlineCache *>(nullptr),
                    _u("TypePropertyCache get hit"),
                    propertyId,
                    requestContext,
                    propertyObject);
            }
        #endif

        #if DBG
            const PropertyIndex typeHandlerPropertyIndex =
                VarTo<DynamicObject>(propertyObject)
                    ->GetDynamicType()
                    ->GetTypeHandler()
                    ->InlineOrAuxSlotIndexToPropertyIndex(propertyIndex, isInlineSlot);
            Assert(typeHandlerPropertyIndex == propertyObject->GetPropertyIndex(propertyId));
        #endif
            if (OutputExistence)
            {
                *propertyValue = JavascriptBoolean::ToVar(!isMissing, requestContext);
                Assert(isMissing == !JavascriptOperators::HasProperty(propertyObject, propertyId));
            }
            else
            {
                *propertyValue =
                    isInlineSlot
                        ? VarTo<DynamicObject>(propertyObject)->GetInlineSlot(propertyIndex)
                        : VarTo<DynamicObject>(propertyObject)->GetAuxSlot(propertyIndex);
            }

            if(propertyObject->GetScriptContext() == requestContext)
            {
                if (!OutputExistence)
                {
                    DebugOnly(Var getPropertyValue = JavascriptOperators::GetProperty(propertyObject, propertyId, requestContext));
                    Assert(*propertyValue == getPropertyValue ||
                        (getPropertyValue == requestContext->GetLibrary()->GetNull() && requestContext->GetThreadContext()->IsDisableImplicitCall() && propertyObject->GetType()->IsExternal()));
                }

                CacheOperators::Cache<false, true, false>(
                    false,
                    VarTo<DynamicObject>(propertyObject),
                    false,
                    propertyObject->GetType(),
                    nullptr,
                    propertyId,
                    propertyIndex,
                    isInlineSlot,
                    false,
                    0,
                    propertyValueInfo,
                    requestContext);
                return true;
            }
            else if (!OutputExistence)
            {
                *propertyValue = CrossSite::MarshalVar(requestContext, *propertyValue);
            }
            // Cannot use GetProperty and compare results since they may not compare equal when they're marshaled

            if(operationInfo)
            {
                operationInfo->cacheType = CacheType_TypeProperty;
                operationInfo->slotType = isInlineSlot ? SlotType_Inline : SlotType_Aux;
            }
            return true;
        }

    #if DBG_DUMP
        if(PHASE_TRACE1(TypePropertyCachePhase))
        {
            CacheOperators::TraceCache(
                static_cast<InlineCache *>(nullptr),
                _u("TypePropertyCache get hit prototype"),
                propertyId,
                requestContext,
                propertyObject);
        }
    #endif

    #if DBG
        const PropertyIndex typeHandlerPropertyIndex =
            prototypeObjectWithProperty
                ->GetDynamicType()
                ->GetTypeHandler()
                ->InlineOrAuxSlotIndexToPropertyIndex(propertyIndex, isInlineSlot);
        Assert(typeHandlerPropertyIndex == prototypeObjectWithProperty->GetPropertyIndex(propertyId));
    #endif

        if (OutputExistence)
        {
            *propertyValue = JavascriptBoolean::ToVar(!isMissing, requestContext);
            Assert(isMissing == !JavascriptOperators::HasProperty(propertyObject, propertyId));
        }
        else
        {
            *propertyValue =
                isInlineSlot
                    ? prototypeObjectWithProperty->GetInlineSlot(propertyIndex)
                    : prototypeObjectWithProperty->GetAuxSlot(propertyIndex);
        }
        if(prototypeObjectWithProperty->GetScriptContext() == requestContext)
        {
            if (!OutputExistence)
            {
                DebugOnly(Var getPropertyValue = JavascriptOperators::GetProperty(propertyObject, propertyId, requestContext));
                Assert(*propertyValue == getPropertyValue ||
                    // In some cases, such as CustomExternalObject, if implicit calls are disabled GetPropertyQuery may return null. See CustomExternalObject::GetPropertyQuery for an example.
                    (getPropertyValue == requestContext->GetLibrary()->GetNull() && requestContext->GetThreadContext()->IsDisableImplicitCall() && propertyObject->GetType()->IsExternal()));
            }

            if(propertyObject->GetScriptContext() != requestContext)
            {
                return true;
            }

            CacheOperators::Cache<false, true, false>(
                true,
                prototypeObjectWithProperty,
                false,
                propertyObject->GetType(),
                nullptr,
                propertyId,
                propertyIndex,
                isInlineSlot,
                isMissing,
                0,
                propertyValueInfo,
                requestContext);
            return true;
        }
        else if (!OutputExistence)
        {
            *propertyValue = CrossSite::MarshalVar(requestContext, *propertyValue);
        }
        // Cannot use GetProperty and compare results since they may not compare equal when they're marshaled

        if(operationInfo)
        {
            operationInfo->cacheType = CacheType_TypeProperty;
            operationInfo->slotType = isInlineSlot ? SlotType_Inline : SlotType_Aux;
        }
        return true;
    }
    template bool TypePropertyCache::TryGetProperty<false>(
        const bool checkMissing,
        RecyclableObject *const propertyObject,
        const PropertyId propertyId,
        Var *const propertyValue,
        ScriptContext *const requestContext,
        PropertyCacheOperationInfo *const operationInfo,
        PropertyValueInfo *const propertyValueInfo);
    template bool TypePropertyCache::TryGetProperty<true>(
        const bool checkMissing,
        RecyclableObject *const propertyObject,
        const PropertyId propertyId,
        Var *const propertyValue,
        ScriptContext *const requestContext,
        PropertyCacheOperationInfo *const operationInfo,
        PropertyValueInfo *const propertyValueInfo);

    bool TypePropertyCache::TrySetProperty(
        RecyclableObject *const object,
        const PropertyId propertyId,
        Var propertyValue,
        ScriptContext *const requestContext,
        PropertyCacheOperationInfo *const operationInfo,
        PropertyValueInfo *const propertyValueInfo)
    {
        Assert(propertyValueInfo);
        Assert(propertyValueInfo->GetInlineCache() || propertyValueInfo->GetPolymorphicInlineCache());

        PropertyIndex propertyIndex;
        bool isInlineSlot;
        if(!TryGetIndexForStore(propertyId, &propertyIndex, &isInlineSlot))
        {
        #if DBG_DUMP
            if(PHASE_TRACE1(TypePropertyCachePhase))
            {
                CacheOperators::TraceCache(
                    static_cast<InlineCache *>(nullptr),
                    _u("TypePropertyCache set miss"),
                    propertyId,
                    requestContext,
                    object);
            }
        #endif
            return false;
        }

    #if DBG_DUMP
        if(PHASE_TRACE1(TypePropertyCachePhase))
        {
            CacheOperators::TraceCache(
                static_cast<InlineCache *>(nullptr),
                _u("TypePropertyCache set hit"),
                propertyId,
                requestContext,
                object);
        }
    #endif

#if ENABLE_FIXED_FIELDS
        Assert(!object->IsFixedProperty(propertyId));
#endif
        Assert(
            (
                VarTo<DynamicObject>(object)
                    ->GetDynamicType()
                    ->GetTypeHandler()
                    ->InlineOrAuxSlotIndexToPropertyIndex(propertyIndex, isInlineSlot)
            ) ==
            object->GetPropertyIndex(propertyId));
        Assert(object->CanStorePropertyValueDirectly(propertyId, false));

        ScriptContext *const objectScriptContext = object->GetScriptContext();
        // force check: propertyValue's context != object's context.
        // TODO: investigate why?
        propertyValue = CrossSite::MarshalVar(objectScriptContext, propertyValue);

        if(isInlineSlot)
        {
            VarTo<DynamicObject>(object)->SetInlineSlot(SetSlotArguments(propertyId, propertyIndex, propertyValue));
        }
        else
        {
            VarTo<DynamicObject>(object)->SetAuxSlot(SetSlotArguments(propertyId, propertyIndex, propertyValue));
        }

        if(objectScriptContext == requestContext)
        {
            CacheOperators::Cache<false, false, false>(
                false,
                VarTo<DynamicObject>(object),
                false,
                object->GetType(),
                nullptr,
                propertyId,
                propertyIndex,
                isInlineSlot,
                false,
                0,
                propertyValueInfo,
                requestContext);
            return true;
        }

        if(operationInfo)
        {
            operationInfo->cacheType = CacheType_TypeProperty;
            operationInfo->slotType = isInlineSlot ? SlotType_Inline : SlotType_Aux;
        }
        return true;
    }

    void TypePropertyCache::Cache(
        const PropertyId id,
        const PropertyIndex index,
        const bool isInlineSlot,
        const bool isSetPropertyAllowed)
    {
        elements[ElementIndex(id)].Cache(id, index, isInlineSlot, isSetPropertyAllowed);
    }

    void TypePropertyCache::Cache(
        const PropertyId id,
        const PropertyIndex index,
        const bool isInlineSlot,
        const bool isSetPropertyAllowed,
        const bool isMissing,
        DynamicObject *const prototypeObjectWithProperty,
        Type *const myParentType)
    {
        Assert(myParentType);
        Assert(myParentType->GetPropertyCache() == this);

        elements[ElementIndex(id)].Cache(
            id,
            index,
            isInlineSlot,
            isSetPropertyAllowed,
            isMissing,
            prototypeObjectWithProperty,
            myParentType);
    }

    void TypePropertyCache::ClearIfPropertyIsOnAPrototype(const PropertyId id)
    {
        TypePropertyCacheElement &element = elements[ElementIndex(id)];
        if(element.Id() == id && element.PrototypeObjectWithProperty())
            element.Clear();
    }

    void TypePropertyCache::Clear(const PropertyId id)
    {
        TypePropertyCacheElement &element = elements[ElementIndex(id)];
        if(element.Id() == id)
            element.Clear();
    }
}

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLanguagePch.h"

#if DBG
#include "Types/PathTypeHandler.h"
#endif

namespace Js
{
    void CacheOperators::CachePropertyRead(
        Var startingObject,
        RecyclableObject * objectWithProperty,
        const bool isRoot,
        PropertyId propertyId,
        const bool isMissing,
        PropertyValueInfo* info,
        ScriptContext * requestContext)
    {
        if (!CanCachePropertyRead(info, objectWithProperty, requestContext))
        {
            return;
        }

        AssertMsg(!(info->GetFlags() & InlineCacheGetterFlag), "We cache getters only in DictionaryTypeHandler::GetProperty() before they were executed");
        AssertMsg((info->GetFlags() & InlineCacheSetterFlag) == 0, "invalid setter flag in CachePropertyRead");
        if (info->GetInstance() != objectWithProperty) // We can't cache if slot owner is not the object
        {
            AssertMsg(info->IsNoCache() || objectWithProperty->GetPropertyIndex(propertyId) == Constants::NoSlot, "Missed updating PropertyValueInfo?");
            return;
        }

        PropertyIndex propertyIndex = info->GetPropertyIndex();

        Assert(propertyIndex == objectWithProperty->GetPropertyIndex(propertyId) ||
            (VarIs<RootObjectBase>(objectWithProperty) && propertyIndex == VarTo<RootObjectBase>(objectWithProperty)->GetRootPropertyIndex(propertyId)));
        Assert(DynamicType::Is(objectWithProperty->GetTypeId()));

#if ENABLE_FIXED_FIELDS
        // We are populating a cache guarded by the instance's type (not the type of the object with property somewhere in the prototype chain),
        // so we only care if the instance's property (if any) is fixed.
        Assert(info->IsNoCache() || !info->IsStoreFieldCacheEnabled() || info->GetInstance() != objectWithProperty || !objectWithProperty->IsFixedProperty(propertyId));
#endif

        DynamicObject * dynamicObjectWithProperty = VarTo<DynamicObject>(objectWithProperty);
        PropertyIndex slotIndex;
        bool isInlineSlot;
        dynamicObjectWithProperty->GetDynamicType()->GetTypeHandler()->PropertyIndexToInlineOrAuxSlotIndex(propertyIndex, &slotIndex, &isInlineSlot);

        const bool isProto = objectWithProperty != startingObject;
        if(!isProto)
        {
            Assert(info->IsWritable() == (objectWithProperty->IsWritable(propertyId) ? true : false));
            // Because StFld and LdFld caches aren't shared, we can safely cache for LdFld operations even if the property isn't writable.
        }
        else if(
            PropertyValueInfo::PrototypeCacheDisabled((PropertyValueInfo*)info) ||
            !VarIs<RecyclableObject>(startingObject) ||
            UnsafeVarTo<RecyclableObject>(startingObject)->GetScriptContext() != requestContext)
        {
            // Don't need to cache if the beginning property is number etc.
            return;
        }

#ifdef TELEMETRY_AddToCache
        // For performance reasons, only execute this code in interpreted mode, not JIT.
        // This method only returns true in interpreted mode and can be used to detect interpreted mode.
        if (info->AllowResizingPolymorphicInlineCache())
        {
            if (TELEMETRY_PROPERTY_OPCODE_FILTER(propertyId))
            {
                requestContext->GetTelemetry().GetOpcodeTelemetry().GetProperty(objectWithProperty, propertyId, nullptr);
            }
        }
#endif

        Cache<false, true, true>(
            isProto,
            dynamicObjectWithProperty,
            isRoot,
            VarTo<RecyclableObject>(startingObject)->GetType(),
            nullptr,
            propertyId,
            slotIndex,
            isInlineSlot,
            isMissing,
            0,
            info,
            requestContext);
    }

    void CacheOperators::CachePropertyReadForGetter(
        PropertyValueInfo *info,
        Var originalInstance,
        JsUtil::CharacterBuffer<WCHAR> const& propertyName,
        ScriptContext* requestContext)
    {
        PropertyRecord const* propertyRecord;
        requestContext->GetOrAddPropertyRecord(propertyName, &propertyRecord);
        CachePropertyReadForGetter(info, originalInstance, propertyRecord->GetPropertyId(), requestContext);
    }

    void CacheOperators::CachePropertyReadForGetter(
        PropertyValueInfo *info,
        Var originalInstance,
        PropertyId propertyId,
        ScriptContext* requestContext)
    {
        RecyclableObject* originalObj = JavascriptOperators::TryFromVar<RecyclableObject>(originalInstance);
        if (!info || !originalObj || !CacheOperators::CanCachePropertyRead(info, info->GetInstance(), requestContext))
        {
            return;
        }

        Assert(DynamicType::Is(info->GetInstance()->GetTypeId()));

        DynamicObject * dynamicInstance = VarTo<DynamicObject>(info->GetInstance());
        PropertyIndex slotIndex;
        bool isInlineSlot;
        dynamicInstance->GetDynamicType()->GetTypeHandler()->PropertyIndexToInlineOrAuxSlotIndex(info->GetPropertyIndex(), &slotIndex, &isInlineSlot);

        const bool isProto = info->GetInstance() != originalInstance;
        if (isProto && originalObj->GetScriptContext() != requestContext)
        {
            // Don't need to cache if the beginning property is number etc.
            return;
        }

#ifdef TELEMETRY_AddToCache
        if (info->AllowResizingPolymorphicInlineCache()) // If in interpreted mode, not JIT.
        {
            if (TELEMETRY_PROPERTY_OPCODE_FILTER(propertyId))
            {
                requestContext->GetTelemetry().GetOpcodeTelemetry().GetProperty(info->GetInstance(), propertyId, nullptr);
            }
        }
#endif


        Cache<true, true, false>(
            isProto,
            dynamicInstance,
            false,
            originalObj->GetType(),
            nullptr,
            propertyId,
            slotIndex,
            isInlineSlot,
            false,
            0,
            info,
            requestContext);
    }

    void CacheOperators::CachePropertyWrite(
        RecyclableObject * object,
        const bool isRoot,
        Type* typeWithoutProperty,
        PropertyId propertyId,
        PropertyValueInfo* info,
        ScriptContext * requestContext)
    {
        Assert(typeWithoutProperty != nullptr);

        if (!CacheOperators::CanCachePropertyWrite(info, object, requestContext))
        {
            return;
        }

        BOOL isSetter = (info->GetFlags() == InlineCacheSetterFlag);

        if (info->GetInstance() != object) // We can't cache if slot owner is not the object
        {
            if (!isSetter)
            {
                AssertMsg(info->IsNoCache() || object->GetPropertyIndex(propertyId) == Constants::NoSlot, "Missed updating PropertyValueInfo?");
                return;
            }
        }

        PropertyIndex propertyIndex = info->GetPropertyIndex();
        if (isSetter)
        {
            if (propertyIndex & 0xf000)
            {
                return;
            }
        }
        else
        {
            if (propertyIndex == Constants::NoSlot)
            {
                return;
            }
        }

        Assert((!isRoot && propertyIndex == object->GetPropertyIndex(propertyId)) || isSetter ||
            (isRoot && propertyIndex == VarTo<RootObjectBase>(object)->GetRootPropertyIndex(propertyId)));
        Assert(DynamicType::Is(object->GetTypeId()));
        AssertMsg((info->GetFlags() & InlineCacheGetterFlag) == 0, "invalid getter for CachePropertyWrite");

        RecyclableObject* instance = info->GetInstance();
        DynamicObject * dynamicInstance = VarTo<DynamicObject>(instance);
        PropertyIndex slotIndex;
        bool isInlineSlot;
        dynamicInstance->GetDynamicType()->GetTypeHandler()->PropertyIndexToInlineOrAuxSlotIndex(propertyIndex, &slotIndex, &isInlineSlot);

        if (!isSetter)
        {
            AssertMsg(instance == object, "invalid instance for non setter");
            Assert(DynamicType::Is(typeWithoutProperty));
#if ENABLE_FIXED_FIELDS
            Assert(info->IsNoCache() || !info->IsStoreFieldCacheEnabled() || object->CanStorePropertyValueDirectly(propertyId, isRoot));
#endif
            Assert(info->IsWritable());

            DynamicType* newType = (DynamicType*)object->GetType();
            DynamicType* oldType = (DynamicType*)typeWithoutProperty;

            Assert(newType);

            int requiredAuxSlotCapacity;
            // Don't cache property adds for types that aren't (yet) shared.  We must go down the slow path to force type sharing
            // and invalidate any potential fixed fields this type may have.
            // Don't cache property adds to prototypes, so we don't have to check if the object is a prototype on the fast path.
            if (newType != oldType && newType->GetIsShared() && newType->GetTypeHandler()->IsPathTypeHandler() && (!oldType->GetTypeHandler()->GetIsPrototype()))
            {
                DynamicTypeHandler* oldTypeHandler = oldType->GetTypeHandler();
                DynamicTypeHandler* newTypeHandler = newType->GetTypeHandler();

                // This may be the transition from deferred type handler to path type handler. Don't try to cache now.
                if (!oldTypeHandler->IsPathTypeHandler())
                {
                    return;
                }

                int oldCapacity = oldTypeHandler->GetSlotCapacity();
                int newCapacity = newTypeHandler->GetSlotCapacity();
                int newInlineCapacity = newTypeHandler->GetInlineSlotCapacity();

                // We are adding only one property here.  If some other properties were added as a side effect on the slow path
                // we should never cache the type transition, as the other property slots will not be populated by the fast path.
                AssertMsg(((PathTypeHandlerBase *)oldTypeHandler)->GetPropertyCount() + 1 == ((PathTypeHandlerBase *)newTypeHandler)->GetPropertyCount() ||
                          ((PathTypeHandlerBase *)oldTypeHandler)->GetPropertyCount() == ((PathTypeHandlerBase *)newTypeHandler)->GetPropertyCount(),
                    "Don't cache type transitions that add multiple properties.");

                // InlineCache::TrySetProperty assumes the following invariants to decide if and how to adjust auxiliary slot capacity.
                AssertMsg(DynamicObject::IsTypeHandlerCompatibleForObjectHeaderInlining(oldTypeHandler, newTypeHandler),
                    "TypeHandler should be compatible for transition.");

                // If the slot is inlined then we should never need to adjust auxiliary slot capacity.
                Assert(!isInlineSlot || oldCapacity == newCapacity || newCapacity <= newInlineCapacity);
                // If the slot is not inlined then the new type must have some auxiliary slots.
                Assert(isInlineSlot || newCapacity > newInlineCapacity);

                // If the slot is not inlined and the property being added exceeds the old type's slot capacity, then slotIndex corresponds
                // to the number of occupied auxiliary slots (i.e. the old type's auxiliary slot capacity).
                // If the object is optimized for <=2 properties, then slotIndex should be same as the oldCapacity(as there is no inlineSlots in the new typeHandler).
                Assert(
                    isInlineSlot ||
                    oldCapacity == newCapacity ||
                    slotIndex == oldCapacity - oldTypeHandler->GetInlineSlotCapacity() ||
                    (
                        oldTypeHandler->IsObjectHeaderInlinedTypeHandler() &&
                        newInlineCapacity ==
                            oldTypeHandler->GetInlineSlotCapacity() -
                            DynamicTypeHandler::GetObjectHeaderInlinableSlotCapacity() &&
                        slotIndex == DynamicTypeHandler::GetObjectHeaderInlinableSlotCapacity()
                    ));

                requiredAuxSlotCapacity = (!isInlineSlot && oldCapacity < newCapacity) ? newCapacity - newInlineCapacity : 0;

                // Required auxiliary slot capacity must fit in the available inline cache bits.
                Assert(requiredAuxSlotCapacity < (0x01 << InlineCache::RequiredAuxSlotCapacityBitCount));
            }
            else
            {
                typeWithoutProperty = nullptr;
                requiredAuxSlotCapacity = 0;
            }

            Cache<false, false, true>(
                false,
                VarTo<DynamicObject>(object),
                isRoot,
                object->GetType(),
                typeWithoutProperty,
                propertyId,
                slotIndex,
                isInlineSlot,
                false,
                requiredAuxSlotCapacity,
                info,
                requestContext);
            return;
        }

        const bool isProto = instance != object;
        if(isProto && instance->GetScriptContext() != requestContext)
        {
            // Don't cache if object and prototype are from different context
            return;
        }

        Cache<true, false, false>(
            isProto,
            dynamicInstance,
            false,
            object->GetType(),
            nullptr,
            propertyId,
            slotIndex,
            isInlineSlot,
            false,
            0,
            info,
            requestContext);
    }

    bool CacheOperators::CanCachePropertyRead(const PropertyValueInfo *info, RecyclableObject * object, ScriptContext * requestContext)
    {
        return
            info &&
            info->GetPropertyIndex() != Constants::NoSlot &&
            (info->GetInlineCache() || info->GetPolymorphicInlineCache() || info->GetFunctionBody()) &&
            CanCachePropertyRead(object, requestContext);
    }

    bool CacheOperators::CanCachePropertyRead(RecyclableObject * object, ScriptContext * requestContext)
    {
        return object->GetScriptContext() == requestContext && !PHASE_OFF1(InlineCachePhase);
    }

    bool CacheOperators::CanCachePropertyWrite(const PropertyValueInfo *info, RecyclableObject * object, ScriptContext * requestContext)
    {
        return
            info &&
            (info->GetInlineCache() || info->GetPolymorphicInlineCache() || info->GetFunctionBody()) &&
            CanCachePropertyWrite(object, requestContext);
    }

    bool CacheOperators::CanCachePropertyWrite(RecyclableObject * object, ScriptContext * requestContext)
    {
        return object->GetScriptContext() == requestContext && DynamicType::Is(object->GetTypeId()) && !PHASE_OFF1(InlineCachePhase);
    }

#if DBG_DUMP
    void CacheOperators::TraceCache(InlineCache * inlineCache, const char16 * methodName, PropertyId propertyId, ScriptContext * requestContext, RecyclableObject * object)
    {
        TraceCacheCommon(methodName, propertyId, requestContext, object);
        if(inlineCache)
        {
            Output::Print(_u("Inline Cache: \n  "));
            inlineCache->Dump();
        }
        Output::Print(_u("\n"));
        Output::Flush();
    }

    void CacheOperators::TraceCache(PolymorphicInlineCache * polymorphicInlineCache, const char16 * methodName, PropertyId propertyId, ScriptContext * requestContext, RecyclableObject * object)
    {
        TraceCacheCommon(methodName, propertyId, requestContext, object);
        Output::Print(_u("Polymorphic Inline Cache, size = %d :\n"), polymorphicInlineCache->GetSize());
        polymorphicInlineCache->Dump();
        Output::Flush();
    }

    void CacheOperators::TraceCacheCommon(const char16 * methodName, PropertyId propertyId, ScriptContext * requestContext, RecyclableObject * object)
    {
        if(object)
        {
            JavascriptFunction* caller;
            const WCHAR* callerName = NULL;
            uint lineNumber = 0;
            uint columnNumber = 0;
            if(JavascriptStackWalker::GetCaller(&caller, requestContext))
            {
                FunctionBody *functionBody = caller->GetFunctionBody();
                callerName = functionBody->GetExternalDisplayName();
                lineNumber = functionBody->GetLineNumber();
                columnNumber = functionBody->GetColumnNumber();
            }
            Output::Print(_u("%s, %s, %s(%d:%d), InType: 0x%X "),
                methodName,
                requestContext->GetPropertyName(propertyId)->GetBuffer(),
                callerName,
                lineNumber,
                columnNumber,
                object->GetType());
        }
    }
#endif
}

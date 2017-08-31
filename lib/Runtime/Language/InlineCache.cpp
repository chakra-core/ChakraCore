//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLanguagePch.h"

#if ENABLE_NATIVE_CODEGEN
#include "JITType.h"
#endif

namespace Js
{
    void InlineCache::CacheLocal(
        Type *const type,
        const PropertyId propertyId,
        const PropertyIndex propertyIndex,
        const bool isInlineSlot,
        Type *const typeWithoutProperty,
        int requiredAuxSlotCapacity,
        ScriptContext *const requestContext)
    {
        Assert(type);
        Assert(propertyId != Constants::NoProperty);
        Assert(propertyIndex != Constants::NoSlot);
        Assert(requestContext);
        Assert(type->GetScriptContext() == requestContext);
        DebugOnly(VerifyRegistrationForInvalidation(this, requestContext, propertyId));
        Assert(requiredAuxSlotCapacity >= 0 && requiredAuxSlotCapacity < 0x01 << RequiredAuxSlotCapacityBitCount);
        // Store field and load field caches are never shared so we should never have a prototype cache morphing into an add property cache.
        // We may, however, have a flags cache (setter) change to add property cache.
        Assert(typeWithoutProperty == nullptr || !IsProto());

        requestContext->SetHasUsedInlineCache(true);

        // Add cache into a store field cache list if required, but not there yet.
        if (typeWithoutProperty != nullptr && invalidationListSlotPtr == nullptr)
        {
            // Note, this can throw due to OOM, so we need to do it before the inline cache is set below.
            requestContext->RegisterStoreFieldInlineCache(this, propertyId);
        }

        if (isInlineSlot)
        {
            u.local.type = type;
            u.local.typeWithoutProperty = typeWithoutProperty;
        }
        else
        {
            u.local.type = TypeWithAuxSlotTag(type);
            u.local.typeWithoutProperty = typeWithoutProperty ? TypeWithAuxSlotTag(typeWithoutProperty) : nullptr;
        }

        u.local.isLocal = true;
        u.local.slotIndex = propertyIndex;
        u.local.requiredAuxSlotCapacity = requiredAuxSlotCapacity;

        type->SetHasBeenCached();
        if (typeWithoutProperty)
        {
            typeWithoutProperty->SetHasBeenCached();
        }

        DebugOnly(VerifyRegistrationForInvalidation(this, requestContext, propertyId));

#if DBG_DUMP
        if (PHASE_VERBOSE_TRACE1(Js::InlineCachePhase))
        {
            Output::Print(_u("IC::CacheLocal, %s: "), requestContext->GetPropertyName(propertyId)->GetBuffer());
            Dump();
            Output::Print(_u("\n"));
            Output::Flush();
        }
#endif
    }

    void InlineCache::CacheProto(
        DynamicObject *const prototypeObjectWithProperty,
        const PropertyId propertyId,
        const PropertyIndex propertyIndex,
        const bool isInlineSlot,
        const bool isMissing,
        Type *const type,
        ScriptContext *const requestContext)
    {
        Assert(prototypeObjectWithProperty);
        Assert(propertyId != Constants::NoProperty);
        Assert(propertyIndex != Constants::NoSlot);
        Assert(type);
        Assert(requestContext);
        Assert(prototypeObjectWithProperty->GetScriptContext() == requestContext);
        DebugOnly(VerifyRegistrationForInvalidation(this, requestContext, propertyId));

        // This is an interesting quirk.  In the browser Chakra's global object cannot be used directly as a prototype, because
        // the global object (referenced either as window or as this) always points to the host object.  Thus, when we retrieve
        // a property from Chakra's global object the prototypeObjectWithProperty != info->GetInstance() and we will never cache
        // such property loads (see CacheOperators::CachePropertyRead).  However, in jc.exe or jshost.exe the only global object
        // is Chakra's global object, and so prototypeObjectWithProperty == info->GetInstance() and we can cache.  Hence, the
        // assert below is only correct when running in the browser.
        // Assert(prototypeObjectWithProperty != prototypeObjectWithProperty->type->GetLibrary()->GetGlobalObject());

        // Store field and load field caches are never shared so we should never have an add property cache morphing into a prototype cache.
        Assert(!IsLocal() || u.local.typeWithoutProperty == nullptr);

        requestContext->SetHasUsedInlineCache(true);

        // Add cache into a proto cache list if not there yet.
        if (invalidationListSlotPtr == nullptr)
        {
            // Note, this can throw due to OOM, so we need to do it before the inline cache is set below.
            requestContext->RegisterProtoInlineCache(this, propertyId);
        }

        u.proto.prototypeObject = prototypeObjectWithProperty;
        u.proto.isProto = true;
        u.proto.isMissing = isMissing;
        u.proto.slotIndex = propertyIndex;
        if (isInlineSlot)
        {
            u.proto.type = type;
        }
        else
        {
            u.proto.type = TypeWithAuxSlotTag(type);
        }

        prototypeObjectWithProperty->GetType()->SetHasBeenCached();

        DebugOnly(VerifyRegistrationForInvalidation(this, requestContext, propertyId));
        Assert(u.proto.isMissing == (uint16)(u.proto.prototypeObject == requestContext->GetLibrary()->GetMissingPropertyHolder()));

#if DBG_DUMP
        if (PHASE_VERBOSE_TRACE1(Js::InlineCachePhase))
        {
            Output::Print(_u("IC::CacheProto, %s: "), requestContext->GetPropertyName(propertyId)->GetBuffer());
            Dump();
            Output::Print(_u("\n"));
            Output::Flush();
        }
#endif
    }

    // TODO (InlineCacheCleanup): When simplifying inline caches due to not sharing between loads and stores, create two
    // separate methods CacheSetter and CacheGetter.
    void InlineCache::CacheAccessor(
        const bool isGetter,
        const PropertyId propertyId,
        const PropertyIndex propertyIndex,
        const bool isInlineSlot,
        Type *const type,
        DynamicObject *const object,
        const bool isOnProto,
        ScriptContext *const requestContext)
    {
        Assert(propertyId != Constants::NoProperty);
        Assert(propertyIndex != Constants::NoSlot);
        Assert(type);
        Assert(object);
        Assert(requestContext);
        DebugOnly(VerifyRegistrationForInvalidation(this, requestContext, propertyId));
        // It is possible that prototype is from a different scriptContext than the original instance. We don't want to cache
        // in this case.
        Assert(type->GetScriptContext() == requestContext);

        requestContext->SetHasUsedInlineCache(true);

        if (isOnProto && invalidationListSlotPtr == nullptr)
        {
            // Note, this can throw due to OOM, so we need to do it before the inline cache is set below.
            if (!isGetter)
            {
                // If the setter is on a prototype, this cache must be invalidated whenever proto
                // caches are invalidated, so we must register it here.  Note that store field inline
                // caches are invalidated any time proto caches are invalidated.
                requestContext->RegisterStoreFieldInlineCache(this, propertyId);
            }
            else
            {
                requestContext->RegisterProtoInlineCache(this, propertyId);
            }
        }

        u.accessor.isAccessor = true;
        // TODO (PersistentInlineCaches): Consider removing the flag altogether and just have a bit indicating
        // whether the cache itself is a store field cache (isStore?).
        u.accessor.flags = isGetter ? InlineCacheGetterFlag : InlineCacheSetterFlag;
        u.accessor.isOnProto = isOnProto;
        u.accessor.type = isInlineSlot ? type : TypeWithAuxSlotTag(type);
        u.accessor.slotIndex = propertyIndex;
        u.accessor.object = object;

        type->SetHasBeenCached();

        DebugOnly(VerifyRegistrationForInvalidation(this, requestContext, propertyId));

#if DBG_DUMP
        if (PHASE_VERBOSE_TRACE1(Js::InlineCachePhase))
        {
            Output::Print(_u("IC::CacheAccessor, %s: "), requestContext->GetPropertyName(propertyId)->GetBuffer());
            Dump();
            Output::Print(_u("\n"));
            Output::Flush();
        }
#endif
    }

    bool InlineCache::PretendTryGetProperty(Type *const type, PropertyCacheOperationInfo * operationInfo) const
    {
        if (type == u.local.type)
        {
            operationInfo->cacheType = CacheType_Local;
            operationInfo->slotType = SlotType_Inline;
            return true;
        }

        if (TypeWithAuxSlotTag(type) == u.local.type)
        {
            operationInfo->cacheType = CacheType_Local;
            operationInfo->slotType = SlotType_Aux;
            return true;
        }

        if (type == u.proto.type)
        {
            operationInfo->cacheType = CacheType_Proto;
            operationInfo->slotType = SlotType_Inline;
            return true;
        }

        if (TypeWithAuxSlotTag(type) == u.proto.type)
        {
            operationInfo->cacheType = CacheType_Proto;
            operationInfo->slotType = SlotType_Aux;
            return true;
        }

        if (type == u.accessor.type)
        {
            Assert(u.accessor.flags & InlineCacheGetterFlag);

            operationInfo->cacheType = CacheType_Getter;
            operationInfo->slotType = SlotType_Inline;
            return true;
        }

        if (TypeWithAuxSlotTag(type) == u.accessor.type)
        {
            Assert(u.accessor.flags & InlineCacheGetterFlag);

            operationInfo->cacheType = CacheType_Getter;
            operationInfo->slotType = SlotType_Aux;
            return true;
        }

        return false;
    }

    bool InlineCache::PretendTrySetProperty(Type *const type, Type *const oldType, PropertyCacheOperationInfo * operationInfo) const
    {
        if (oldType == u.local.typeWithoutProperty)
        {
            operationInfo->cacheType = CacheType_LocalWithoutProperty;
            operationInfo->slotType = SlotType_Inline;
            return true;
        }

        if (TypeWithAuxSlotTag(oldType) == u.local.typeWithoutProperty)
        {
            operationInfo->cacheType = CacheType_LocalWithoutProperty;
            operationInfo->slotType = SlotType_Aux;
            return true;
        }

        if (type == u.local.type)
        {
            operationInfo->cacheType = CacheType_Local;
            operationInfo->slotType = SlotType_Inline;
            return true;
        }

        if (TypeWithAuxSlotTag(type) == u.local.type)
        {
            operationInfo->cacheType = CacheType_Local;
            operationInfo->slotType = SlotType_Aux;
            return true;
        }

        if (type == u.accessor.type)
        {
            if (u.accessor.flags & InlineCacheSetterFlag)
            {
                operationInfo->cacheType = CacheType_Setter;
                operationInfo->slotType = SlotType_Inline;
                return true;
            }
        }

        if (TypeWithAuxSlotTag(type) == u.accessor.type)
        {
            if (u.accessor.flags & InlineCacheSetterFlag)
            {
                operationInfo->cacheType = CacheType_Setter;
                operationInfo->slotType = SlotType_Aux;
                return true;
            }
        }

        return false;
    }

    bool InlineCache::GetGetterSetter(Type *const type, RecyclableObject **callee)
    {
        Type *const taggedType = TypeWithAuxSlotTag(type);
        *callee = nullptr;

        if (u.accessor.flags & (InlineCacheGetterFlag | InlineCacheSetterFlag))
        {
            if (type == u.accessor.type)
            {
                *callee = RecyclableObject::FromVar(u.accessor.object->GetInlineSlot(u.accessor.slotIndex));
                return true;
            }
            else if (taggedType == u.accessor.type)
            {
                *callee = RecyclableObject::FromVar(u.accessor.object->GetAuxSlot(u.accessor.slotIndex));
                return true;
            }
        }
        return false;
    }

    bool InlineCache::GetCallApplyTarget(RecyclableObject* obj, RecyclableObject **callee)
    {
        Type *const type = obj->GetType();
        Type *const taggedType = TypeWithAuxSlotTag(type);
        *callee = nullptr;

        if (IsLocal())
        {
            if (type == u.local.type)
            {
                const Var objectAtInlineSlot = DynamicObject::FromVar(obj)->GetInlineSlot(u.local.slotIndex);
                if (!Js::TaggedNumber::Is(objectAtInlineSlot))
                {
                    *callee = RecyclableObject::FromVar(objectAtInlineSlot);
                    return true;
                }
            }
            else if (taggedType == u.local.type)
            {
                const Var objectAtAuxSlot = DynamicObject::FromVar(obj)->GetAuxSlot(u.local.slotIndex);
                if (!Js::TaggedNumber::Is(objectAtAuxSlot))
                {
                    *callee = RecyclableObject::FromVar(DynamicObject::FromVar(obj)->GetAuxSlot(u.local.slotIndex));
                    return true;
                }
            }
            return false;
        }
        else if (IsProto())
        {
            if (type == u.proto.type)
            {
                const Var objectAtInlineSlot = u.proto.prototypeObject->GetInlineSlot(u.proto.slotIndex);
                if (!Js::TaggedNumber::Is(objectAtInlineSlot))
                {
                    *callee = RecyclableObject::FromVar(objectAtInlineSlot);
                    return true;
                }
            }
            else if (taggedType == u.proto.type)
            {
                const Var objectAtAuxSlot = u.proto.prototypeObject->GetAuxSlot(u.proto.slotIndex);
                if (!Js::TaggedNumber::Is(objectAtAuxSlot))
                {
                    *callee = RecyclableObject::FromVar(objectAtAuxSlot);
                    return true;
                }
            }
            return false;
        }
        return false;
    }

    void InlineCache::Clear()
    {
#if DBG
        if (!IsAll((byte*)this, sizeof(InlineCache), 0))
#endif
        {
            memset(this, 0, sizeof(InlineCache));
        }
    }

    void InlineCache::RemoveFromInvalidationListAndClear(ThreadContext* threadContext)
    {
        if (this->RemoveFromInvalidationList())
        {
            threadContext->NotifyInlineCacheBatchUnregistered(1);
        }

        this->Clear();
    }

    InlineCache *InlineCache::Clone(Js::PropertyId propertyId, ScriptContext* scriptContext)
    {
        Assert(scriptContext);

        InlineCacheAllocator* allocator = scriptContext->GetInlineCacheAllocator();
        // Important to zero the allocated cache to be sure CopyTo doesn't see garbage
        // when it uses the next pointer.
        InlineCache* clone = AllocatorNewZ(InlineCacheAllocator, allocator, InlineCache);
        CopyTo(propertyId, scriptContext, clone);
        return clone;
    }

#if ENABLE_FIXED_FIELDS
    bool InlineCache::TryGetFixedMethodFromCache(Js::FunctionBody* functionBody, uint cacheId, Js::JavascriptFunction** pFixedMethod)
    {
        Assert(pFixedMethod);

        if (IsEmpty())
        {
            return false;
        }
        Js::Type * propertyOwnerType = nullptr;
        bool isLocal = IsLocal();
        bool isProto = IsProto();
        if (isLocal)
        {
            propertyOwnerType = TypeWithoutAuxSlotTag(this->u.local.type);
        }
        else if (isProto)
        {
            // TODO (InlineCacheCleanup): For loads from proto, we could at least grab the value from protoObject's slot
            // (given by the cache) and see if its a function. Only then, does it make sense to check with the type handler.
            propertyOwnerType = this->u.proto.prototypeObject->GetType();
        }
        else
        {
            propertyOwnerType = this->u.accessor.object->GetType();
        }

        Assert(propertyOwnerType != nullptr);

        if (Js::DynamicType::Is(propertyOwnerType->GetTypeId()))
        {
            Js::DynamicTypeHandler* propertyOwnerTypeHandler = ((Js::DynamicType*)propertyOwnerType)->GetTypeHandler();
            Js::PropertyId propertyId = functionBody->GetPropertyIdFromCacheId(cacheId);
            Js::PropertyRecord const * const methodPropertyRecord = functionBody->GetScriptContext()->GetPropertyName(propertyId);

            Var fixedMethod = nullptr;
            bool isUseFixedProperty;
            if (isLocal || isProto)
            {
                isUseFixedProperty = propertyOwnerTypeHandler->TryUseFixedProperty(methodPropertyRecord, &fixedMethod, Js::FixedPropertyKind::FixedMethodProperty, functionBody->GetScriptContext());
            }
            else
            {
                isUseFixedProperty = propertyOwnerTypeHandler->TryUseFixedAccessor(methodPropertyRecord, &fixedMethod, Js::FixedPropertyKind::FixedAccessorProperty, this->IsGetterAccessor(), functionBody->GetScriptContext());
            }
            AssertMsg(fixedMethod == nullptr || Js::JavascriptFunction::Is(fixedMethod), "The fixed value should have been a Method !!!");
            *pFixedMethod = reinterpret_cast<JavascriptFunction*>(fixedMethod);
            return isUseFixedProperty;
        }

        return false;
    }
#endif

    void InlineCache::CopyTo(PropertyId propertyId, ScriptContext * scriptContext, InlineCache * const clone)
    {
        DebugOnly(VerifyRegistrationForInvalidation(this, scriptContext, propertyId));
        DebugOnly(VerifyRegistrationForInvalidation(clone, scriptContext, propertyId));
        Assert(clone != nullptr);

        // Note, the Register methods can throw due to OOM, so we need to do it before the inline cache is copied below.
        if (this->invalidationListSlotPtr != nullptr && clone->invalidationListSlotPtr == nullptr)
        {
            if (this->NeedsToBeRegisteredForProtoInvalidation())
            {
                scriptContext->RegisterProtoInlineCache(clone, propertyId);
            }
            else if (this->NeedsToBeRegisteredForStoreFieldInvalidation())
            {
                scriptContext->RegisterStoreFieldInlineCache(clone, propertyId);
            }
        }

#if DBG
        // inline cache pages might been locked after ZeroAll
        if (memcmp(&clone->u, &this->u, sizeof(this->u)) != 0)
#endif
        {
            clone->u = this->u;
        }

        DebugOnly(VerifyRegistrationForInvalidation(clone, scriptContext, propertyId));
    }

    template <bool isAccessor>
    bool InlineCache::HasDifferentType(const bool isProto, const Type * type, const Type * typeWithoutProperty) const
    {
        Assert(!isAccessor && !isProto || !typeWithoutProperty);

        if (isAccessor)
        {
            return !IsEmpty() && u.accessor.type != type && u.accessor.type != TypeWithAuxSlotTag(type);
        }
        if (isProto)
        {
            return !IsEmpty() && u.proto.type != type && u.proto.type != TypeWithAuxSlotTag(type);
        }

        // If the new type matches the cached type, the types without property must also match (unless one of them is null).
        Assert((u.local.typeWithoutProperty == nullptr || typeWithoutProperty == nullptr) ||
            ((u.local.type != type || u.local.typeWithoutProperty == typeWithoutProperty) &&
                (u.local.type != TypeWithAuxSlotTag(type) || u.local.typeWithoutProperty == TypeWithAuxSlotTag(typeWithoutProperty))));

        // Don't consider a cache polymorphic, if it differs only by the typeWithoutProperty.  We can handle this case with
        // the monomorphic cache.
        return !IsEmpty() && (u.local.type != type && u.local.type != TypeWithAuxSlotTag(type));
    }

    // explicit instantiation
    template bool InlineCache::HasDifferentType<true>(const bool isProto, const Type * type, const Type * typeWithoutProperty) const;
    template bool InlineCache::HasDifferentType<false>(const bool isProto, const Type * type, const Type * typeWithoutProperty) const;

    bool InlineCache::NeedsToBeRegisteredForProtoInvalidation() const
    {
        return (IsProto() || IsGetterAccessorOnProto());
    }

    bool InlineCache::NeedsToBeRegisteredForStoreFieldInvalidation() const
    {
        return (IsLocal() && this->u.local.typeWithoutProperty != nullptr) || IsSetterAccessorOnProto();
    }

#if DEBUG
    bool InlineCache::NeedsToBeRegisteredForInvalidation() const
    {
        int howManyInvalidationsNeeded =
            (int)NeedsToBeRegisteredForProtoInvalidation() +
            (int)NeedsToBeRegisteredForStoreFieldInvalidation();
        Assert(howManyInvalidationsNeeded <= 1);
        return howManyInvalidationsNeeded > 0;
    }

    void InlineCache::VerifyRegistrationForInvalidation(const InlineCache* cache, ScriptContext* scriptContext, Js::PropertyId propertyId)
    {
        bool needsProtoInvalidation = cache->NeedsToBeRegisteredForProtoInvalidation();
        bool needsStoreFieldInvalidation = cache->NeedsToBeRegisteredForStoreFieldInvalidation();
        int howManyInvalidationsNeeded = (int)needsProtoInvalidation + (int)needsStoreFieldInvalidation;
        bool hasListSlotPtr = cache->invalidationListSlotPtr != nullptr;
        bool isProtoRegistered = hasListSlotPtr ? scriptContext->GetThreadContext()->IsProtoInlineCacheRegistered(cache, propertyId) : false;
        bool isStoreFieldRegistered = hasListSlotPtr ? scriptContext->GetThreadContext()->IsStoreFieldInlineCacheRegistered(cache, propertyId) : false;
        int howManyRegistrations = (int)isProtoRegistered + (int)isStoreFieldRegistered;

        Assert(howManyInvalidationsNeeded <= 1);
        Assert((howManyInvalidationsNeeded == 0) || hasListSlotPtr);
        Assert(!needsProtoInvalidation || isProtoRegistered);
        Assert(!needsStoreFieldInvalidation || isStoreFieldRegistered);
        Assert(!hasListSlotPtr || howManyRegistrations > 0);
        Assert(!hasListSlotPtr || (*cache->invalidationListSlotPtr) == cache);
    }

    // Confirm inline cache miss against instance property lookup info.
    bool InlineCache::ConfirmCacheMiss(const Type * oldType, const PropertyValueInfo* info) const
    {
        return u.local.type != oldType
            && u.proto.type != oldType
            && (u.accessor.type != oldType || info == NULL || u.accessor.flags != info->GetFlags());
    }
#endif

#if DBG_DUMP
    void InlineCache::Dump()
    {
        if (this->u.local.isLocal)
        {
            Output::Print(_u("LOCAL { types: 0x%X -> 0x%X, slot = %d, list slot ptr = 0x%X }"),
                this->u.local.typeWithoutProperty,
                this->u.local.type,
                this->u.local.slotIndex,
                this->invalidationListSlotPtr
                );
        }
        else if (this->u.proto.isProto)
        {
            Output::Print(_u("PROTO { type = 0x%X, prototype = 0x%X, slot = %d, list slot ptr = 0x%X }"),
                this->u.proto.type,
                this->u.proto.prototypeObject,
                this->u.proto.slotIndex,
                this->invalidationListSlotPtr
                );
        }
        else if (this->u.accessor.isAccessor)
        {
            Output::Print(_u("FLAGS { type = 0x%X, object = 0x%X, flag = 0x%X, slot = %d, list slot ptr = 0x%X }"),
                this->u.accessor.type,
                this->u.accessor.object,
                this->u.accessor.slotIndex,
                this->u.accessor.flags,
                this->invalidationListSlotPtr
                );
        }
        else
        {
            Assert(this->u.accessor.type == 0);
            Assert(this->u.accessor.slotIndex == 0);
            Output::Print(_u("uninitialized"));
        }
    }

#endif

    template<bool isAccessor>
    bool PolymorphicInlineCache::HasDifferentType(
        const bool isProto,
        const Type * type,
        const Type * typeWithoutProperty) const
    {
        Assert(!isAccessor && !isProto || !typeWithoutProperty);

        uint inlineCacheIndex = GetInlineCacheIndexForType(type);
        if (inlineCaches[inlineCacheIndex].HasDifferentType<isAccessor>(isProto, type, typeWithoutProperty))
        {
            return true;
        }

        if (!isAccessor && !isProto && typeWithoutProperty)
        {
            inlineCacheIndex = GetInlineCacheIndexForType(typeWithoutProperty);
            return inlineCaches[inlineCacheIndex].HasDifferentType<isAccessor>(isProto, type, typeWithoutProperty);
        }

        return false;
    }

    // explicit instantiation
    template bool PolymorphicInlineCache::HasDifferentType<true>(const bool isProto, const Type * type, const Type * typeWithoutProperty) const;
    template bool PolymorphicInlineCache::HasDifferentType<false>(const bool isProto, const Type * type, const Type * typeWithoutProperty) const;

    bool PolymorphicInlineCache::HasType_Flags(const Type * type) const
    {
        uint inlineCacheIndex = GetInlineCacheIndexForType(type);
        return inlineCaches[inlineCacheIndex].HasType_Flags(type);
    }

    void PolymorphicInlineCache::UpdateInlineCachesFillInfo(uint index, bool set)
    {
        Assert(index < 0x20);
        if (set)
        {
            this->inlineCachesFillInfo |= 1 << index;
        }
        else
        {
            this->inlineCachesFillInfo &= ~(1 << index);
        }
    }

    bool PolymorphicInlineCache::IsFull()
    {
        Assert(this->size <= 0x20);
        return this->inlineCachesFillInfo == ((1 << (this->size - 1)) << 1) - 1;
    }

    void PolymorphicInlineCache::CacheLocal(
        Type *const type,
        const PropertyId propertyId,
        const PropertyIndex propertyIndex,
        const bool isInlineSlot,
        Type *const typeWithoutProperty,
        int requiredAuxSlotCapacity,
        ScriptContext *const requestContext)
    {
        // Let's not waste polymorphic cache slots by caching both the type without property and type with property. If the
        // cache is used for both adding a property and setting the existing property, then those instances will cause both
        // types to be cached. Until then, caching both types proactively here can unnecessarily trash useful cached info
        // because the types use different slots, unlike a monomorphic inline cache.
        if (!typeWithoutProperty)
        {
            uint inlineCacheIndex = GetInlineCacheIndexForType(type);
#if INTRUSIVE_TESTTRACE_PolymorphicInlineCache
            bool collision = !inlineCaches[inlineCacheIndex].IsEmpty();
#endif
            if (!PHASE_OFF1(Js::CloneCacheInCollisionPhase))
            {
                if (!inlineCaches[inlineCacheIndex].IsEmpty() && !inlineCaches[inlineCacheIndex].NeedsToBeRegisteredForStoreFieldInvalidation() && GetSize() != 1)
                {
                    if (inlineCaches[inlineCacheIndex].IsLocal())
                    {
                        CloneInlineCacheToEmptySlotInCollision<true, false, false>(type, inlineCacheIndex);
                    }
                    else if (inlineCaches[inlineCacheIndex].IsProto())
                    {
                        CloneInlineCacheToEmptySlotInCollision<false, true, false>(type, inlineCacheIndex);
                    }
                    else
                    {
                        CloneInlineCacheToEmptySlotInCollision<false, false, true>(type, inlineCacheIndex);
                    }
                }
            }

            inlineCaches[inlineCacheIndex].CacheLocal(
                type, propertyId, propertyIndex, isInlineSlot, nullptr, requiredAuxSlotCapacity, requestContext);
            UpdateInlineCachesFillInfo(inlineCacheIndex, true /*set*/);

#if DBG_DUMP
            if (PHASE_VERBOSE_TRACE1(Js::PolymorphicInlineCachePhase))
            {
                Output::Print(_u("PIC::CacheLocal, %s, %d: "), requestContext->GetPropertyName(propertyId)->GetBuffer(), inlineCacheIndex);
                inlineCaches[inlineCacheIndex].Dump();
                Output::Print(_u("\n"));
                Output::Flush();
            }
#endif
            PHASE_PRINT_INTRUSIVE_TESTTRACE1(
                Js::PolymorphicInlineCachePhase,
                _u("TestTrace PIC: CacheLocal, 0x%x, entryIndex = %d, collision = %s, entries = %d\n"), this, inlineCacheIndex, collision ? _u("true") : _u("false"), GetEntryCount());
        }
        else
        {
            uint inlineCacheIndex = GetInlineCacheIndexForType(typeWithoutProperty);
#if INTRUSIVE_TESTTRACE_PolymorphicInlineCache
            bool collision = !inlineCaches[inlineCacheIndex].IsEmpty();
#endif
            inlineCaches[inlineCacheIndex].CacheLocal(
                type, propertyId, propertyIndex, isInlineSlot, typeWithoutProperty, requiredAuxSlotCapacity, requestContext);
            UpdateInlineCachesFillInfo(inlineCacheIndex, true /*set*/);

#if DBG_DUMP
            if (PHASE_VERBOSE_TRACE1(Js::PolymorphicInlineCachePhase))
            {
                Output::Print(_u("PIC::CacheLocal, %s, %d: "), requestContext->GetPropertyName(propertyId)->GetBuffer(), inlineCacheIndex);
                inlineCaches[inlineCacheIndex].Dump();
                Output::Print(_u("\n"));
                Output::Flush();
            }
#endif
            PHASE_PRINT_INTRUSIVE_TESTTRACE1(
                Js::PolymorphicInlineCachePhase,
                _u("TestTrace PIC: CacheLocal, 0x%x, entryIndex = %d, collision = %s, entries = %d\n"), this, inlineCacheIndex, collision ? _u("true") : _u("false"), GetEntryCount());
        }
    }

    void PolymorphicInlineCache::CacheProto(
        DynamicObject *const prototypeObjectWithProperty,
        const PropertyId propertyId,
        const PropertyIndex propertyIndex,
        const bool isInlineSlot,
        const bool isMissing,
        Type *const type,
        ScriptContext *const requestContext)
    {
        uint inlineCacheIndex = GetInlineCacheIndexForType(type);
#if INTRUSIVE_TESTTRACE_PolymorphicInlineCache
        bool collision = !inlineCaches[inlineCacheIndex].IsEmpty();
#endif
        if (!PHASE_OFF1(Js::CloneCacheInCollisionPhase))
        {
            if (!inlineCaches[inlineCacheIndex].IsEmpty() && !inlineCaches[inlineCacheIndex].NeedsToBeRegisteredForStoreFieldInvalidation() && GetSize() != 1)
            {
                if (inlineCaches[inlineCacheIndex].IsLocal())
                {
                    CloneInlineCacheToEmptySlotInCollision<true, false, false>(type, inlineCacheIndex);
                }
                else if (inlineCaches[inlineCacheIndex].IsProto())
                {
                    CloneInlineCacheToEmptySlotInCollision<false, true, false>(type, inlineCacheIndex);
                }
                else
                {
                    CloneInlineCacheToEmptySlotInCollision<false, false, true>(type, inlineCacheIndex);
                }
            }
        }

        inlineCaches[inlineCacheIndex].CacheProto(
            prototypeObjectWithProperty, propertyId, propertyIndex, isInlineSlot, isMissing, type, requestContext);
        UpdateInlineCachesFillInfo(inlineCacheIndex, true /*set*/);

#if DBG_DUMP
        if (PHASE_VERBOSE_TRACE1(Js::PolymorphicInlineCachePhase))
        {
            Output::Print(_u("PIC::CacheProto, %s, %d: "), requestContext->GetPropertyName(propertyId)->GetBuffer(), inlineCacheIndex);
            inlineCaches[inlineCacheIndex].Dump();
            Output::Print(_u("\n"));
            Output::Flush();
        }
#endif
        PHASE_PRINT_INTRUSIVE_TESTTRACE1(
            Js::PolymorphicInlineCachePhase,
            _u("TestTrace PIC: CacheProto, 0x%x, entryIndex = %d, collision = %s, entries = %d\n"), this, inlineCacheIndex, collision ? _u("true") : _u("false"), GetEntryCount());
    }

    void PolymorphicInlineCache::CacheAccessor(
        const bool isGetter,
        const PropertyId propertyId,
        const PropertyIndex propertyIndex,
        const bool isInlineSlot,
        Type *const type,
        DynamicObject *const object,
        const bool isOnProto,
        ScriptContext *const requestContext)
    {
        uint inlineCacheIndex = GetInlineCacheIndexForType(type);
#if INTRUSIVE_TESTTRACE_PolymorphicInlineCache
        bool collision = !inlineCaches[inlineCacheIndex].IsEmpty();
#endif
        if (!PHASE_OFF1(Js::CloneCacheInCollisionPhase))
        {
            if (!inlineCaches[inlineCacheIndex].IsEmpty() && !inlineCaches[inlineCacheIndex].NeedsToBeRegisteredForStoreFieldInvalidation() && GetSize() != 1)
            {
                if (inlineCaches[inlineCacheIndex].IsLocal())
                {
                    CloneInlineCacheToEmptySlotInCollision<true, false, false>(type, inlineCacheIndex);
                }
                else if (inlineCaches[inlineCacheIndex].IsProto())
                {
                    CloneInlineCacheToEmptySlotInCollision<false, true, false>(type, inlineCacheIndex);
                }
                else
                {
                    CloneInlineCacheToEmptySlotInCollision<false, false, true>(type, inlineCacheIndex);
                }
            }
        }

        inlineCaches[inlineCacheIndex].CacheAccessor(isGetter, propertyId, propertyIndex, isInlineSlot, type, object, isOnProto, requestContext);
        UpdateInlineCachesFillInfo(inlineCacheIndex, true /*set*/);

#if DBG_DUMP
        if (PHASE_VERBOSE_TRACE1(Js::PolymorphicInlineCachePhase))
        {
            Output::Print(_u("PIC::CacheAccessor, %s, %d: "), requestContext->GetPropertyName(propertyId)->GetBuffer(), inlineCacheIndex);
            inlineCaches[inlineCacheIndex].Dump();
            Output::Print(_u("\n"));
            Output::Flush();
        }
#endif
        PHASE_PRINT_INTRUSIVE_TESTTRACE1(
            Js::PolymorphicInlineCachePhase,
            _u("TestTrace PIC: CacheAccessor, 0x%x, entryIndex = %d, collision = %s, entries = %d\n"), this, inlineCacheIndex, collision ? _u("true") : _u("false"), GetEntryCount());
    }

    bool PolymorphicInlineCache::PretendTryGetProperty(
        Type *const type,
        PropertyCacheOperationInfo * operationInfo)
    {
        uint inlineCacheIndex = GetInlineCacheIndexForType(type);
        return inlineCaches[inlineCacheIndex].PretendTryGetProperty(type, operationInfo);
    }

    bool PolymorphicInlineCache::PretendTrySetProperty(
        Type *const type,
        Type *const oldType,
        PropertyCacheOperationInfo * operationInfo)
    {
        uint inlineCacheIndex = GetInlineCacheIndexForType(type);
        return inlineCaches[inlineCacheIndex].PretendTrySetProperty(type, oldType, operationInfo);
    }

    void PolymorphicInlineCache::CopyTo(PropertyId propertyId, ScriptContext* scriptContext, PolymorphicInlineCache *const clone)
    {
        Assert(clone);

        clone->ignoreForEquivalentObjTypeSpec = this->ignoreForEquivalentObjTypeSpec;
        clone->cloneForJitTimeUse = this->cloneForJitTimeUse;

        for (uint i = 0; i < GetSize(); ++i)
        {
            Type * type = inlineCaches[i].GetType();
            if (type)
            {
                uint inlineCacheIndex = clone->GetInlineCacheIndexForType(type);

                // When copying inline caches from one polymorphic cache to another, types are again hashed to get the corresponding indices in the new polymorphic cache.
                // This might lead to collision in the new cache. We need to try to resolve that collision.
                if (!PHASE_OFF1(Js::CloneCacheInCollisionPhase))
                {
                    if (!clone->inlineCaches[inlineCacheIndex].IsEmpty() && !clone->inlineCaches[inlineCacheIndex].NeedsToBeRegisteredForStoreFieldInvalidation() && GetSize() != 1)
                    {
                        if (clone->inlineCaches[inlineCacheIndex].IsLocal())
                        {
                            clone->CloneInlineCacheToEmptySlotInCollision<true, false, false>(type, inlineCacheIndex);
                        }
                        else if (clone->inlineCaches[inlineCacheIndex].IsProto())
                        {
                            clone->CloneInlineCacheToEmptySlotInCollision<false, true, false>(type, inlineCacheIndex);
                        }
                        else
                        {
                            clone->CloneInlineCacheToEmptySlotInCollision<false, false, true>(type, inlineCacheIndex);
                        }

                    }
                }
                inlineCaches[i].CopyTo(propertyId, scriptContext, &clone->inlineCaches[inlineCacheIndex]);
                clone->UpdateInlineCachesFillInfo(inlineCacheIndex, true /*set*/);
            }
        }
    }

#if DBG_DUMP
    void PolymorphicInlineCache::Dump()
    {
        for (uint i = 0; i < size; ++i)
        {
            if (!inlineCaches[i].IsEmpty())
            {
                Output::Print(_u("  %d: "), i);
                inlineCaches[i].Dump();
                Output::Print(_u("\n"));
            }
        }
    }
#endif

    FunctionBodyPolymorphicInlineCache * FunctionBodyPolymorphicInlineCache::New(uint16 size, FunctionBody * functionBody)
    {
        ScriptContext * scriptContext = functionBody->GetScriptContext();
        InlineCache * inlineCaches = AllocatorNewArrayZ(InlineCacheAllocator, scriptContext->GetInlineCacheAllocator(), InlineCache, size);
#ifdef POLY_INLINE_CACHE_SIZE_STATS
        scriptContext->GetInlineCacheAllocator()->LogPolyCacheAlloc(size * sizeof(InlineCache));
#endif
        FunctionBodyPolymorphicInlineCache * polymorphicInlineCache = RecyclerNewFinalizedLeaf(scriptContext->GetRecycler(), FunctionBodyPolymorphicInlineCache, inlineCaches, size, functionBody);

        polymorphicInlineCache->prev = nullptr;
        polymorphicInlineCache->next = polymorphicInlineCache->functionBody->GetPolymorphicInlineCachesHead();
        if (polymorphicInlineCache->next)
        {
            polymorphicInlineCache->next->prev = polymorphicInlineCache;
        }
        polymorphicInlineCache->functionBody->SetPolymorphicInlineCachesHead(polymorphicInlineCache);

        return polymorphicInlineCache;
    }

    void FunctionBodyPolymorphicInlineCache::Finalize(bool isShutdown)
    {
        if (size == 0)
        {
            // Already finalized
            Assert(!inlineCaches && !prev && !next);
            return;
        }

        uint unregisteredInlineCacheCount = 0;

        Assert(inlineCaches && size > 0);

        // If we're not shutting down (as in closing the script context), we need to remove our inline caches from
        // thread context's invalidation lists, and release memory back to the arena.  During script context shutdown,
        // we leave everything in place, because the inline cache arena will stay alive until script context is destroyed
        // (as in destructor has been called) and thus the invalidation lists are safe to keep references to caches from this
        // script context.  We will, however, zero all inline caches so that we don't have to process them on subsequent
        // collections, which may still happen from other script contexts.
        if (isShutdown)
        {
#if DBG
            for (int i = 0; i < size; i++)
            {
                inlineCaches[i].Clear();
            }
#else
            memset(inlineCaches, 0, size * sizeof(InlineCache));
#endif
        }
        else
        {
            for (int i = 0; i < size; i++)
            {
                if (inlineCaches[i].RemoveFromInvalidationList())
                {
                    unregisteredInlineCacheCount++;
                }
            }

            AllocatorDeleteArray(InlineCacheAllocator, this->functionBody->GetScriptContext()->GetInlineCacheAllocator(), size, inlineCaches);
#ifdef POLY_INLINE_CACHE_SIZE_STATS
            this->functionBody->GetScriptContext()->GetInlineCacheAllocator()->LogPolyCacheFree(size * sizeof(InlineCache));
#endif
        }

        // Remove this PolymorphicInlineCache from the list
        if (this == this->functionBody->GetPolymorphicInlineCachesHead())
        {
            Assert(!prev);
            if (next)
            {
                Assert(next->prev == this);
                next->prev = nullptr;
            }
            this->functionBody->SetPolymorphicInlineCachesHead(next);
        }
        else
        {
            if (prev)
            {
                Assert(prev->next == this);
                prev->next = next;
            }
            if (next)
            {
                Assert(next->prev == this);
                next->prev = prev;
            }
        }
        prev = next = nullptr;
        inlineCaches = nullptr;
        size = 0;
        if (unregisteredInlineCacheCount > 0)
        {
            this->functionBody->GetScriptContext()->GetThreadContext()->NotifyInlineCacheBatchUnregistered(unregisteredInlineCacheCount);
        }
    }

    ScriptContextPolymorphicInlineCache * ScriptContextPolymorphicInlineCache::New(uint16 size, JavascriptLibrary* javascriptLibrary)
    {
        ScriptContext * scriptContext = javascriptLibrary->GetScriptContext();
        InlineCache * inlineCaches = AllocatorNewArrayZ(InlineCacheAllocator, scriptContext->GetInlineCacheAllocator(), InlineCache, size);
#ifdef POLY_INLINE_CACHE_SIZE_STATS
        scriptContext->GetInlineCacheAllocator()->LogPolyCacheAlloc(size * sizeof(InlineCache));
#endif
        ScriptContextPolymorphicInlineCache * polymorphicInlineCache = RecyclerNewFinalized(scriptContext->GetRecycler(), ScriptContextPolymorphicInlineCache, inlineCaches, size, javascriptLibrary);

        return polymorphicInlineCache;
    }

    void ScriptContextPolymorphicInlineCache::Finalize(bool isShutdown)
    {
        if (size == 0)
        {
            // Already finalized
            Assert(!inlineCaches);
            return;
        }

        uint unregisteredInlineCacheCount = 0;

        Assert(inlineCaches && size > 0);

        // If we're not shutting down (as in closing the script context), we need to remove our inline caches from
        // thread context's invalidation lists, and release memory back to the arena.  During script context shutdown,
        // we leave everything in place, because the inline cache arena will stay alive until script context is destroyed
        // (as in destructor has been called) and thus the invalidation lists are safe to keep references to caches from this
        // script context.  We will, however, zero all inline caches so that we don't have to process them on subsequent
        // collections, which may still happen from other script contexts.
        if (isShutdown)
        {
#if DBG
            for (int i = 0; i < size; i++)
            {
                inlineCaches[i].Clear();
            }
#else
            memset(inlineCaches, 0, size * sizeof(InlineCache));
#endif
        }
        else
        {
            for (int i = 0; i < size; i++)
            {
                if (inlineCaches[i].RemoveFromInvalidationList())
                {
                    unregisteredInlineCacheCount++;
                }
            }
            AllocatorDeleteArray(InlineCacheAllocator, this->javascriptLibrary->scriptContext->GetInlineCacheAllocator(), size, inlineCaches);
#ifdef POLY_INLINE_CACHE_SIZE_STATS
            this->javascriptLibrary->scriptContext->GetInlineCacheAllocator()->LogPolyCacheFree(size * sizeof(InlineCache));
#endif
        }

        inlineCaches = nullptr;
        size = 0;
        if (unregisteredInlineCacheCount > 0)
        {
            this->javascriptLibrary->scriptContext->GetThreadContext()->NotifyInlineCacheBatchUnregistered(unregisteredInlineCacheCount);
        }
    }

#ifdef INLINE_CACHE_STATS
    void FunctionBodyPolymorphicInlineCache::PrintStats(InlineCacheData *data) const
    {
        char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
        wchar funcName[1024];
        uint total = data->hits + data->misses;
        char16 const *propName = this->functionBody->GetScriptContext()->GetThreadContext()->GetPropertyName(data->propertyId)->GetBuffer();
        swprintf_s(funcName, _u("%s (%s)"), this->functionBody->GetExternalDisplayName(), this->functionBody->GetDebugNumberSet(debugStringBuffer));

        Output::Print(_u("%s,%s,%s,%d,%d,%f,%d,%f,%d\n"),
            funcName,
            propName,
            data->isGetCache ? _u("get") : _u("set"),
            total,
            data->misses,
            static_cast<float>(data->misses) / total,
            data->collisions,
            static_cast<float>(data->collisions) / total,
            GetSize()
        );
    }
    ScriptContext* FunctionBodyPolymorphicInlineCache::GetScriptContext() const
    {
        return this->functionBody->GetScriptContext();
    }

    void ScriptContextPolymorphicInlineCache::PrintStats(InlineCacheData *data) const
    {
        uint total = data->hits + data->misses;

        Output::Print(_u("ScriptContext,%s,%s,%d,%d,%f,%d,%f,%d\n"),
            data->isGetCache ? _u("get") : _u("set"),
            total,
            data->misses,
            static_cast<float>(data->misses) / total,
            data->collisions,
            static_cast<float>(data->collisions) / total,
            GetSize()
        );
    }

    ScriptContext* ScriptContextPolymorphicInlineCache::GetScriptContext() const
    {
        return this->javascriptLibrary->scriptContext;
    }
#endif

    void IsInstInlineCache::Set(Type * instanceType, JavascriptFunction * function, JavascriptBoolean * result)
    {
        this->type = instanceType;
        this->function = function;
        this->result = result;
    }

    void IsInstInlineCache::Clear()
    {
#if DBG
        if (!IsAll((byte*)this, sizeof(IsInstInlineCache), 0))
#endif
        {
            memset(this, 0, sizeof(IsInstInlineCache));
        }
    }

    void IsInstInlineCache::Unregister(ScriptContext * scriptContext)
    {
        scriptContext->GetThreadContext()->UnregisterIsInstInlineCache(this, this->function);
    }

    bool IsInstInlineCache::TryGetResult(Var instance, JavascriptFunction * function, JavascriptBoolean ** result)
    {
        // In order to get the result from the cache we must have a function instance.
        Assert(function != NULL);

        if (this->function == function &&
            this->type == RecyclableObject::FromVar(instance)->GetType())
        {
            if (result != nullptr)
            {
                (*result = this->result);
            }
            return true;
        }
        else
        {
            return false;
        }
    }

    void IsInstInlineCache::Cache(Type * instanceType, JavascriptFunction * function, JavascriptBoolean *  result, ScriptContext * scriptContext)
    {
        // In order to populate the cache we must have a function instance.
        Assert(function != nullptr);

        // We assume the following invariant: (cache->function != nullptr) => script context is registered as having some populated instance-of inline caches and
        // this cache is registered with thread context for invalidation.
        if (this->function == function)
        {
            Assert(scriptContext->IsIsInstInlineCacheRegistered(this, function));
            this->Set(instanceType, function, result);
        }
        else
        {
            if (this->function != nullptr)
            {
                Unregister(scriptContext);
                Clear();
            }

            // If the cache's function is not null, the cache must have been registered already.  No need to register again.
            // In fact, ThreadContext::RegisterIsInstInlineCache, would assert if we tried to re-register the same cache (to enforce the invariant above).
            // Review (jedmiad): What happens if we run out of memory inside RegisterIsInstInlieCache?
            scriptContext->RegisterIsInstInlineCache(this, function);
            this->Set(instanceType, function, result);
        }
    }
    
    /* static */
    uint32 IsInstInlineCache::OffsetOfFunction()
    {
        return offsetof(IsInstInlineCache, function);
    }

    /* static */
    uint32 IsInstInlineCache::OffsetOfType()
    {
        return offsetof(IsInstInlineCache, type);
    }

    /* static */
    uint32 IsInstInlineCache::OffsetOfResult()
    {
        return offsetof(IsInstInlineCache, result);
    }
}

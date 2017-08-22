//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

CompileAssert(sizeof(ObjTypeSpecFldIDL) == sizeof(ObjTypeSpecFldInfo));

bool
ObjTypeSpecFldInfo::UsesAuxSlot() const
{
    return GetFlags().usesAuxSlot;
}

bool
ObjTypeSpecFldInfo::UsesAccessor() const
{
    return GetFlags().usesAccessor;
}

bool
ObjTypeSpecFldInfo::IsRootObjectNonConfigurableFieldLoad() const
{
    return GetFlags().isRootObjectNonConfigurableFieldLoad;
}

bool
ObjTypeSpecFldInfo::HasEquivalentTypeSet() const
{
    return m_data.typeSet != nullptr;
}

bool
ObjTypeSpecFldInfo::DoesntHaveEquivalence() const
{
    return GetFlags().doesntHaveEquivalence;
}

bool
ObjTypeSpecFldInfo::IsPoly() const
{
    return GetFlags().isPolymorphic;
}

bool
ObjTypeSpecFldInfo::IsMono() const
{
    return !IsPoly();
}

bool
ObjTypeSpecFldInfo::IsBuiltIn() const
{
    return GetFlags().isBuiltIn;
}

bool
ObjTypeSpecFldInfo::IsLoadedFromProto() const
{
    return GetFlags().isLoadedFromProto;
}

bool
ObjTypeSpecFldInfo::HasFixedValue() const
{
    return GetFlags().hasFixedValue;
}

bool
ObjTypeSpecFldInfo::IsBeingStored() const
{
    return GetFlags().isBeingStored;
}

bool
ObjTypeSpecFldInfo::IsBeingAdded() const
{
    return GetFlags().isBeingAdded;
}

bool
ObjTypeSpecFldInfo::IsRootObjectNonConfigurableField() const
{
    return GetFlags().isRootObjectNonConfigurableField;
}

bool
ObjTypeSpecFldInfo::HasInitialType() const
{
    return IsMono() && !IsLoadedFromProto() && m_data.initialType != nullptr;
}

bool
ObjTypeSpecFldInfo::IsMonoObjTypeSpecCandidate() const
{
    return IsMono();
}

bool
ObjTypeSpecFldInfo::IsPolyObjTypeSpecCandidate() const
{
    return IsPoly();
}

Js::TypeId
ObjTypeSpecFldInfo::GetTypeId() const
{
    Assert(m_data.typeId != Js::TypeIds_Limit);
    return (Js::TypeId)m_data.typeId;
}

Js::TypeId
ObjTypeSpecFldInfo::GetTypeId(uint i) const
{
    Assert(IsPoly());
    AssertOrFailFast(i < m_data.fixedFieldInfoArraySize);
    return (Js::TypeId)GetFixedFieldInfoArray()[i].GetType()->GetTypeId();
}

Js::PropertyId
ObjTypeSpecFldInfo::GetPropertyId() const
{
    return (Js::PropertyId)m_data.propertyId;
}

uint16
ObjTypeSpecFldInfo::GetSlotIndex() const
{
    return m_data.slotIndex;
}

uint16
ObjTypeSpecFldInfo::GetFixedFieldCount() const
{
    return m_data.fixedFieldCount;
}

uint
ObjTypeSpecFldInfo::GetObjTypeSpecFldId() const
{
    return m_data.id;
}

intptr_t
ObjTypeSpecFldInfo::GetProtoObject() const
{
    return (intptr_t)PointerValue(m_data.protoObjectAddr);
}

intptr_t
ObjTypeSpecFldInfo::GetFieldValue(uint i) const
{
    Assert(IsPoly());
    AssertOrFailFast(i < m_data.fixedFieldInfoArraySize);
    return GetFixedFieldInfoArray()[i].GetFieldValue();
}

intptr_t
ObjTypeSpecFldInfo::GetPropertyGuardValueAddr() const
{
    return (intptr_t)PointerValue(m_data.propertyGuardValueAddr);
}

intptr_t
ObjTypeSpecFldInfo::GetFieldValueAsFixedDataIfAvailable() const
{
    Assert(HasFixedValue() && GetFixedFieldCount() == 1);
    AssertOrFailFast(m_data.fixedFieldInfoArraySize > 0);

    return GetFixedFieldInfoArray()[0].GetFieldValue();
}

JITTimeConstructorCache *
ObjTypeSpecFldInfo::GetCtorCache() const
{
    return (JITTimeConstructorCache*)PointerValue(m_data.ctorCache);
}

Js::EquivalentTypeSet *
ObjTypeSpecFldInfo::GetEquivalentTypeSet() const
{
    return (Js::EquivalentTypeSet *)PointerValue(m_data.typeSet);
}

JITTypeHolder
ObjTypeSpecFldInfo::GetType() const
{
    Assert(IsMono());
    return GetType(0);
}

JITTypeHolder
ObjTypeSpecFldInfo::GetType(uint i) const
{
    Assert(i == 0 || IsPoly());
    AssertOrFailFast(i < m_data.fixedFieldInfoArraySize);
    JITType * type = GetFixedFieldInfoArray()[i].GetType();
    if (!type)
    {
        return nullptr;
    }
    return JITTypeHolder(GetFixedFieldInfoArray()[i].GetType());
}

JITTypeHolder
ObjTypeSpecFldInfo::GetInitialType() const
{
    return JITTypeHolder((JITType *)(PointerValue(m_data.initialType)));
}

JITTypeHolder
ObjTypeSpecFldInfo::GetFirstEquivalentType() const
{
    Assert(HasEquivalentTypeSet());
    return JITTypeHolder(GetEquivalentTypeSet()->GetFirstType());
}

void
ObjTypeSpecFldInfo::SetIsBeingStored(bool value)
{
    GetFlagsPtr()->isBeingStored = value;
}

FixedFieldInfo *
ObjTypeSpecFldInfo::GetFixedFieldIfAvailableAsFixedFunction()
{
    Assert(HasFixedValue());
    Assert(IsMono() || (IsPoly() && !DoesntHaveEquivalence()));
    AssertOrFailFast(m_data.fixedFieldInfoArraySize > 0);
    Assert(GetFixedFieldInfoArray());
    if (GetFixedFieldInfoArray()[0].GetFuncInfoAddr() != 0)
    {
        return &GetFixedFieldInfoArray()[0];
    }
    return nullptr;
}

FixedFieldInfo *
ObjTypeSpecFldInfo::GetFixedFieldIfAvailableAsFixedFunction(uint i)
{
    Assert(HasFixedValue());
    Assert(IsPoly());
    if (m_data.fixedFieldCount > 0)
    {
        AssertOrFailFast(i < m_data.fixedFieldInfoArraySize);
        if (GetFixedFieldInfoArray()[i].GetFuncInfoAddr() != 0)
        {
            return &GetFixedFieldInfoArray()[i];
        }
    }
    return nullptr;
}

FixedFieldInfo *
ObjTypeSpecFldInfo::GetFixedFieldInfoArray() const
{
    return (FixedFieldInfo*)(PointerValue(m_data.fixedFieldInfoArray));
}

ObjTypeSpecFldInfo* ObjTypeSpecFldInfo::CreateFrom(uint id, Js::InlineCache* cache, uint cacheId,
    Js::EntryPointInfo *entryPoint, Js::FunctionBody* const topFunctionBody, Js::FunctionBody *const functionBody, Js::FieldAccessStatsPtr inlineCacheStats)
{
    if (cache->IsEmpty())
    {
        return nullptr;
    }

    Js::InlineCache localCache(*cache);

    // Need to keep a reference to the types before memory allocation in case they are tagged
    Js::Type * type = nullptr;
    Js::Type * typeWithoutProperty = nullptr;
    Js::Type * propertyOwnerType;
    bool isLocal = localCache.IsLocal();
    bool isProto = localCache.IsProto();
    bool isAccessor = localCache.IsAccessor();
#if ENABLE_FIXED_FIELDS
    bool isGetter = localCache.IsGetterAccessor();
#endif
    if (isLocal)
    {
        type = TypeWithoutAuxSlotTag(localCache.u.local.type);
        if (localCache.u.local.typeWithoutProperty)
        {
            typeWithoutProperty = TypeWithoutAuxSlotTag(localCache.u.local.typeWithoutProperty);
        }
        propertyOwnerType = type;
    }
    else if (isProto)
    {
        type = TypeWithoutAuxSlotTag(localCache.u.proto.type);
        propertyOwnerType = localCache.u.proto.prototypeObject->GetType();
    }
    else
    {
        if (PHASE_OFF(Js::FixAccessorPropsPhase, functionBody))
        {
            return nullptr;
        }

        type = TypeWithoutAuxSlotTag(localCache.u.accessor.type);
        propertyOwnerType = localCache.u.accessor.object->GetType();
    }

    Js::ScriptContext* scriptContext = functionBody->GetScriptContext();
    Recycler *const recycler = scriptContext->GetRecycler();

    Js::PropertyId propertyId = functionBody->GetPropertyIdFromCacheId(cacheId);
    uint16 slotIndex = Js::Constants::NoSlot;
    bool usesAuxSlot = false;
    Js::DynamicObject* prototypeObject = nullptr;
    Js::PropertyGuard* propertyGuard = nullptr;
    JITTimeConstructorCache* ctorCache = nullptr;
    Js::Var fieldValue = nullptr;
    bool keepFieldValue = false;
    bool isFieldValueFixed = false;
    bool isMissing = false;
    bool isBuiltIn = false;

    // Save untagged type pointers, remembering whether the original type was tagged.
    // The type pointers must be untagged so that the types cannot be collected during JIT.

    if (isLocal)
    {
        slotIndex = localCache.u.local.slotIndex;
        if (type != localCache.u.local.type)
        {
            usesAuxSlot = true;
        }
        if (typeWithoutProperty)
        {
            Assert(entryPoint->GetJitTransferData() != nullptr);
            entryPoint->GetJitTransferData()->AddJitTimeTypeRef(typeWithoutProperty, recycler);

            // These shared property guards are registered on the main thread and checked during entry point installation
            // (see NativeCodeGenerator::CheckCodeGenDone) to ensure that no property became read-only while we were
            // JIT-ing on the background thread.
            propertyGuard = entryPoint->RegisterSharedPropertyGuard(propertyId, scriptContext);
        }
    }
    else if (isProto)
    {
        prototypeObject = localCache.u.proto.prototypeObject;
        slotIndex = localCache.u.proto.slotIndex;
        if (type != localCache.u.proto.type)
        {
            usesAuxSlot = true;
            fieldValue = prototypeObject->GetAuxSlot(slotIndex);
        }
        else
        {
            fieldValue = prototypeObject->GetInlineSlot(slotIndex);
        }
        isMissing = localCache.u.proto.isMissing;
        propertyGuard = entryPoint->RegisterSharedPropertyGuard(propertyId, scriptContext);
    }
    else
    {
        slotIndex = localCache.u.accessor.slotIndex;
        if (type != localCache.u.accessor.type)
        {
            usesAuxSlot = true;
            fieldValue = localCache.u.accessor.object->GetAuxSlot(slotIndex);
        }
        else
        {
            fieldValue = localCache.u.accessor.object->GetInlineSlot(slotIndex);
        }
    }

    // Keep the type alive until the entry point is installed. Note that this is longer than just during JIT, for which references
    // from the JitTimeData would have been enough.
    Assert(entryPoint->GetJitTransferData() != nullptr);
    entryPoint->GetJitTransferData()->AddJitTimeTypeRef(type, recycler);

#if ENABLE_FIXED_FIELDS
    bool allFixedPhaseOFF = PHASE_OFF(Js::FixedMethodsPhase, topFunctionBody) & PHASE_OFF(Js::UseFixedDataPropsPhase, topFunctionBody);

    if (!allFixedPhaseOFF)
    {
        Assert(propertyOwnerType != nullptr);
        if (Js::DynamicType::Is(propertyOwnerType->GetTypeId()))
        {
            Js::DynamicTypeHandler* propertyOwnerTypeHandler = ((Js::DynamicType*)propertyOwnerType)->GetTypeHandler();
            Js::PropertyRecord const * const fixedPropertyRecord = functionBody->GetScriptContext()->GetPropertyName(propertyId);
            Js::Var fixedProperty = nullptr;
            Js::JavascriptFunction* functionObject = nullptr;

            if (isLocal || isProto)
            {
                if (typeWithoutProperty == nullptr)
                {
                    // Since we don't know if this cache is used for LdMethodFld, we may fix up (use as fixed) too
                    // aggressively here, if we load a function that we don't actually call.  This happens in the case
                    // of constructors (e.g. Object.defineProperty or Object.create).
                    // TODO (InlineCacheCleanup): if we don't zero some slot(s) in the inline cache, we could store a bit there
                    // indicating if this cache is used by LdMethodFld, and only ask then. We could even store a bit in the cache
                    // indicating that the value loaded is a fixed function (or at least may still be). That last bit could work
                    // even if we zero inline caches, since we would always set it when populating the cache.
                    propertyOwnerTypeHandler->TryUseFixedProperty(fixedPropertyRecord, &fixedProperty, (Js::FixedPropertyKind)(Js::FixedPropertyKind::FixedMethodProperty | Js::FixedPropertyKind::FixedDataProperty), scriptContext);
                }
            }
            else
            {
                propertyOwnerTypeHandler->TryUseFixedAccessor(fixedPropertyRecord, &fixedProperty, (Js::FixedPropertyKind)(Js::FixedPropertyKind::FixedAccessorProperty), isGetter, scriptContext);
            }

            if (fixedProperty != nullptr && propertyGuard == nullptr)
            {
                propertyGuard = entryPoint->RegisterSharedPropertyGuard(propertyId, scriptContext);
            }

            if (fixedProperty != nullptr && Js::JavascriptFunction::Is(fixedProperty))
            {
                functionObject = (Js::JavascriptFunction *)fixedProperty;
                if (PHASE_VERBOSE_TRACE(Js::FixedMethodsPhase, functionBody))
                {
                    char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
                    char16 debugStringBuffer2[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];

                    Js::DynamicObject* protoObject = isProto ? prototypeObject : nullptr;
                    Output::Print(_u("FixedFields: function %s (%s) cloning cache with fixed method: %s (%s), function: 0x%p, body: 0x%p (cache id: %d, layout: %s, type: 0x%p, proto: 0x%p, proto type: 0x%p)\n"),
                        functionBody->GetDisplayName(), functionBody->GetDebugNumberSet(debugStringBuffer),
                        fixedPropertyRecord->GetBuffer(), functionObject->GetFunctionInfo()->GetFunctionProxy() ?
                        functionObject->GetFunctionInfo()->GetFunctionProxy()->GetDebugNumberSet(debugStringBuffer2) : _u("(null)"), functionObject, functionObject->GetFunctionInfo(),
                        cacheId, isProto ? _u("proto") : _u("local"), type, protoObject, protoObject != nullptr ? protoObject->GetType() : nullptr);
                    Output::Flush();
                }

                if (PHASE_VERBOSE_TESTTRACE(Js::FixedMethodsPhase, functionBody))
                {
                    char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
                    char16 debugStringBuffer2[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];

                    Output::Print(_u("FixedFields: function %s (%s) cloning cache with fixed method: %s (%s) (cache id: %d, layout: %s)\n"),
                        functionBody->GetDisplayName(), functionBody->GetDebugNumberSet(debugStringBuffer), fixedPropertyRecord->GetBuffer(), functionObject->GetFunctionInfo()->GetFunctionProxy() ?
                        functionObject->GetFunctionInfo()->GetFunctionProxy()->GetDebugNumberSet(debugStringBuffer2) : _u("(null)"), functionObject, functionObject->GetFunctionInfo(),
                        cacheId, isProto ? _u("proto") : _u("local"));
                    Output::Flush();
                }

                // We don't need to check for a singleton here. We checked that the singleton still existed
                // when we obtained the fixed field value inside TryUseFixedProperty. Since we don't need the
                // singleton instance later in this function, we don't care if the instance got collected.
                // We get the type handler from the propertyOwnerType, which we got from the cache. At runtime,
                // if the object is collected, it is by definition unreachable and thus the code in the function
                // we're about to emit cannot reach the object to try to access any of this object's properties.

                Js::ConstructorCache* runtimeConstructorCache = functionObject->GetConstructorCache();
                if (runtimeConstructorCache->IsSetUpForJit() && runtimeConstructorCache->GetScriptContext() == scriptContext)
                {
                    Js::FunctionInfo* functionInfo = functionObject->GetFunctionInfo();
                    Assert(functionInfo != nullptr);
                    Assert((functionInfo->GetAttributes() & Js::FunctionInfo::ErrorOnNew) == 0);

                    Assert(!runtimeConstructorCache->IsDefault());

                    if (runtimeConstructorCache->IsNormal())
                    {
                        Assert(runtimeConstructorCache->GetType()->GetIsShared());
                        // If we populated the cache with initial type before calling the constructor, but then didn't end up updating the cache
                        // after the constructor and shrinking (and locking) the inline slot capacity, we must lock it now.  In that case it is
                        // also possible that the inline slot capacity was shrunk since we originally cached it, so we must update it also.
#if DBG_DUMP
                        if (!runtimeConstructorCache->GetType()->GetTypeHandler()->GetIsInlineSlotCapacityLocked())
                        {
                            if (PHASE_TRACE(Js::FixedNewObjPhase, functionBody))
                            {
                                char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
                                char16 debugStringBuffer2[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];

                                Output::Print(_u("FixedNewObj: function %s (%s) ctor cache for %s (%s) about to be cloned has unlocked inline slot count: guard value = 0x%p, type = 0x%p, slots = %d, inline slots = %d\n"),
                                    functionBody->GetDisplayName(), functionBody->GetDebugNumberSet(debugStringBuffer), fixedPropertyRecord->GetBuffer(), functionObject->GetFunctionInfo()->GetFunctionBody() ?
                                    functionObject->GetFunctionInfo()->GetFunctionBody()->GetDebugNumberSet(debugStringBuffer2) : _u("(null)"),
                                    runtimeConstructorCache->GetRawGuardValue(), runtimeConstructorCache->GetType(),
                                    runtimeConstructorCache->GetSlotCount(), runtimeConstructorCache->GetInlineSlotCount());
                                Output::Flush();
                            }
                        }
#endif
                        runtimeConstructorCache->GetType()->GetTypeHandler()->EnsureInlineSlotCapacityIsLocked();
                        runtimeConstructorCache->UpdateInlineSlotCount();
                    }

                    // We must keep the runtime cache alive as long as this entry point exists and may try to dereference it.
                    entryPoint->RegisterConstructorCache(runtimeConstructorCache, recycler);
                    ctorCache = RecyclerNew(recycler, JITTimeConstructorCache, functionObject, runtimeConstructorCache);

                    if (PHASE_TRACE(Js::FixedNewObjPhase, functionBody))
                    {
                        char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
                        char16 debugStringBuffer2[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];

                        Output::Print(_u("FixedNewObj: function %s (%s) cloning ctor cache for %s (%s): guard value = 0x%p, type = 0x%p, slots = %d, inline slots = %d\n"),
                            functionBody->GetDisplayName(), functionBody->GetDebugNumberSet(debugStringBuffer), fixedPropertyRecord->GetBuffer(), functionObject->GetFunctionInfo()->GetFunctionBody() ?
                            functionObject->GetFunctionInfo()->GetFunctionBody()->GetDebugNumberSet(debugStringBuffer2) : _u("(null)"), functionObject, functionObject->GetFunctionInfo(),
                            runtimeConstructorCache->GetRawGuardValue(), runtimeConstructorCache->IsNormal() ? runtimeConstructorCache->GetType() : nullptr,
                            runtimeConstructorCache->GetSlotCount(), runtimeConstructorCache->GetInlineSlotCount());
                        Output::Flush();
                    }
                }
                else
                {
                    if (!runtimeConstructorCache->IsDefault())
                    {
                        if (PHASE_TRACE(Js::FixedNewObjPhase, functionBody))
                        {
                            char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
                            char16 debugStringBuffer2[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];

                            Output::Print(_u("FixedNewObj: function %s (%s) skipping ctor cache for %s (%s), because %s (guard value = 0x%p, script context = %p).\n"),
                                functionBody->GetDisplayName(), functionBody->GetDebugNumberSet(debugStringBuffer), fixedPropertyRecord->GetBuffer(), functionObject->GetFunctionInfo()->GetFunctionBody() ?
                                functionObject->GetFunctionInfo()->GetFunctionBody()->GetDebugNumberSet(debugStringBuffer2) : _u("(null)"), functionObject, functionObject->GetFunctionInfo(),
                                runtimeConstructorCache->IsEmpty() ? _u("cache is empty (or has been cleared)") :
                                runtimeConstructorCache->IsInvalidated() ? _u("cache is invalidated") :
                                runtimeConstructorCache->SkipDefaultNewObject() ? _u("default new object isn't needed") :
                                runtimeConstructorCache->NeedsTypeUpdate() ? _u("cache needs to be updated") :
                                runtimeConstructorCache->NeedsUpdateAfterCtor() ? _u("cache needs update after ctor") :
                                runtimeConstructorCache->IsPolymorphic() ? _u("cache is polymorphic") :
                                runtimeConstructorCache->GetScriptContext() != functionBody->GetScriptContext() ? _u("script context mismatch") : _u("of an unexpected situation"),
                                runtimeConstructorCache->GetRawGuardValue(), runtimeConstructorCache->GetScriptContext());
                            Output::Flush();
                        }
                    }
                }

                isBuiltIn = Js::JavascriptLibrary::GetBuiltinFunctionForPropId(propertyId) != Js::BuiltinFunction::None;
            }

            if (fixedProperty != nullptr)
            {
                Assert(fieldValue == nullptr || fieldValue == fixedProperty);
                fieldValue = fixedProperty;
                isFieldValueFixed = true;
                keepFieldValue = true;
            }
        }
    }
#endif

    FixedFieldInfo * fixedFieldInfoArray = RecyclerNewArrayZ(recycler, FixedFieldInfo, 1);

    FixedFieldInfo::PopulateFixedField(type, fieldValue, &fixedFieldInfoArray[0]);

    ObjTypeSpecFldInfo* info;

    // If we stress equivalent object type spec, let's pretend that every inline cache was polymorphic and equivalent.
    bool forcePoly = false;
    if ((!PHASE_OFF(Js::EquivObjTypeSpecByDefaultPhase, topFunctionBody) ||
        PHASE_STRESS(Js::EquivObjTypeSpecPhase, topFunctionBody))
        && !PHASE_OFF(Js::EquivObjTypeSpecPhase, topFunctionBody))
    {
        Assert(topFunctionBody->HasDynamicProfileInfo());
        auto profileData = topFunctionBody->GetAnyDynamicProfileInfo();
        // We only support equivalent fixed fields on loads from prototype, and no equivalence on missing properties
        forcePoly |= !profileData->IsEquivalentObjTypeSpecDisabled() && (!isFieldValueFixed || isProto) && !isMissing;
    }

    if (isFieldValueFixed)
    {
        // Fixed field checks allow us to assume a specific type ID, but the assumption is only
        // valid if we lock the type. Otherwise, the type ID may change out from under us without
        // evolving the type.
        if (Js::DynamicType::Is(type->GetTypeId()))
        {
            Js::DynamicType *dynamicType = static_cast<Js::DynamicType*>(type);
            if (!dynamicType->GetIsLocked())
            {
                dynamicType->LockType();
            }
        }
    }

    JITType * jitTypeWithoutProperty = nullptr;
    if (typeWithoutProperty)
    {
        jitTypeWithoutProperty = RecyclerNew(recycler, JITType);
        JITType::BuildFromJsType(typeWithoutProperty, jitTypeWithoutProperty);
    }
    if (forcePoly)
    {
        uint16 typeCount = 1;
        RecyclerJITTypeHolder* types = RecyclerNewArray(recycler, RecyclerJITTypeHolder, typeCount);
        types[0].t = RecyclerNew(recycler, JITType);
        JITType::BuildFromJsType(type, types[0].t);
        Js::EquivalentTypeSet* typeSet = RecyclerNew(recycler, Js::EquivalentTypeSet, types, typeCount);

        info = RecyclerNew(recycler, ObjTypeSpecFldInfo,
            id, type->GetTypeId(), jitTypeWithoutProperty, typeSet, usesAuxSlot, isProto, isAccessor, isFieldValueFixed, keepFieldValue, false/*doesntHaveEquivalence*/, false, slotIndex, propertyId,
            prototypeObject, propertyGuard, ctorCache, fixedFieldInfoArray, 1);

        if (PHASE_TRACE(Js::ObjTypeSpecPhase, topFunctionBody) || PHASE_TRACE(Js::EquivObjTypeSpecPhase, topFunctionBody))
        {
            const Js::PropertyRecord* propertyRecord = scriptContext->GetPropertyName(propertyId);
            Output::Print(_u("Created ObjTypeSpecFldInfo: id %u, property %s(#%u), slot %u, type set: 0x%p\n"),
                id, propertyRecord->GetBuffer(), propertyId, slotIndex, type);
            Output::Flush();
        }
    }
    else
    {
        info = RecyclerNew(recycler, ObjTypeSpecFldInfo,
            id, type->GetTypeId(), jitTypeWithoutProperty, usesAuxSlot, isProto, isAccessor, isFieldValueFixed, keepFieldValue, isBuiltIn, slotIndex, propertyId,
            prototypeObject, propertyGuard, ctorCache, fixedFieldInfoArray);

        if (PHASE_TRACE(Js::ObjTypeSpecPhase, topFunctionBody) || PHASE_TRACE(Js::EquivObjTypeSpecPhase, topFunctionBody))
        {
            const Js::PropertyRecord* propertyRecord = scriptContext->GetPropertyName(propertyId);
            Output::Print(_u("Created ObjTypeSpecFldInfo: id %u, property %s(#%u), slot %u, type: 0x%p\n"),
                id, propertyRecord->GetBuffer(), propertyId, slotIndex, type);
            Output::Flush();
        }
    }

    return info;
}

ObjTypeSpecFldInfo* ObjTypeSpecFldInfo::CreateFrom(uint id, Js::PolymorphicInlineCache* cache, uint cacheId,
    Js::EntryPointInfo *entryPoint, Js::FunctionBody* const topFunctionBody, Js::FunctionBody *const functionBody, Js::FieldAccessStatsPtr inlineCacheStats)
{

#ifdef FIELD_ACCESS_STATS
#define IncInlineCacheCount(counter) if (inlineCacheStats) { inlineCacheStats->counter++; }
#else
#define IncInlineCacheCount(counter)
#endif

    Assert(topFunctionBody->HasDynamicProfileInfo());
    auto profileData = topFunctionBody->GetAnyDynamicProfileInfo();

    bool gatherDataForInlining = cache->GetCloneForJitTimeUse() && functionBody->PolyInliningUsingFixedMethodsAllowedByConfigFlags(topFunctionBody);

    if (PHASE_OFF(Js::EquivObjTypeSpecPhase, topFunctionBody) || profileData->IsEquivalentObjTypeSpecDisabled())
    {
        if (!gatherDataForInlining)
        {
            return nullptr;
        }
    }

    Assert(cache->GetSize() < UINT16_MAX);
    Js::InlineCache* inlineCaches = cache->GetInlineCaches();
    Js::DynamicObject* prototypeObject = nullptr;
    Js::DynamicObject* accessorOwnerObject = nullptr;
    Js::TypeId typeId = Js::TypeIds_Limit;
    uint16 polyCacheSize = (uint16)cache->GetSize();
    uint16 firstNonEmptyCacheIndex = UINT16_MAX;
    uint16 slotIndex = 0;
    bool areEquivalent = true;
    bool usesAuxSlot = false;
    bool isProto = false;
    bool isAccessor = false;
    bool isGetterAccessor = false;
    bool isAccessorOnProto = false;

    bool stress = PHASE_STRESS(Js::EquivObjTypeSpecPhase, topFunctionBody);
    bool areStressEquivalent = stress;

    uint16 typeCount = 0;
    for (uint16 i = 0; (areEquivalent || stress || gatherDataForInlining) && i < polyCacheSize; i++)
    {
        Js::InlineCache& inlineCache = inlineCaches[i];
        if (inlineCache.IsEmpty()) continue;

        if (firstNonEmptyCacheIndex == UINT16_MAX)
        {
            if (inlineCache.IsLocal())
            {
                typeId = TypeWithoutAuxSlotTag(inlineCache.u.local.type)->GetTypeId();
                usesAuxSlot = TypeHasAuxSlotTag(inlineCache.u.local.type);
                slotIndex = inlineCache.u.local.slotIndex;
                // We don't support equivalent object type spec for adding properties.
                areEquivalent = inlineCache.u.local.typeWithoutProperty == nullptr;
                gatherDataForInlining = false;
            }
            // Missing properties cannot be treated as equivalent, because for objects with SDTH or DTH, we don't change the object's type
            // when we add a property.  We also don't invalidate proto inline caches (and guards) unless the property being added exists on the proto chain.
            // Missing properties by definition do not exist on the proto chain, so in the end we could have an EquivalentObjTypeSpec cache hit on a
            // property that once was missing, but has since been added. (See OS Bugs 280582).
            else if (inlineCache.IsProto() && !inlineCache.u.proto.isMissing)
            {
                isProto = true;
                typeId = TypeWithoutAuxSlotTag(inlineCache.u.proto.type)->GetTypeId();
                usesAuxSlot = TypeHasAuxSlotTag(inlineCache.u.proto.type);
                slotIndex = inlineCache.u.proto.slotIndex;
                prototypeObject = inlineCache.u.proto.prototypeObject;
            }
            else
            {
                if (!PHASE_OFF(Js::FixAccessorPropsPhase, functionBody))
                {
                    isAccessor = true;
                    isGetterAccessor = inlineCache.IsGetterAccessor();
                    isAccessorOnProto = inlineCache.u.accessor.isOnProto;
                    accessorOwnerObject = inlineCache.u.accessor.object;
                    typeId = TypeWithoutAuxSlotTag(inlineCache.u.accessor.type)->GetTypeId();
                    usesAuxSlot = TypeHasAuxSlotTag(inlineCache.u.accessor.type);
                    slotIndex = inlineCache.u.accessor.slotIndex;
                }
                else
                {
                    areEquivalent = false;
                    areStressEquivalent = false;
                }
                gatherDataForInlining = false;
            }

            // If we're stressing equivalent object type spec then let's keep trying to find a cache that we could use.
            if (!stress || areStressEquivalent)
            {
                firstNonEmptyCacheIndex = i;
            }
        }
        else
        {
            if (inlineCache.IsLocal())
            {
                if (isProto || isAccessor || inlineCache.u.local.typeWithoutProperty != nullptr || slotIndex != inlineCache.u.local.slotIndex ||
                    typeId != TypeWithoutAuxSlotTag(inlineCache.u.local.type)->GetTypeId() || usesAuxSlot != TypeHasAuxSlotTag(inlineCache.u.local.type))
                {
                    areEquivalent = false;
                }
                gatherDataForInlining = false;
            }
            else if (inlineCache.IsProto())
            {
                if (!isProto || isAccessor || prototypeObject != inlineCache.u.proto.prototypeObject || slotIndex != inlineCache.u.proto.slotIndex ||
                    typeId != TypeWithoutAuxSlotTag(inlineCache.u.proto.type)->GetTypeId() || usesAuxSlot != TypeHasAuxSlotTag(inlineCache.u.proto.type))
                {
                    areEquivalent = false;
                }
            }
            else
            {
                // Supporting equivalent obj type spec only for those polymorphic accessor property operations for which
                // 1. the property is on the same prototype, and
                // 2. the types are equivalent.
                //
                // This is done to keep the equivalence check helper as-is
                if (!isAccessor || isGetterAccessor != inlineCache.IsGetterAccessor() || !isAccessorOnProto || !inlineCache.u.accessor.isOnProto || accessorOwnerObject != inlineCache.u.accessor.object ||
                    slotIndex != inlineCache.u.accessor.slotIndex || typeId != TypeWithoutAuxSlotTag(inlineCache.u.accessor.type)->GetTypeId() || usesAuxSlot != TypeHasAuxSlotTag(inlineCache.u.accessor.type))
                {
                    areEquivalent = false;
                }
                gatherDataForInlining = false;
            }
        }
        typeCount++;
    }

    if (firstNonEmptyCacheIndex == UINT16_MAX)
    {
        IncInlineCacheCount(emptyPolyInlineCacheCount);
        return nullptr;
    }

    if (cache->GetIgnoreForEquivalentObjTypeSpec())
    {
        areEquivalent = areStressEquivalent = false;
    }

#if ENABLE_FIXED_FIELDS
    gatherDataForInlining = gatherDataForInlining && (typeCount <= 4); // Only support 4-way (max) polymorphic inlining
#else
    gatherDataForInlining = false;
#endif

    if (!areEquivalent && !areStressEquivalent)
    {
        IncInlineCacheCount(nonEquivPolyInlineCacheCount);
        cache->SetIgnoreForEquivalentObjTypeSpec(true);
        if (!gatherDataForInlining)
        {
            return nullptr;
        }
    }

    Assert(firstNonEmptyCacheIndex < polyCacheSize);
    Assert(typeId != Js::TypeIds_Limit);
    IncInlineCacheCount(equivPolyInlineCacheCount);

    // If we're stressing equivalent object type spec and the types are not equivalent, let's grab the first one only.
    if (stress && (areEquivalent != areStressEquivalent))
    {
        polyCacheSize = firstNonEmptyCacheIndex + 1;
    }

    Js::ScriptContext* scriptContext = functionBody->GetScriptContext();
    Recycler* recycler = scriptContext->GetRecycler();

    uint16 fixedFunctionCount = 0;

    // Need to create a local array here and not allocate one from the recycler,
    // as the allocation may trigger a GC which can clear the inline caches.
    FixedFieldInfo localFixedFieldInfoArray[Js::DynamicProfileInfo::maxPolymorphicInliningSize] = {};

#if ENABLE_FIXED_FIELDS
    // For polymorphic field loads we only support fixed functions on prototypes. This helps keep the equivalence check helper simple.
    // Since all types in the polymorphic cache share the same prototype, it's enough to grab the fixed function from the prototype object.
    Js::Var fixedProperty = nullptr;
    if ((isProto || isAccessorOnProto) && (areEquivalent || areStressEquivalent))
    {
        const Js::PropertyRecord* propertyRecord = scriptContext->GetPropertyName(functionBody->GetPropertyIdFromCacheId(cacheId));
        if (isProto)
        {
            prototypeObject->GetDynamicType()->GetTypeHandler()->TryUseFixedProperty(propertyRecord, &fixedProperty, (Js::FixedPropertyKind)(Js::FixedPropertyKind::FixedMethodProperty | Js::FixedPropertyKind::FixedDataProperty), scriptContext);
        }
        else if (isAccessorOnProto)
        {
            accessorOwnerObject->GetDynamicType()->GetTypeHandler()->TryUseFixedAccessor(propertyRecord, &fixedProperty, Js::FixedPropertyKind::FixedAccessorProperty, isGetterAccessor, scriptContext);
        }

        FixedFieldInfo::PopulateFixedField(nullptr, fixedProperty, &localFixedFieldInfoArray[0]);

        // TODO (ObjTypeSpec): Enable constructor caches on equivalent polymorphic field loads with fixed functions.
    }
#endif
    // Let's get the types.
    Js::Type* localTypes[MaxPolymorphicInlineCacheSize];

    uint16 typeNumber = 0;
    for (uint16 i = firstNonEmptyCacheIndex; i < polyCacheSize; i++)
    {
        Js::InlineCache& inlineCache = inlineCaches[i];
        if (inlineCache.IsEmpty()) continue;

        localTypes[typeNumber] = inlineCache.IsLocal() ? TypeWithoutAuxSlotTag(inlineCache.u.local.type) :
            inlineCache.IsProto() ? TypeWithoutAuxSlotTag(inlineCache.u.proto.type) :
            TypeWithoutAuxSlotTag(inlineCache.u.accessor.type);

#if ENABLE_FIXED_FIELDS
        if (gatherDataForInlining)
        {
            Js::JavascriptFunction* fixedFunctionObject = nullptr;
            inlineCache.TryGetFixedMethodFromCache(functionBody, cacheId, &fixedFunctionObject);
            if (!fixedFunctionObject || !fixedFunctionObject->GetFunctionInfo()->HasBody())
            {
                if (!(areEquivalent || areStressEquivalent))
                {
                    // If we reach here only because we are gathering data for inlining, and one of the Inline Caches doesn't have a fixedfunction object, return.
                    return nullptr;
                }
                else
                {
                    // If one of the inline caches doesn't have a fixed function object, abort gathering inlining data.
                    gatherDataForInlining = false;
                    typeNumber++;
                    continue;
                }
            }

            // We got a fixed function object from the cache.

            FixedFieldInfo::PopulateFixedField(localTypes[typeNumber], fixedFunctionObject, &localFixedFieldInfoArray[typeNumber]);

            fixedFunctionCount++;
        }
#endif
        typeNumber++;
    }

    if (isAccessor && gatherDataForInlining)
    {
        Assert(fixedFunctionCount <= 1);
    }

    if (stress && (areEquivalent != areStressEquivalent))
    {
        typeCount = 1;
    }

    AnalysisAssert(typeNumber == typeCount);

    // Now that we've copied all material info into local variables, we can start allocating without fear
    // that a garbage collection will clear any of the live inline caches.

    FixedFieldInfo* fixedFieldInfoArray;
    if (gatherDataForInlining)
    {
        fixedFieldInfoArray = RecyclerNewArrayZ(recycler, FixedFieldInfo, fixedFunctionCount);
        CopyArray<FixedFieldInfo, Field(Js::Var)>(
            fixedFieldInfoArray, fixedFunctionCount, localFixedFieldInfoArray, fixedFunctionCount);
    }
    else
    {
        fixedFieldInfoArray = RecyclerNewArrayZ(recycler, FixedFieldInfo, 1);
        CopyArray<FixedFieldInfo, Field(Js::Var)>(fixedFieldInfoArray, 1, localFixedFieldInfoArray, 1);
    }

    Js::PropertyId propertyId = functionBody->GetPropertyIdFromCacheId(cacheId);
    Js::PropertyGuard* propertyGuard = entryPoint->RegisterSharedPropertyGuard(propertyId, scriptContext);

    // For polymorphic, non-equivalent objTypeSpecFldInfo's, hasFixedValue is true only if each of the inline caches has a fixed function for the given cacheId, or
    // in the case of an accessor cache, only if the there is only one version of the accessor.
    bool hasFixedValue = gatherDataForInlining ||
        ((isProto || isAccessorOnProto) && (areEquivalent || areStressEquivalent) && localFixedFieldInfoArray[0].GetFieldValue());

    bool doesntHaveEquivalence = !(areEquivalent || areStressEquivalent);

    Js::EquivalentTypeSet* typeSet = nullptr;
    auto jitTransferData = entryPoint->GetJitTransferData();
    Assert(jitTransferData != nullptr);
    if (areEquivalent || areStressEquivalent)
    {
        RecyclerJITTypeHolder* types = RecyclerNewArray(recycler, RecyclerJITTypeHolder, typeCount);
        for (uint16 i = 0; i < typeCount; i++)
        {
            jitTransferData->AddJitTimeTypeRef(localTypes[i], recycler);
            if (hasFixedValue)
            {
                // Fixed field checks allow us to assume a specific type ID, but the assumption is only
                // valid if we lock the type. Otherwise, the type ID may change out from under us without
                // evolving the type.
                if (Js::DynamicType::Is(localTypes[i]->GetTypeId()))
                {
                    Js::DynamicType *dynamicType = static_cast<Js::DynamicType*>(localTypes[i]);
                    if (!dynamicType->GetIsLocked())
                    {
                        dynamicType->LockType();
                    }
                }
            }
            // TODO: OOP JIT, consider putting these inline
            types[i].t = RecyclerNew(recycler, JITType);
            __analysis_assume(localTypes[i] != nullptr);
            JITType::BuildFromJsType(localTypes[i], types[i].t);
        }
        typeSet = RecyclerNew(recycler, Js::EquivalentTypeSet, types, typeCount);
    }

    ObjTypeSpecFldInfo* info = RecyclerNew(recycler, ObjTypeSpecFldInfo,
        id, typeId, nullptr, typeSet, usesAuxSlot, isProto, isAccessor, hasFixedValue, hasFixedValue, doesntHaveEquivalence, true, slotIndex, propertyId,
        prototypeObject, propertyGuard, nullptr, fixedFieldInfoArray, fixedFunctionCount/*, nullptr, nullptr, nullptr*/);

    if (PHASE_TRACE(Js::ObjTypeSpecPhase, topFunctionBody) || PHASE_TRACE(Js::EquivObjTypeSpecPhase, topFunctionBody))
    {
        if (PHASE_TRACE(Js::ObjTypeSpecPhase, topFunctionBody) || PHASE_TRACE(Js::EquivObjTypeSpecPhase, topFunctionBody))
        {
            if (typeSet)
            {
                const Js::PropertyRecord* propertyRecord = scriptContext->GetPropertyName(propertyId);
                Output::Print(_u("Created ObjTypeSpecFldInfo: id %u, property %s(#%u), slot %u, type set: "),
                    id, propertyRecord->GetBuffer(), propertyId, slotIndex);
                for (uint16 ti = 0; ti < typeCount - 1; ti++)
                {
                    Output::Print(_u("0x%p, "), typeSet->GetType(ti));
                }
                Output::Print(_u("0x%p\n"), typeSet->GetType(typeCount - 1));
                Output::Flush();
            }
        }
    }

    return info;

#undef IncInlineCacheCount
}

ObjTypeSpecFldInfoFlags
ObjTypeSpecFldInfo::GetFlags() const
{
    return (ObjTypeSpecFldInfoFlags)m_data.flags;
}

ObjTypeSpecFldInfoFlags *
ObjTypeSpecFldInfo::GetFlagsPtr()
{
    return (ObjTypeSpecFldInfoFlags*)&m_data.flags;
}

ObjTypeSpecFldInfoArray::ObjTypeSpecFldInfoArray()
    : infoArray(nullptr)
#if DBG
    , infoCount(0)
#endif
{
}

void ObjTypeSpecFldInfoArray::EnsureArray(Recycler *const recycler, Js::FunctionBody *const functionBody)
{
    Assert(recycler != nullptr);
    Assert(functionBody != nullptr);
    Assert(functionBody->GetInlineCacheCount() != 0);

    if (this->infoArray)
    {
        Assert(functionBody->GetInlineCacheCount() == this->infoCount);
        return;
    }

    this->infoArray = RecyclerNewArrayZ(recycler, Field(ObjTypeSpecFldInfo*), functionBody->GetInlineCacheCount());
#if DBG
    this->infoCount = functionBody->GetInlineCacheCount();
#endif
}

ObjTypeSpecFldInfo* ObjTypeSpecFldInfoArray::GetInfo(Js::FunctionBody *const functionBody, const uint index) const
{
    Assert(functionBody);
    Assert(this->infoArray == nullptr || functionBody->GetInlineCacheCount() == this->infoCount);
    Assert(index < functionBody->GetInlineCacheCount());
    return this->infoArray ? this->infoArray[index] : nullptr;
}

void ObjTypeSpecFldInfoArray::SetInfo(Recycler *const recycler, Js::FunctionBody *const functionBody,
    const uint index, ObjTypeSpecFldInfo* info)
{
    Assert(recycler);
    Assert(functionBody);
    Assert(this->infoArray == nullptr || functionBody->GetInlineCacheCount() == this->infoCount);
    Assert(index < functionBody->GetInlineCacheCount());
    Assert(info);

    EnsureArray(recycler, functionBody);
    this->infoArray[index] = info;
}

void ObjTypeSpecFldInfoArray::Reset()
{
    this->infoArray = nullptr;
#if DBG
    this->infoCount = 0;
#endif
}

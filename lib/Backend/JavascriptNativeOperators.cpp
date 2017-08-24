//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

namespace Js
{
#if ENABLE_NATIVE_CODEGEN
    void * JavascriptNativeOperators::Op_SwitchStringLookUp(JavascriptString* str, Js::BranchDictionaryWrapper<JavascriptString*>* branchTargets, uintptr_t funcStart, uintptr_t funcEnd)
    {
        void* defaultTarget = branchTargets->defaultTarget;
        Js::BranchDictionaryWrapper<JavascriptString*>::BranchDictionary& stringDictionary = branchTargets->dictionary;
        void* target = stringDictionary.Lookup(str, defaultTarget);
        uintptr_t utarget = (uintptr_t)target;

        if ((utarget - funcStart) > (funcEnd - funcStart))
        {
            AssertMsg(false, "Switch string dictionary jump target outside of function");
            Throw::FatalInternalError();
        }
        return target;
    }

#if ENABLE_DEBUG_CONFIG_OPTIONS
    void JavascriptNativeOperators::TracePropertyEquivalenceCheck(const JitEquivalentTypeGuard* guard, const Type* type, const Type* refType, bool isEquivalent, uint failedPropertyIndex)
    {
        if (PHASE_TRACE1(Js::EquivObjTypeSpecPhase))
        {
            uint propertyCount = guard->GetCache()->record.propertyCount;

            Output::Print(_u("EquivObjTypeSpec: checking %u properties on operation %u, (type = 0x%p, ref type = 0x%p):\n"),
                propertyCount, guard->GetObjTypeSpecFldId(), type, refType);

            const Js::TypeEquivalenceRecord& record = guard->GetCache()->record;
            ScriptContext* scriptContext = type->GetScriptContext();
            if (isEquivalent)
            {
                if (Js::Configuration::Global.flags.Verbose)
                {
                    Output::Print(_u("    <start>, "));
                    for (uint pi = 0; pi < propertyCount; pi++)
                    {
                        const EquivalentPropertyEntry* refInfo = &record.properties[pi];
                        const PropertyRecord* propertyRecord = scriptContext->GetPropertyName(refInfo->propertyId);
                        Output::Print(_u("%s(#%d)@%ua%dw%d, "), propertyRecord->GetBuffer(), propertyRecord->GetPropertyId(), refInfo->slotIndex, refInfo->isAuxSlot, refInfo->mustBeWritable);
                    }
                    Output::Print(_u("<end>\n"));
                }
            }
            else
            {
                const EquivalentPropertyEntry* refInfo = &record.properties[failedPropertyIndex];
                Js::PropertyEquivalenceInfo info(Constants::NoSlot, false, false);
                const PropertyRecord* propertyRecord = scriptContext->GetPropertyName(refInfo->propertyId);
                if (DynamicType::Is(type->GetTypeId()))
                {
                    Js::DynamicTypeHandler* typeHandler = (static_cast<const DynamicType*>(type))->GetTypeHandler();
                    typeHandler->GetPropertyEquivalenceInfo(propertyRecord, info);
                }

                Output::Print(_u("EquivObjTypeSpec: check failed for %s (#%d) on operation %u:\n"),
                    propertyRecord->GetBuffer(), propertyRecord->GetPropertyId(), guard->GetObjTypeSpecFldId());
                Output::Print(_u("    type = 0x%p, ref type = 0x%p, slot = 0x%u (%d), ref slot = 0x%u (%d), is writable = %d, required writable = %d\n"),
                    type, refType, info.slotIndex, refInfo->slotIndex, info.isAuxSlot, refInfo->isAuxSlot, info.isWritable, refInfo->mustBeWritable);
            }

            Output::Flush();
        }
    }
#endif

    bool JavascriptNativeOperators::IsStaticTypeObjTypeSpecEquivalent(const TypeEquivalenceRecord& equivalenceRecord, uint& failedIndex)
    {
        uint propertyCount = equivalenceRecord.propertyCount;
        Js::EquivalentPropertyEntry* properties = equivalenceRecord.properties;
        for (uint pi = 0; pi < propertyCount; pi++)
        {
            const EquivalentPropertyEntry* refInfo = &properties[pi];
            if (!IsStaticTypeObjTypeSpecEquivalent(refInfo))
            {
                failedIndex = pi;
                return false;
            }
        }
        return true;
    }

    bool JavascriptNativeOperators::IsStaticTypeObjTypeSpecEquivalent(const EquivalentPropertyEntry *entry)
    {
        // Objects of static types have no local properties, but they may load fields from their prototypes.
        return entry->slotIndex == Constants::NoSlot && !entry->mustBeWritable;
    }

    bool JavascriptNativeOperators::CheckIfTypeIsEquivalentForFixedField(Type* type, JitEquivalentTypeGuard* guard)
    {
        if (guard->GetValue() == PropertyGuard::GuardValue::Invalidated_DuringSweep)
        {
            return false;
        }
        return CheckIfTypeIsEquivalent(type, guard);
    }

    bool JavascriptNativeOperators::CheckIfTypeIsEquivalent(Type* type, JitEquivalentTypeGuard* guard)
    {
        if (guard->GetValue() == PropertyGuard::GuardValue::Invalidated)
        {
            return false;
        }

        AssertMsg(type && type->GetScriptContext(), "type and it's ScriptContext should be valid.");

        if (!guard->IsInvalidatedDuringSweep() && ((Js::Type*)guard->GetTypeAddr())->GetScriptContext() != type->GetScriptContext())
        {
            // For valid guard value, can't cache cross-context objects
            return false;
        }

        // CONSIDER : Add stats on how often the cache hits, and simply force bailout if
        // the efficacy is too low.

        EquivalentTypeCache* cache = guard->GetCache();
        // CONSIDER : Consider emitting o.type == equivTypes[hash(o.type)] in machine code before calling
        // this helper, particularly if we want to handle polymorphism with frequently changing types.
        Assert(EQUIVALENT_TYPE_CACHE_SIZE == 8);
        Type** equivTypes = cache->types;

        Type* refType = equivTypes[0];
        if (refType == nullptr || refType->GetScriptContext() != type->GetScriptContext())
        {
            // We could have guard that was invalidated while sweeping and now we have type coming from
            // different scriptContext. Make sure that it matches the scriptContext in cachedTypes.
            // If not, return false because as mentioned above, we don't cache cross-context objects.
#if DBG
            if (refType == nullptr)
            {
                for (int i = 1; i < EQUIVALENT_TYPE_CACHE_SIZE; i++)
                {
                    AssertMsg(equivTypes[i] == nullptr, "In equiv typed caches, if first element is nullptr, all others should be nullptr");
                }
            }
#endif
            return false;
        }

        if (type == equivTypes[0] || type == equivTypes[1] || type == equivTypes[2] || type == equivTypes[3] ||
            type == equivTypes[4] || type == equivTypes[5] || type == equivTypes[6] || type == equivTypes[7])
        {
#if DBG
            if (PHASE_TRACE1(Js::EquivObjTypeSpecPhase))
            {
                if (guard->WasReincarnated())
                {
                    Output::Print(_u("EquivObjTypeSpec: Guard 0x%p was reincarnated and working now \n"), guard);
                    Output::Flush();
                }
            }
#endif
            guard->SetTypeAddr((intptr_t)type);
            return true;
        }

        // If we didn't find the type in the cache, let's check if it's equivalent the slow way, by comparing
        // each of its relevant property slots to its equivalent in one of the cached types.
        // We are making a few assumption that simplify the process:
        // 1. If two types have the same prototype, any properties loaded from a prototype must come from the same slot.
        //    If any of the prototypes in the chain was altered such that this is no longer true, the corresponding
        //    property guard would have been invalidated and we would bail out at the guard check (either on this
        //    type check or downstream, but before the property load is attempted).
        // 2. For polymorphic field loads fixed fields are only supported on prototypes.  Hence, if two types have the
        //    same prototype, any of the equivalent fixed properties will match. If any has been overwritten, the
        //    corresponding guard would have been invalidated and we would bail out (as above).

        if (cache->IsLoadedFromProto() && type->GetPrototype() != refType->GetPrototype())
        {
            if (PHASE_TRACE1(Js::EquivObjTypeSpecPhase))
            {
                Output::Print(_u("EquivObjTypeSpec: failed check on operation %u (type = 0x%x, ref type = 0x%x, proto = 0x%x, ref proto = 0x%x) \n"),
                    guard->GetObjTypeSpecFldId(), type, refType, type->GetPrototype(), refType->GetPrototype());
                Output::Flush();
            }

            return false;
        }

#pragma prefast(suppress:6011) // If type is nullptr, we would AV at the beginning of this method
        if (type->GetTypeId() != refType->GetTypeId())
        {
            if (PHASE_TRACE1(Js::EquivObjTypeSpecPhase))
            {
                Output::Print(_u("EquivObjTypeSpec: failed check on operation %u (type = 0x%x, ref type = 0x%x, proto = 0x%x, ref proto = 0x%x) \n"),
                    guard->GetObjTypeSpecFldId(), type, refType, type->GetPrototype(), refType->GetPrototype());
                Output::Flush();
            }

            return false;
        }

        // Review : This is quite slow.  We could make it somewhat faster, by keeping slot indexes instead
        // of property IDs, but that would mean we would need to look up property IDs from slot indexes when installing
        // property guards, or maintain a whole separate list of equivalent slot indexes.
        Assert(cache->record.propertyCount > 0);

        // Before checking for equivalence, track existing cached non-shared types
        DynamicType * dynamicType = (type && DynamicType::Is(type->GetTypeId())) ? static_cast<DynamicType*>(type) : nullptr;
        bool isEquivTypesCacheFull = equivTypes[EQUIVALENT_TYPE_CACHE_SIZE - 1] != nullptr;
        int emptySlotIndex = -1;
        int nonSharedTypeSlotIndex = -1;
        for (int i = 0; i < EQUIVALENT_TYPE_CACHE_SIZE; i++)
        {
            // Track presence of cached non-shared type if cache is full
            if (isEquivTypesCacheFull)
            {
                if (DynamicType::Is(equivTypes[i]->GetTypeId()) &&
                    nonSharedTypeSlotIndex == -1 &&
                    !(static_cast<DynamicType*>(equivTypes[i]))->GetIsShared())
                {
                    nonSharedTypeSlotIndex = i;
                }
            }
            // Otherwise get the next available empty index
            else if (equivTypes[i] == nullptr)
            {
                emptySlotIndex = i;
                break;
            };
        }

        // If we get non-shared type while cache is full and we don't have any non-shared type to evict
        // consider this type as non-equivalent
        if (dynamicType != nullptr &&
            isEquivTypesCacheFull &&
            !dynamicType->GetIsShared() &&
            nonSharedTypeSlotIndex == -1)
        {
            return false;
        }

        // CONSIDER (EquivObjTypeSpec): Impose a limit on the number of properties guarded by an equivalent type check.
        // The trick is where in the glob opt to make the cut off. Perhaps in the forward pass we could track the number of
        // field operations protected by a type check (keep a counter on the type's value info), and if that counter exceeds
        // some threshold, simply stop optimizing any further instructions.

        bool isEquivalent;
        uint failedPropertyIndex;
        if (dynamicType != nullptr)
        {
            Js::DynamicTypeHandler* typeHandler = dynamicType->GetTypeHandler();
            isEquivalent = typeHandler->IsObjTypeSpecEquivalent(type, cache->record, failedPropertyIndex);
        }
        else
        {
            Assert(StaticType::Is(type->GetTypeId()));
            isEquivalent = IsStaticTypeObjTypeSpecEquivalent(cache->record, failedPropertyIndex);
        }

#if ENABLE_DEBUG_CONFIG_OPTIONS
        TracePropertyEquivalenceCheck(guard, type, refType, isEquivalent, failedPropertyIndex);
#endif

        if (!isEquivalent)
        {
            return false;
        }

        AssertMsg(!isEquivTypesCacheFull || !dynamicType || dynamicType->GetIsShared() || nonSharedTypeSlotIndex > -1, "If equiv cache is full, then this should be sharedType or we will evict non-shared type.");

        // If cache is full, then this is definitely a sharedType, so evict non-shared type.
        // Else evict next empty slot (only applicable for DynamicTypes)
        emptySlotIndex = (isEquivTypesCacheFull && dynamicType) ? nonSharedTypeSlotIndex : emptySlotIndex;

        // We have some empty slots, let us use those first
        if (emptySlotIndex != -1)
        {
            if (PHASE_TRACE1(Js::EquivObjTypeSpecPhase))
            {
                Output::Print(_u("EquivObjTypeSpec: Saving type in unused slot of equiv types cache. \n"));
                Output::Flush();
            }
            equivTypes[emptySlotIndex] = type;
        }
        else
        {
            // CONSIDER (EquivObjTypeSpec): Invent some form of least recently used eviction scheme.
            uintptr_t index = (reinterpret_cast<uintptr_t>(type) >> 4) & (EQUIVALENT_TYPE_CACHE_SIZE - 1);

            if (cache->nextEvictionVictim == EQUIVALENT_TYPE_CACHE_SIZE)
            {
                __analysis_assume(index < EQUIVALENT_TYPE_CACHE_SIZE);
                // If nextEvictionVictim was never set, set it to next element after index
                cache->nextEvictionVictim = (index + 1) & (EQUIVALENT_TYPE_CACHE_SIZE - 1);
            }
            else
            {
                Assert(cache->nextEvictionVictim < EQUIVALENT_TYPE_CACHE_SIZE);
                __analysis_assume(cache->nextEvictionVictim < EQUIVALENT_TYPE_CACHE_SIZE);
                equivTypes[cache->nextEvictionVictim] = equivTypes[index];
                // Else, set it to next element after current nextEvictionVictim index
                cache->nextEvictionVictim = (cache->nextEvictionVictim + 1) & (EQUIVALENT_TYPE_CACHE_SIZE - 1);
            }

            if (PHASE_TRACE1(Js::EquivObjTypeSpecPhase))
            {
                Output::Print(_u("EquivObjTypeSpec: Saving type in used slot of equiv types cache at index = %d. NextEvictionVictim = %d. \n"), index, cache->nextEvictionVictim);
                Output::Flush();
            }
            Assert(index < EQUIVALENT_TYPE_CACHE_SIZE);
            __analysis_assume(index < EQUIVALENT_TYPE_CACHE_SIZE);
            equivTypes[index] = type;
        }

        // Fixed field checks allow us to assume a specific type ID, but the assumption is only
        // valid if we lock the type. Otherwise, the type ID may change out from under us without
        // evolving the type.
        // We also need to lock the type in case of, for instance, adding a property to a dictionary type handler.
        if (dynamicType != nullptr)
        {
            if (!dynamicType->GetIsLocked())
            {
                // We only need to lock the type to prevent against the type evolving after it has been cached. If the type becomes shared
                // in the future, any further changes to the type will result in creating a new type handler.
                dynamicType->LockTypeOnly();
            }
        }

        type->SetHasBeenCached();
        guard->SetTypeAddr((intptr_t)type);
        return true;
    }
#endif

    Var JavascriptNativeOperators::OP_GetElementI_JIT_ExpectingNativeFloatArray(Var instance, Var index, ScriptContext *scriptContext)
    {
        Assert(Js::JavascriptStackWalker::ValidateTopJitFrame(scriptContext));

        JavascriptOperators::UpdateNativeArrayProfileInfoToCreateVarArray(instance, true, false);
        return JavascriptOperators::OP_GetElementI_JIT(instance, index, scriptContext);
    }

    Var JavascriptNativeOperators::OP_GetElementI_JIT_ExpectingVarArray(Var instance, Var index, ScriptContext *scriptContext)
    {
        Assert(Js::JavascriptStackWalker::ValidateTopJitFrame(scriptContext));

        JavascriptOperators::UpdateNativeArrayProfileInfoToCreateVarArray(instance, false, true);
        return JavascriptOperators::OP_GetElementI_JIT(instance, index, scriptContext);
    }

    Var JavascriptNativeOperators::OP_GetElementI_UInt32_ExpectingNativeFloatArray(Var instance, uint32 index, ScriptContext* scriptContext)
    {
#if ENABLE_PROFILE_INFO
        JavascriptOperators::UpdateNativeArrayProfileInfoToCreateVarArray(instance, true, false);
#endif
        return JavascriptOperators::OP_GetElementI_UInt32(instance, index, scriptContext);
    }

    Var JavascriptNativeOperators::OP_GetElementI_UInt32_ExpectingVarArray(Var instance, uint32 index, ScriptContext* scriptContext)
    {
#if ENABLE_PROFILE_INFO
        JavascriptOperators::UpdateNativeArrayProfileInfoToCreateVarArray(instance, false, true);
#endif
        return JavascriptOperators::OP_GetElementI_UInt32(instance, index, scriptContext);
    }

    Var JavascriptNativeOperators::OP_GetElementI_Int32_ExpectingNativeFloatArray(Var instance, int32 index, ScriptContext* scriptContext)
    {
#if ENABLE_PROFILE_INFO
        JavascriptOperators::UpdateNativeArrayProfileInfoToCreateVarArray(instance, true, false);
#endif
        return JavascriptOperators::OP_GetElementI_Int32(instance, index, scriptContext);
    }

    Var JavascriptNativeOperators::OP_GetElementI_Int32_ExpectingVarArray(Var instance, int32 index, ScriptContext* scriptContext)
    {
#if ENABLE_PROFILE_INFO
        JavascriptOperators::UpdateNativeArrayProfileInfoToCreateVarArray(instance, false, true);
#endif
        return JavascriptOperators::OP_GetElementI_Int32(instance, index, scriptContext);
    }

};

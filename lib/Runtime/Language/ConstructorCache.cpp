//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"


namespace Js
{
    ConstructorCache ConstructorCache::DefaultInstance;

    ConstructorCache::ConstructorCache()
    {
        this->content.type = nullptr;
        this->content.scriptContext = nullptr;
        this->content.slotCount = 0;
        this->content.inlineSlotCount = 0;
        this->content.updateAfterCtor = false;
        this->content.ctorHasNoExplicitReturnValue = false;
        this->content.skipDefaultNewObject = false;
        this->content.isPopulated = false;
        this->content.isPolymorphic = false;
        this->content.typeUpdatePending = false;
        this->content.typeIsFinal = false;
        this->content.hasPrototypeChanged = false;
        this->content.callCount = 0;
        Assert(IsConsistent());
    }

    ConstructorCache::ConstructorCache(ConstructorCache const * other)
    {
        Assert(other != nullptr);
        this->content.type = other->content.type;
        this->content.scriptContext = other->content.scriptContext;
        this->content.slotCount = other->content.slotCount;
        this->content.inlineSlotCount = other->content.inlineSlotCount;
        this->content.updateAfterCtor = other->content.updateAfterCtor;
        this->content.ctorHasNoExplicitReturnValue = other->content.ctorHasNoExplicitReturnValue;
        this->content.skipDefaultNewObject = other->content.skipDefaultNewObject;
        this->content.isPopulated = other->content.isPopulated;
        this->content.isPolymorphic = other->content.isPolymorphic;
        this->content.typeUpdatePending = other->content.typeUpdatePending;
        this->content.typeIsFinal = other->content.typeIsFinal;
        this->content.hasPrototypeChanged = other->content.hasPrototypeChanged;
        this->content.callCount = other->content.callCount;
        Assert(IsConsistent());
    }

    void ConstructorCache::Populate(DynamicType* type, ScriptContext* scriptContext, bool ctorHasNoExplicitReturnValue, bool updateAfterCtor)
    {
        Assert(scriptContext == type->GetScriptContext());
        Assert(type->GetIsShared());
        Assert(IsConsistent());
        Assert(!this->content.isPopulated || this->content.isPolymorphic);
        Assert(type->GetTypeHandler()->GetSlotCapacity() <= MaxCachedSlotCount);
        this->content.isPopulated = true;
        this->content.type = type;
        this->content.scriptContext = scriptContext;
        this->content.slotCount = type->GetTypeHandler()->GetSlotCapacity();
        this->content.inlineSlotCount = type->GetTypeHandler()->GetInlineSlotCapacity();
        this->content.ctorHasNoExplicitReturnValue = ctorHasNoExplicitReturnValue;
        this->content.updateAfterCtor = updateAfterCtor;
        Assert(IsConsistent());
    }

    void ConstructorCache::PopulateForSkipDefaultNewObject(ScriptContext* scriptContext)
    {
        Assert(IsConsistent());
        Assert(!this->content.isPopulated);
        this->content.isPopulated = true;
        this->guard.value = CtorCacheGuardValues::Special;
        this->content.scriptContext = scriptContext;
        this->content.skipDefaultNewObject = true;
        Assert(IsConsistent());
    }

    bool ConstructorCache::TryUpdateAfterConstructor(DynamicType* type, ScriptContext* scriptContext)
    {
        Assert(scriptContext == type->GetScriptContext());
        Assert(type->GetTypeHandler()->GetMayBecomeShared());
        Assert(IsConsistent());
        Assert(this->content.isPopulated);
        Assert(this->content.scriptContext == scriptContext);
        Assert(!this->content.typeUpdatePending);
        Assert(this->content.ctorHasNoExplicitReturnValue);

        if (type->GetTypeHandler()->GetSlotCapacity() > MaxCachedSlotCount)
        {
            return false;
        }

        if (type->GetIsShared())
        {
            this->content.type = type;
            this->content.typeIsFinal = true;
            this->content.pendingType = nullptr;
        }
        else
        {
            AssertMsg(false, "No one calls this part of the code?");
            this->guard.value = CtorCacheGuardValues::Special;
            this->content.pendingType = type;
            this->content.typeUpdatePending = true;
        }
        this->content.slotCount = type->GetTypeHandler()->GetSlotCapacity();
        this->content.inlineSlotCount = type->GetTypeHandler()->GetInlineSlotCapacity();
        Assert(IsConsistent());
        return true;
    }

    void ConstructorCache::UpdateInlineSlotCount()
    {
        Assert(IsConsistent());
        Assert(this->content.isPopulated);
        Assert(IsEnabled() || NeedsTypeUpdate());
        DynamicType* type = this->content.typeUpdatePending ? this->content.pendingType : this->content.type;
        DynamicTypeHandler* typeHandler = type->GetTypeHandler();
        // Inline slot capacity should never grow as a result of shrinking.
        Assert(typeHandler->GetInlineSlotCapacity() <= this->content.inlineSlotCount);
        // Slot capacity should never grow as a result of shrinking.
        Assert(typeHandler->GetSlotCapacity() <= this->content.slotCount);
        this->content.slotCount = typeHandler->GetSlotCapacity();
        this->content.inlineSlotCount = typeHandler->GetInlineSlotCapacity();
        Assert(IsConsistent());
    }

    void ConstructorCache::EnableAfterTypeUpdate()
    {
        Assert(IsConsistent());
        Assert(this->content.isPopulated);
        Assert(!IsEnabled());
        Assert(this->guard.value == CtorCacheGuardValues::Special);
        Assert(this->content.typeUpdatePending);
        Assert(this->content.slotCount == this->content.pendingType->GetTypeHandler()->GetSlotCapacity());
        Assert(this->content.inlineSlotCount == this->content.pendingType->GetTypeHandler()->GetInlineSlotCapacity());
        Assert(this->content.pendingType->GetIsShared());
        this->content.type = this->content.pendingType;
        this->content.typeIsFinal = true;
        this->content.pendingType = nullptr;
        this->content.typeUpdatePending = false;
        Assert(IsConsistent());
    }

    ConstructorCache* ConstructorCache::EnsureValidInstance(ConstructorCache* currentCache, ScriptContext* scriptContext)
    {
        Assert(currentCache != nullptr);

        ConstructorCache* newCache = currentCache;

        // If the old cache has been invalidated, we need to create a new one to avoid incorrectly re-validating
        // caches that may have been hard-coded in the JIT-ed code with different prototype and type.  However, if
        // the cache is already polymorphic, it will not be hard-coded, and hence we don't need to allocate a new
        // one - in case the prototype property changes frequently.
        if (ConstructorCache::IsDefault(currentCache) || (currentCache->IsInvalidated() && !currentCache->IsPolymorphic()))
        {
            // Review (jedmiad): I don't think we need to zero the struct, since we initialize each field.
            newCache = RecyclerNew(scriptContext->GetRecycler(), ConstructorCache);
            // TODO: Consider marking the cache as polymorphic only if the prototype and type actually changed.  In fact,
            // if they didn't change we could reuse the same cache and simply mark it as valid.  Not really true.  The cache
            // might have been invalidated due to a property becoming read-only.  In that case we can't re-validate an old
            // monomorphic cache.  We must allocate a new one.
            newCache->content.isPolymorphic = currentCache->content.isPopulated && currentCache->content.hasPrototypeChanged;
        }

        // If we kept the old invalidated cache, it better be marked as polymorphic.
        Assert(!newCache->IsInvalidated() || newCache->IsPolymorphic());

        // If the cache was polymorphic, we shouldn't have allocated a new one.
        Assert(!currentCache->IsPolymorphic() || newCache == currentCache);

        return newCache;
    }

    void ConstructorCache::InvalidateOnPrototypeChange()
    {
        if (IsDefault(this))
        {
            Assert(this->guard.value == CtorCacheGuardValues::Invalid);
            Assert(!this->content.isPopulated);
        }
        else if (this->guard.value == CtorCacheGuardValues::Special && this->content.skipDefaultNewObject)
        {
            // Do nothing.  If we skip the default object, changes to the prototype property don't affect
            // what we'll do during object allocation.

            // Can't assert the following because we set the prototype property during library initialization.
            // AssertMsg(false, "Overriding a prototype on a built-in constructor should be illegal.");
        }
        else
        {
            this->guard.value = CtorCacheGuardValues::Invalid;
            this->content.hasPrototypeChanged = true;
            // Make sure we don't leak the old type.
            Assert(this->content.type == nullptr);
            this->content.pendingType = nullptr;
            Assert(this->content.pendingType == nullptr);
            Assert(IsInvalidated());
        }
        Assert(IsConsistent());
    }

#if DBG_DUMP
    void ConstructorCache::Dump() const
    {
        Output::Print(_u("guard value or type = 0x%p, script context = 0x%p, pending type = 0x%p, slots = %d, inline slots = %d, populated = %d, polymorphic = %d, update cache = %d, update type = %d, skip default = %d, no return = %d"),
            this->GetRawGuardValue(), this->GetScriptContext(), this->GetPendingType(), this->GetSlotCount(), this->GetInlineSlotCount(),
            this->IsPopulated(), this->IsPolymorphic(), this->GetUpdateCacheAfterCtor(), this->GetTypeUpdatePending(),
            this->GetSkipDefaultNewObject(), this->GetCtorHasNoExplicitReturnValue());
    }
#endif
}
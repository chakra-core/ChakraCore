//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    enum class CtorCacheGuardValues : intptr_t
    {
        TagFlag = 0x01,

        Invalid = 0x00,
        Special = TagFlag
    };
    ENUM_CLASS_HELPERS(CtorCacheGuardValues, intptr_t);

    #define MaxCachedSlotCount 65535

    struct ConstructorCache
    {
        friend class JavascriptFunction;

        struct GuardStruct
        {
            Field(CtorCacheGuardValues) value;
        };

        struct ContentStruct
        {
            Field(DynamicType*) type;
            Field(ScriptContext*) scriptContext;
            // In a pinch we could eliminate this and store type pending sharing in the type field as long
            // as the guard value flags fit below the object alignment boundary.  However, this wouldn't
            // keep the type alive, so it would only work if we zeroed constructor caches before GC.
            Field(DynamicType*) pendingType;

            // We cache only types whose slotCount < 64K to ensure the slotCount field doesn't look like a pointer to the recycler.
            Field(int) slotCount;

            // This layout (i.e. one-byte bit fields first, then the one-byte updateAfterCtor, and then the two byte inlineSlotCount) is
            // chosen intentionally to make sure the whole four bytes never look like a pointer and create a false reference pinning something
            // in recycler heap.  The isPopulated bit is always set when the cache holds any data - even if it got invalidated.
            Field(bool) isPopulated : 1;
            Field(bool) isPolymorphic : 1;
            Field(bool) typeUpdatePending : 1;
            Field(bool) ctorHasNoExplicitReturnValue : 1;
            Field(bool) skipDefaultNewObject : 1;
            // This field indicates that the type stored in this cache is the final type after constructor.
            Field(bool) typeIsFinal : 1;
            // This field indicates that the constructor cache has been invalidated due to a constructor's prototype property change.
            // We use this flag to determine if we should mark the cache as polymorphic and not attempt subsequent optimizations.
            // The cache may also be invalidated due to a guard invalidation resulting from some property change (e.g. in proto chain),
            // in which case we won't deem the cache polymorphic.
            Field(bool) hasPrototypeChanged : 1;

            Field(uint8) callCount;

            // Separate from the bit field below for convenient compare from the JIT-ed code. Doesn't currently increase the size.
            // If size becomes an issue, we could merge back into the bit field and use a TEST instead of CMP.
            Field(bool) updateAfterCtor;

            Field(int16) inlineSlotCount;
        };

        union
        {
            Field(GuardStruct) guard;
            Field(ContentStruct) content;
        };

        CompileAssert(offsetof(GuardStruct, value) == offsetof(ContentStruct, type));
        CompileAssert(sizeof(((GuardStruct*)nullptr)->value) == sizeof(((ContentStruct*)nullptr)->type));
        CompileAssert(static_cast<intptr_t>(CtorCacheGuardValues::Invalid) == static_cast<intptr_t>(NULL));

        static ConstructorCache DefaultInstance;

    public:
        ConstructorCache();
        ConstructorCache(ConstructorCache const * other);

        static size_t const GetOffsetOfGuardValue() { return offsetof(Js::ConstructorCache, guard.value); }
        static size_t const GetSizeOfGuardValue() { return sizeof(((Js::ConstructorCache*)nullptr)->guard.value); }

        void Populate(DynamicType* type, ScriptContext* scriptContext, bool ctorHasNoExplicitReturnValue, bool updateAfterCtor);
        void PopulateForSkipDefaultNewObject(ScriptContext* scriptContext);
        bool TryUpdateAfterConstructor(DynamicType* type, ScriptContext* scriptContext);
        void UpdateInlineSlotCount();
        void EnableAfterTypeUpdate();

        intptr_t GetRawGuardValue() const
        {
            return static_cast<intptr_t>(this->guard.value);
        }

        DynamicType* GetGuardValueAsType() const
        {
            return reinterpret_cast<DynamicType*>(this->guard.value & ~CtorCacheGuardValues::TagFlag);
        }

        DynamicType* GetType() const
        {
            Assert(static_cast<intptr_t>(this->guard.value & CtorCacheGuardValues::TagFlag) == 0);
            return this->content.type;
        }

        DynamicType* GetPendingType() const
        {
            return this->content.pendingType;
        }

        ScriptContext* GetScriptContext() const
        {
            return this->content.scriptContext;
        }

        int GetSlotCount() const
        {
            return this->content.slotCount;
        }

        int16 GetInlineSlotCount() const
        {
            return this->content.inlineSlotCount;
        }

        static bool IsDefault(const ConstructorCache* constructorCache)
        {
            return constructorCache == &ConstructorCache::DefaultInstance;
        }

        bool IsDefault() const
        {
            return IsDefault(this);
        }

        bool IsPopulated() const
        {
            Assert(IsConsistent());
            return this->content.isPopulated;
        }

        bool IsEmpty() const
        {
            Assert(IsConsistent());
            return !this->content.isPopulated;
        }

        bool IsPolymorphic() const
        {
            Assert(IsConsistent());
            return this->content.isPolymorphic;
        }

        bool GetSkipDefaultNewObject() const
        {
            return this->content.skipDefaultNewObject;
        }

        bool GetCtorHasNoExplicitReturnValue() const
        {
            return this->content.ctorHasNoExplicitReturnValue;
        }

        bool GetUpdateCacheAfterCtor() const
        {
            return this->content.updateAfterCtor;
        }

        bool GetTypeUpdatePending() const
        {
            return this->content.typeUpdatePending;
        }

        bool IsEnabled() const
        {
            return GetGuardValueAsType() != nullptr;
        }

        bool IsInvalidated() const
        {
            return this->guard.value == CtorCacheGuardValues::Invalid && this->content.isPopulated;
        }

        bool NeedsTypeUpdate() const
        {
            return this->guard.value == CtorCacheGuardValues::Special && this->content.typeUpdatePending;
        }

        uint8 CallCount() const
        {
            return content.callCount;
        }

        void IncCallCount()
        {
            ++content.callCount;
            Assert(content.callCount != 0);
        }

        bool NeedsUpdateAfterCtor() const
        {
            return this->content.updateAfterCtor;
        }

        bool IsNormal() const
        {
            return this->guard.value != CtorCacheGuardValues::Invalid && static_cast<intptr_t>(this->guard.value & CtorCacheGuardValues::TagFlag) == 0;
        }

        bool SkipDefaultNewObject() const
        {
            return this->guard.value == CtorCacheGuardValues::Special && this->content.skipDefaultNewObject;
        }

        bool IsSetUpForJit() const
        {
            return GetRawGuardValue() != NULL && !IsPolymorphic() && !NeedsUpdateAfterCtor() && (IsNormal() || SkipDefaultNewObject());
        }

        void ClearUpdateAfterCtor()
        {
            Assert(IsConsistent());
            Assert(this->content.isPopulated);
            Assert(this->content.updateAfterCtor);
            this->content.updateAfterCtor = false;
            Assert(IsConsistent());
        }

        static ConstructorCache* EnsureValidInstance(ConstructorCache* currentCache, ScriptContext* scriptContext);

        const void* GetAddressOfGuardValue() const
        {
            return reinterpret_cast<const void*>(&this->guard.value);
        }

        static uint32 GetOffsetOfUpdateAfterCtor()
        {
            return offsetof(ConstructorCache, content.updateAfterCtor);
        }

        void InvalidateAsGuard()
        {
            Assert(!IsDefault(this));
            this->guard.value = CtorCacheGuardValues::Invalid;
            // Make sure we don't leak the types.
            Assert(this->content.type == nullptr);
            Assert(this->content.pendingType == nullptr);
            Assert(IsInvalidated());
            Assert(IsConsistent());
        }

    #if DBG
        bool IsConsistent() const
        {
            return this->guard.value == CtorCacheGuardValues::Invalid ||
                (this->content.isPopulated && (
                (this->guard.value == CtorCacheGuardValues::Special && !this->content.updateAfterCtor && this->content.skipDefaultNewObject && !this->content.typeUpdatePending && this->content.slotCount == 0 && this->content.inlineSlotCount == 0 && this->content.pendingType == nullptr) ||
                    (this->guard.value == CtorCacheGuardValues::Special && !this->content.updateAfterCtor && this->content.typeUpdatePending && !this->content.skipDefaultNewObject && this->content.pendingType != nullptr) ||
                    ((this->guard.value & CtorCacheGuardValues::TagFlag) == CtorCacheGuardValues::Invalid && !this->content.skipDefaultNewObject && !this->content.typeUpdatePending && this->content.pendingType == nullptr)));
        }
    #endif

    #if DBG_DUMP
        void Dump() const;
    #endif

    private:
        void InvalidateOnPrototypeChange();
    };
}
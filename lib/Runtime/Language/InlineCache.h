//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#define TypeWithAuxSlotTag(_t) \
    (reinterpret_cast<Type*>(reinterpret_cast<size_t>(_t) | InlineCacheAuxSlotTypeTag))
#define TypeWithoutAuxSlotTag(_t) \
    (reinterpret_cast<Js::Type*>(reinterpret_cast<size_t>(_t) & ~InlineCacheAuxSlotTypeTag))
#define TypeHasAuxSlotTag(_t) \
    (!!(reinterpret_cast<size_t>(_t) & InlineCacheAuxSlotTypeTag))

#if defined(_M_IX86_OR_ARM32)
#define PolymorphicInlineCacheShift 5 // On 32 bit architectures, the least 5 significant bits of a DynamicTypePointer is 0
#else
#define PolymorphicInlineCacheShift 6 // On 64 bit architectures, the least 6 significant bits of a DynamicTypePointer is 0
#endif

// forward decl
class JITType;
struct InlineCacheData;
template <class TAllocator> class JITTypeHolderBase;
typedef JITTypeHolderBase<void> JITTypeHolder;
typedef JITTypeHolderBase<Recycler> RecyclerJITTypeHolder;

namespace Js
{
    enum CacheType : byte
    {
        CacheType_None,
        CacheType_Local,
        CacheType_Proto,
        CacheType_LocalWithoutProperty,
        CacheType_Getter,
        CacheType_Setter,
        CacheType_TypeProperty,
    };

    enum SlotType : byte
    {
        SlotType_None,
        SlotType_Inline,
        SlotType_Aux,
    };

    struct PropertyCacheOperationInfo
    {
        PropertyCacheOperationInfo()
            : cacheType(CacheType_None), slotType(SlotType_None), isPolymorphic(false)
        {
        }

        CacheType cacheType;
        SlotType slotType;
        bool isPolymorphic;
    };

    struct JitTimeInlineCache;
    struct InlineCache
    {
        static const int CacheLayoutSelectorBitCount = 1;
        static const int RequiredAuxSlotCapacityBitCount = 15;
        static const bool IsPolymorphic = false;

        InlineCache() {}

        union
        {
            // Invariants:
            // - Type* fields do not overlap.
            // - "next" field is non-null iff the cache is linked in a list of proto-caches
            //   (see ScriptContext::RegisterProtoInlineCache and ScriptContext::InvalidateProtoCaches).

            struct s_local
            {
                Type* type;

                // PatchPutValue caches here the type the object has before a new property is added.
                // If this type is hit again we can immediately change the object's type to "type"
                // and store the value into the slot "slotIndex".
                Type* typeWithoutProperty;

                union
                {
                    struct
                    {
                        uint16 isLocal : 1;
                        uint16 requiredAuxSlotCapacity : 15;     // Maximum auxiliary slot capacity (for a path type) must be < 2^16
                    };
                    struct
                    {
                        uint16 rawUInt16;                        // Required for access from JIT-ed code
                    };
                };
                uint16 slotIndex;
            } local;

            struct s_proto
            {
                uint16 isProto : 1;
                uint16 isMissing : 1;
                uint16 unused : 14;
                uint16 slotIndex;

                // It's OK for the type in proto layout to overlap with typeWithoutProperty in the local layout, because
                // we only use typeWithoutProperty on field stores, which can never have a proto layout.
                Type* type;

                DynamicObject* prototypeObject;
            } proto;

            struct s_accessor
            {
                DynamicObject *object;

                union
                {
                    struct {
                        uint16 isAccessor : 1;
                        uint16 flags : 2;
                        uint16 isOnProto : 1;
                        uint16 unused : 12;
                    };
                    uint16 rawUInt16;
                };
                uint16 slotIndex;

                Type * type;
            } accessor;

            CompileAssert(sizeof(s_local) == sizeof(s_proto));
            CompileAssert(sizeof(s_local) == sizeof(s_accessor));
        } u;

        InlineCache** invalidationListSlotPtr;

        bool IsEmpty() const
        {
            return u.local.type == nullptr;
        }

        bool IsLocal() const
        {
            return u.local.isLocal;
        }

        bool IsProto() const
        {
            return u.proto.isProto;
        }

        DynamicObject * GetPrototypeObject() const
        {
            Assert(IsProto());
            return u.proto.prototypeObject;
        }

        DynamicObject * GetAccessorObject() const
        {
            Assert(IsAccessor());
            return u.accessor.object;
        }

        bool IsAccessor() const
        {
            return u.accessor.isAccessor;
        }

        bool IsAccessorOnProto() const
        {
            return IsAccessor() && u.accessor.isOnProto;
        }

        bool IsGetterAccessor() const
        {
            return IsAccessor() && !!(u.accessor.flags & InlineCacheGetterFlag);
        }

        bool IsGetterAccessorOnProto() const
        {
            return IsGetterAccessor() && u.accessor.isOnProto;
        }

        bool IsSetterAccessor() const
        {
            return IsAccessor() && !!(u.accessor.flags & InlineCacheSetterFlag);
        }

        bool IsSetterAccessorOnProto() const
        {
            return IsSetterAccessor() && u.accessor.isOnProto;
        }

        Type* GetRawType() const
        {
            return IsLocal() ? u.local.type : (IsProto() ? u.proto.type : (IsAccessor() ? u.accessor.type : nullptr));
        }

        Type* GetType() const
        {
            return TypeWithoutAuxSlotTag(GetRawType());
        }

        template<bool isAccessor>
        bool HasDifferentType(const bool isProto, const Type * type, const Type * typeWithoutProperty) const;

        bool HasType_Flags(const Type * type) const
        {
            return u.accessor.type == type || u.accessor.type == TypeWithAuxSlotTag(type);
        }

        bool HasDifferentType(const Type * type) const
        {
            return !IsEmpty() && GetType() != type;
        }

        bool RemoveFromInvalidationList()
        {
            if (this->invalidationListSlotPtr == nullptr)
            {
                return false;
            }

            Assert(*this->invalidationListSlotPtr == this);

            *this->invalidationListSlotPtr = nullptr;
            this->invalidationListSlotPtr = nullptr;

            return true;
        }

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        const char16 *LayoutString() const
        {
            if (IsEmpty())
            {
                return _u("Empty");
            }
            if (IsLocal())
            {
                return _u("Local");
            }
            if (IsAccessor())
            {
                return _u("Accessor");
            }
            return _u("Proto");
        }
#endif

    public:
        void CacheLocal(
            Type *const type,
            const PropertyId propertyId,
            const PropertyIndex propertyIndex,
            const bool isInlineSlot,
            Type *const typeWithoutProperty,
            int requiredAuxSlotCapacity,
            ScriptContext *const requestContext);

        void CacheProto(
            DynamicObject *const prototypeObjectWithProperty,
            const PropertyId propertyId,
            const PropertyIndex propertyIndex,
            const bool isInlineSlot,
            const bool isMissing,
            Type *const type,
            ScriptContext *const requestContext);

        void CacheMissing(
            DynamicObject *const missingPropertyHolder,
            const PropertyId propertyId,
            const PropertyIndex propertyIndex,
            const bool isInlineSlot,
            Type *const type,
            ScriptContext *const requestContext);

        void CacheAccessor(
            const bool isGetter,
            const PropertyId propertyId,
            const PropertyIndex propertyIndex,
            const bool isInlineSlot,
            Type *const type,
            DynamicObject *const object,
            const bool isOnProto,
            ScriptContext *const requestContext);

        template<
            bool CheckLocal,
            bool CheckProto,
            bool CheckAccessor,
            bool CheckMissing,
            bool ReturnOperationInfo>
        bool TryGetProperty(
            Var const instance,
            RecyclableObject *const propertyObject,
            const PropertyId propertyId,
            Var *const propertyValue,
            ScriptContext *const requestContext,
            PropertyCacheOperationInfo *const operationInfo);

        template<
            bool CheckLocal,
            bool CheckLocalTypeWithoutProperty,
            bool CheckAccessor,
            bool ReturnOperationInfo>
        bool TrySetProperty(
            RecyclableObject *const object,
            const PropertyId propertyId,
            Var propertyValue,
            ScriptContext *const requestContext,
            PropertyCacheOperationInfo *const operationInfo,
            const PropertyOperationFlags propertyOperationFlags = PropertyOperation_None);

        bool PretendTryGetProperty(Type *const type, PropertyCacheOperationInfo * operationInfo) const;
        bool PretendTrySetProperty(Type *const type, Type *const oldType, PropertyCacheOperationInfo * operationInfo) const;

        void Clear();
        void RemoveFromInvalidationListAndClear(ThreadContext* threadContext);
        template <class TAllocator>
        InlineCache *Clone(TAllocator *const allocator);
        InlineCache *Clone(Js::PropertyId propertyId, ScriptContext* scriptContext);
        void CopyTo(PropertyId propertyId, ScriptContext * scriptContext, InlineCache * const clone);
#if ENABLE_FIXED_FIELDS
        bool TryGetFixedMethodFromCache(Js::FunctionBody* functionBody, uint cacheId, Js::JavascriptFunction** pFixedMethod);
#endif

        bool GetGetterSetter(Type *const type, RecyclableObject **callee);
        bool GetCallApplyTarget(RecyclableObject* obj, RecyclableObject **callee);

        static uint GetGetterFlagMask()
        {
            // First bit is marked for isAccessor in the accessor cache layout.
            return  InlineCacheGetterFlag << 1;
        }

        static uint GetSetterFlagMask()
        {
            // First bit is marked for isAccessor in the accessor cache layout.
            return  InlineCacheSetterFlag << 1;
        }

        static uint GetGetterSetterFlagMask()
        {
            // First bit is marked for isAccessor in the accessor cache layout.
            return  (InlineCacheGetterFlag | InlineCacheSetterFlag) << 1;
        }

        bool NeedsToBeRegisteredForProtoInvalidation() const;
        bool NeedsToBeRegisteredForStoreFieldInvalidation() const;

#if DEBUG
        bool ConfirmCacheMiss(const Type * oldType, const PropertyValueInfo* info) const;
        bool NeedsToBeRegisteredForInvalidation() const;
        static void VerifyRegistrationForInvalidation(const InlineCache* cache, ScriptContext* scriptContext, Js::PropertyId propertyId);
#endif

#if DBG_DUMP
        void Dump();
#endif
    };

#if defined(_M_IX86_OR_ARM32)
    CompileAssert(sizeof(InlineCache) == 0x10);
#else
    CompileAssert(sizeof(InlineCache) == 0x20);
#endif

    CompileAssert(sizeof(InlineCache) == sizeof(InlineCacheAllocator::CacheLayout));
    CompileAssert(offsetof(InlineCache, invalidationListSlotPtr) == offsetof(InlineCacheAllocator::CacheLayout, strongRef));

    class PolymorphicInlineCache _ABSTRACT : public FinalizableObject
    {
#ifdef INLINE_CACHE_STATS
        friend class Js::ScriptContext;
#endif

    public:
        static const bool IsPolymorphic = true;

    protected:
        FieldNoBarrier(InlineCache *) inlineCaches;
        Field(uint16) size;
        Field(bool) ignoreForEquivalentObjTypeSpec;
        Field(bool) cloneForJitTimeUse;

        Field(int32) inlineCachesFillInfo;

        PolymorphicInlineCache(InlineCache * inlineCaches, uint16 size)
            : inlineCaches(inlineCaches), size(size), ignoreForEquivalentObjTypeSpec(false), cloneForJitTimeUse(true), inlineCachesFillInfo(0)
        {
        }

    public:

        bool CanAllocateBigger() { return GetSize() < MaxPolymorphicInlineCacheSize; }
        static uint16 GetNextSize(uint16 currentSize)
        {
            if (currentSize == MaxPolymorphicInlineCacheSize)
            {
                return 0;
            }
            else if (currentSize == MinPropertyStringInlineCacheSize)
            {
                CompileAssert(MinPropertyStringInlineCacheSize < MinPolymorphicInlineCacheSize);
                return MinPolymorphicInlineCacheSize;
            }
            else
            {
                Assert(currentSize >= MinPolymorphicInlineCacheSize && currentSize <= (MaxPolymorphicInlineCacheSize / 2));
                return currentSize * 2;
            }
        }

        template<bool isAccessor>
        bool HasDifferentType(const bool isProto, const Type * type, const Type * typeWithoutProperty) const;
        bool HasType_Flags(const Type * type) const;

        InlineCache * GetInlineCaches() const { return inlineCaches; }
        uint16 GetSize() const { return size; }
        bool GetIgnoreForEquivalentObjTypeSpec() const { return this->ignoreForEquivalentObjTypeSpec; }
        void SetIgnoreForEquivalentObjTypeSpec(bool value) { this->ignoreForEquivalentObjTypeSpec = value; }
        bool GetCloneForJitTimeUse() const { return this->cloneForJitTimeUse; }
        void SetCloneForJitTimeUse(bool value) { this->cloneForJitTimeUse = value; }
        uint32 GetInlineCachesFillInfo() { return this->inlineCachesFillInfo; }
        void UpdateInlineCachesFillInfo(uint32 index, bool set);
        bool IsFull();
        void Clear(Type * type);

        virtual void Dispose(bool isShutdown) override { };
        virtual void Mark(Recycler *recycler) override { AssertMsg(false, "Mark called on object that isn't TrackableObject"); }

        void CacheLocal(
            Type *const type,
            const PropertyId propertyId,
            const PropertyIndex propertyIndex,
            const bool isInlineSlot,
            Type *const typeWithoutProperty,
            int requiredAuxSlotCapacity,
            ScriptContext *const requestContext);

        void CacheProto(
            DynamicObject *const prototypeObjectWithProperty,
            const PropertyId propertyId,
            const PropertyIndex propertyIndex,
            const bool isInlineSlot,
            const bool isMissing,
            Type *const type,
            ScriptContext *const requestContext);

        void CacheAccessor(
            const bool isGetter,
            const PropertyId propertyId,
            const PropertyIndex propertyIndex,
            const bool isInlineSlot,
            Type *const type,
            DynamicObject *const object,
            const bool isOnProto,
            ScriptContext *const requestContext);

        template<
            bool CheckLocal,
            bool CheckProto,
            bool CheckAccessor,
            bool CheckMissing,
            bool IsInlineCacheAvailable,
            bool ReturnOperationInfo>
        bool TryGetProperty(
            Var const instance,
            RecyclableObject *const propertyObject,
            const PropertyId propertyId,
            Var *const propertyValue,
            ScriptContext *const requestContext,
            PropertyCacheOperationInfo *const operationInfo,
            InlineCache *const inlineCacheToPopulate);

        template<
            bool CheckLocal,
            bool CheckLocalTypeWithoutProperty,
            bool CheckAccessor,
            bool ReturnOperationInfo,
            bool PopulateInlineCache>
        bool TrySetProperty(
            RecyclableObject *const object,
            const PropertyId propertyId,
            Var propertyValue,
            ScriptContext *const requestContext,
            PropertyCacheOperationInfo *const operationInfo,
            InlineCache *const inlineCacheToPopulate,
            const PropertyOperationFlags propertyOperationFlags = PropertyOperation_None);

        bool PretendTryGetProperty(Type *const type, PropertyCacheOperationInfo * operationInfo);
        bool PretendTrySetProperty(Type *const type, Type *const oldType, PropertyCacheOperationInfo * operationInfo);

        void CopyTo(PropertyId propertyId, ScriptContext* scriptContext, PolymorphicInlineCache *const clone);

#if DBG_DUMP
        void Dump();
#endif

        uint GetInlineCacheIndexForType(const Type * type) const
        {
            return (((size_t)type) >> PolymorphicInlineCacheShift) & (GetSize() - 1);
        }

#ifdef INLINE_CACHE_STATS
        virtual void PrintStats(InlineCacheData *data) const = 0;
        virtual ScriptContext* GetScriptContext() const = 0;
#endif

        static uint32 GetOffsetOfSize() { return offsetof(Js::PolymorphicInlineCache, size); }
        static uint32 GetOffsetOfInlineCaches() { return offsetof(Js::PolymorphicInlineCache, inlineCaches); }

    private:
        uint GetNextInlineCacheIndex(uint index) const
        {
            if (++index == GetSize())
            {
                index = 0;
            }
            return index;
        }

        template<bool CheckLocal, bool CheckProto, bool CheckAccessor>
        void CloneInlineCacheToEmptySlotInCollision(Type *const type, uint index);

#ifdef CLONE_INLINECACHE_TO_EMPTYSLOT
        template <typename TDelegate>
        bool CheckClonedInlineCache(uint inlineCacheIndex, TDelegate mapper);
#endif
#if INTRUSIVE_TESTTRACE_PolymorphicInlineCache
        uint GetEntryCount()
        {
            uint count = 0;
            for (uint i = 0; i < size; ++i)
            {
                if (!inlineCaches[i].IsEmpty())
                {
                    count++;
                }
            }
            return count;
        }
#endif
    };


    class FunctionBodyPolymorphicInlineCache sealed : public PolymorphicInlineCache
    {
    private:
        FunctionBody * functionBody;

        // DList chaining all polymorphic inline caches of a FunctionBody together.
        // Since PolymorphicInlineCache is a leaf object, these references do not keep
        // the polymorphic inline caches alive. When a PolymorphicInlineCache is finalized,
        // it removes itself from the list and deletes its inline cache array.
        // We maintain this linked list of polymorphic caches because when we allocate a larger cache,
        // the old one might still be used by some code on the stack.  Consequently, we can't release
        // the inline cache array back to the arena allocator.
        FunctionBodyPolymorphicInlineCache * next;
        FunctionBodyPolymorphicInlineCache * prev;

        FunctionBodyPolymorphicInlineCache(InlineCache * inlineCaches, uint16 size, FunctionBody * functionBody)
            : PolymorphicInlineCache(inlineCaches, size), functionBody(functionBody), next(nullptr), prev(nullptr)
        {
            Assert((size == 0 && inlineCaches == nullptr) ||
                (inlineCaches != nullptr && size >= MinPolymorphicInlineCacheSize && size <= MaxPolymorphicInlineCacheSize));
        }
    public:
        static FunctionBodyPolymorphicInlineCache * New(uint16 size, FunctionBody * functionBody);

#ifdef INLINE_CACHE_STATS
        virtual void PrintStats(InlineCacheData *data) const override;
        virtual ScriptContext* GetScriptContext() const override;
#endif

        virtual void Finalize(bool isShutdown) override;
    };

    class ScriptContextPolymorphicInlineCache sealed : public PolymorphicInlineCache
    {
    private:
        Field(JavascriptLibrary*) javascriptLibrary;

        ScriptContextPolymorphicInlineCache(InlineCache * inlineCaches, uint16 size, JavascriptLibrary * javascriptLibrary)
            : PolymorphicInlineCache(inlineCaches, size), javascriptLibrary(javascriptLibrary)
        {
            Assert((size == 0 && inlineCaches == nullptr) ||
                (inlineCaches != nullptr && size >= MinPropertyStringInlineCacheSize && size <= MaxPolymorphicInlineCacheSize));
        }

    public:
        static ScriptContextPolymorphicInlineCache * New(uint16 size, JavascriptLibrary * javascriptLibrary);

#ifdef INLINE_CACHE_STATS
        virtual void PrintStats(InlineCacheData *data) const override;
        virtual ScriptContext* GetScriptContext() const override;
#endif

        virtual void Finalize(bool isShutdown) override;
    };

    // Caches the result of an instanceof operator over a type and a function
    struct IsInstInlineCache
    {
        Type * type;                    // The type of object operand an inline cache caches a result for
        JavascriptFunction * function;  // The function operand an inline cache caches a result for
        JavascriptBoolean * result;     // The result of doing (object instanceof function) where object->type == this->type
        IsInstInlineCache * next;       // Used to link together caches that have the same function operand

    public:
        bool IsEmpty() const { return type == nullptr; }
        bool TryGetResult(Var instance, JavascriptFunction * function, JavascriptBoolean ** result);
        void Cache(Type * instanceType, JavascriptFunction * function, JavascriptBoolean * result, ScriptContext * scriptContext);
        void Unregister(ScriptContext * scriptContext);
        void Clear();

        static uint32 OffsetOfFunction();
        static uint32 OffsetOfResult();
        static uint32 OffsetOfType();

    private:
        void Set(Type * instanceType, JavascriptFunction * function, JavascriptBoolean * result);
    };

    // Two-entry Type-indexed circular cache
    //   cache IsConcatSpreadable() result unless user-defined [@@isConcatSpreadable] exists
    class IsConcatSpreadableCache
    {
        Type *type0, *type1;
        int lastAccess;
        BOOL result0, result1;

    public:
        IsConcatSpreadableCache() :
            type0(nullptr),
            type1(nullptr),
            result0(FALSE),
            result1(FALSE),
            lastAccess(1)
        {
        }

        bool TryGetIsConcatSpreadable(Type *type, _Out_ BOOL *result)
        {
            Assert(type != nullptr);
            Assert(result != nullptr);

            *result = FALSE;
            if (type0 == type)
            {
                *result = result0;
                lastAccess = 0;
                return true;
            }

            if (type1 == type)
            {
                *result = result1;
                lastAccess = 1;
                return true;
            }

            return false;
        }

        void CacheIsConcatSpreadable(Type *type, BOOL result)
        {
            Assert(type != nullptr);

            if (lastAccess == 0)
            {
                type1 = type;
                result1 = result;
                lastAccess = 1;
            }
            else
            {
                type0 = type;
                result0 = result;
                lastAccess = 0;
            }
        }

        void Invalidate()
        {
            type0 = nullptr;
            type1 = nullptr;
        }
    };

#if defined(_M_IX86_OR_ARM32)
    CompileAssert(sizeof(IsInstInlineCache) == 0x10);
#else
    CompileAssert(sizeof(IsInstInlineCache) == 0x20);
#endif
}

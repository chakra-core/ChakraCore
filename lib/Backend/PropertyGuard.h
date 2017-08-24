//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if ENABLE_NATIVE_CODEGEN
namespace Js
{

class PropertyGuard
{
    friend class PropertyGuardValidator;

private:
    Field(intptr_t) value; // value is address of Js::Type
#if DBG
    Field(bool) wasReincarnated = false;
#endif
public:
    static PropertyGuard* New(Recycler* recycler) { return RecyclerNewLeaf(recycler, Js::PropertyGuard); }
    PropertyGuard() : value(GuardValue::Uninitialized) {}
    PropertyGuard(intptr_t value) : value(value)
    {
        // GuardValue::Invalidated and GuardValue::Invalidated_DuringSweeping can only be set using
        // Invalidate() and InvalidatedDuringSweep() methods respectively.
        Assert(this->value != GuardValue::Invalidated && this->value != GuardValue::Invalidated_DuringSweep);
    }

    inline static size_t const GetSizeOfValue() { return sizeof(((PropertyGuard*)0)->value); }
    inline static size_t const GetOffsetOfValue() { return offsetof(PropertyGuard, value); }

    intptr_t GetValue() const { return this->value; }
    bool IsValid()
    {
        return this->value != GuardValue::Invalidated && this->value != GuardValue::Invalidated_DuringSweep;
    }
    bool IsInvalidatedDuringSweep() { return this->value == GuardValue::Invalidated_DuringSweep; }
    void SetValue(intptr_t value)
    {
        // GuardValue::Invalidated and GuardValue::Invalidated_DuringSweeping can only be set using
        // Invalidate() and InvalidatedDuringSweep() methods respectively.
        Assert(value != GuardValue::Invalidated && value != GuardValue::Invalidated_DuringSweep);
        this->value = value;
    }
    intptr_t const* GetAddressOfValue() { return &this->value; }
    void Invalidate() { this->value = GuardValue::Invalidated; }
    void InvalidateDuringSweep()
    {
#if DBG
        wasReincarnated = true;
#endif
        this->value = GuardValue::Invalidated_DuringSweep;
    }
#if DBG
    bool WasReincarnated() { return this->wasReincarnated; }
#endif
    enum GuardValue : intptr_t
    {
        Invalidated = 0,
        Uninitialized = 1,
        Invalidated_DuringSweep = 2
    };
};

class PropertyGuardValidator
{
    // Required by EquivalentTypeGuard::SetType.
    CompileAssert(offsetof(PropertyGuard, value) == 0);
    // CompileAssert(offsetof(ConstructorCache, guard.value) == offsetof(PropertyGuard, value));
};

class JitIndexedPropertyGuard : public Js::PropertyGuard
{
private:
    int index;

public:
    JitIndexedPropertyGuard(intptr_t value, int index) :
        Js::PropertyGuard(value), index(index) {}

    int GetIndex() const { return this->index; }
};

class JitTypePropertyGuard : public Js::JitIndexedPropertyGuard
{
public:
    JitTypePropertyGuard(intptr_t typeAddr, int index) :
        JitIndexedPropertyGuard(typeAddr, index) {}

    intptr_t GetTypeAddr() const { return this->GetValue(); }

};

struct TypeGuardTransferEntry
{
    PropertyId propertyId;
    JitIndexedPropertyGuard* guards[0];

    TypeGuardTransferEntry() : propertyId(Js::Constants::NoProperty) {}
};

class FakePropertyGuardWeakReference : public RecyclerWeakReference<Js::PropertyGuard>
{
public:
    static FakePropertyGuardWeakReference* New(Recycler* recycler, Js::PropertyGuard* guard)
    {
        Assert(guard != nullptr);
        return RecyclerNewLeaf(recycler, Js::FakePropertyGuardWeakReference, guard);
    }
    FakePropertyGuardWeakReference(const Js::PropertyGuard* guard)
    {
        this->strongRef = (char*)guard;
        this->strongRefHeapBlock = &CollectedRecyclerWeakRefHeapBlock::Instance;
    }

    void Zero()
    {
        Assert(this->strongRef != nullptr);
        this->strongRef = nullptr;
    }
};

struct CtorCacheGuardTransferEntry
{
    PropertyId propertyId;
    intptr_t caches[0];

    CtorCacheGuardTransferEntry() : propertyId(Js::Constants::NoProperty) {}
};

struct PropertyEquivalenceInfo
{
    PropertyIndex slotIndex;
    bool isAuxSlot;
    bool isWritable;

    PropertyEquivalenceInfo() :
        slotIndex(Constants::NoSlot), isAuxSlot(false), isWritable(false) {}
    PropertyEquivalenceInfo(PropertyIndex slotIndex, bool isAuxSlot, bool isWritable) :
        slotIndex(slotIndex), isAuxSlot(isAuxSlot), isWritable(isWritable) {}
};

struct EquivalentPropertyEntry
{
    Js::PropertyId propertyId;
    Js::PropertyIndex slotIndex;
    bool isAuxSlot;
    bool mustBeWritable;
};

struct TypeEquivalenceRecord
{
    uint propertyCount;
    EquivalentPropertyEntry* properties;
};

struct EquivalentTypeCache
{
    Js::Type* types[EQUIVALENT_TYPE_CACHE_SIZE];
    PropertyGuard *guard;
    TypeEquivalenceRecord record;
    uint nextEvictionVictim;
    bool isLoadedFromProto;
    bool hasFixedValue;

    EquivalentTypeCache() : nextEvictionVictim(EQUIVALENT_TYPE_CACHE_SIZE) {}
    bool ClearUnusedTypes(Recycler *recycler);
    void SetGuard(PropertyGuard *theGuard) { this->guard = theGuard; }
    void SetIsLoadedFromProto() { this->isLoadedFromProto = true; }
    bool IsLoadedFromProto() const { return this->isLoadedFromProto; }
    void SetHasFixedValue() { this->hasFixedValue = true; }
    bool HasFixedValue() const { return this->hasFixedValue; }
};

class JitEquivalentTypeGuard : public JitIndexedPropertyGuard
{
    // This pointer is allocated from background thread first, and then transferred to recycler,
    // so as to keep the cached types alive.
    EquivalentTypeCache* cache;
    uint32 objTypeSpecFldId;
    // TODO: OOP JIT, reenable these asserts
#if DBG && 0
    // Intentionally have as intptr_t so this guard doesn't hold scriptContext
    intptr_t originalScriptContextValue = 0;
#endif

public:
    JitEquivalentTypeGuard(intptr_t typeAddr, int index, uint32 objTypeSpecFldId) :
        JitIndexedPropertyGuard(typeAddr, index), cache(nullptr), objTypeSpecFldId(objTypeSpecFldId)
    {
#if DBG && 0
        originalScriptContextValue = reinterpret_cast<intptr_t>(type->GetScriptContext());
#endif
    }

    intptr_t GetTypeAddr() const { return this->GetValue(); }

    void SetTypeAddr(const intptr_t typeAddr)
    {
#if DBG && 0
        if (originalScriptContextValue == 0)
        {
            originalScriptContextValue = reinterpret_cast<intptr_t>(type->GetScriptContext());
        }
        else
        {
            AssertMsg(originalScriptContextValue == reinterpret_cast<intptr_t>(type->GetScriptContext()), "Trying to set guard type from different script context.");
        }
#endif
        this->SetValue(typeAddr);
    }

    uint32 GetObjTypeSpecFldId() const
    {
        return this->objTypeSpecFldId;
    }

    Js::EquivalentTypeCache* GetCache() const
    {
        return this->cache;
    }

    void SetCache(Js::EquivalentTypeCache* cache)
    {
        this->cache = cache;
    }
};


};
#endif // ENABLE_NATIVE_CODEGEN
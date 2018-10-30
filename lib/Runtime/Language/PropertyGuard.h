//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{

#if ENABLE_NATIVE_CODEGEN
class JitPolyEquivalentTypeGuard;
#endif

class PropertyGuard
{
    friend class PropertyGuardValidator;
#if ENABLE_NATIVE_CODEGEN
    friend class JitPolyEquivalentTypeGuard;
#endif

private:
    Field(intptr_t) value; // value is address of Js::Type
#if ENABLE_NATIVE_CODEGEN
    Field(bool) isPoly;
#endif
#if DBG
    Field(bool) wasReincarnated = false;
#endif
public:
    static PropertyGuard* New(Recycler* recycler) { return RecyclerNewLeaf(recycler, Js::PropertyGuard); }
    PropertyGuard() : value(GuardValue::Uninitialized)
#if ENABLE_NATIVE_CODEGEN
                      , isPoly(false)
#endif
    {
    }
    PropertyGuard(intptr_t value) : value(value)
#if ENABLE_NATIVE_CODEGEN
                      , isPoly(false)
#endif
    {
        // GuardValue::Invalidated and GuardValue::Invalidated_DuringSweeping can only be set using
        // Invalidate() and InvalidatedDuringSweep() methods respectively.
        Assert(this->value != GuardValue::Invalidated && this->value != GuardValue::Invalidated_DuringSweep);
    }

    inline static size_t GetSizeOfValue() { return sizeof(((PropertyGuard*)0)->value); }
    inline static size_t GetOffsetOfValue() { return offsetof(PropertyGuard, value); }

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
    void Invalidate();
    void InvalidateDuringSweep();
#if DBG
    bool WasReincarnated() { return this->wasReincarnated; }
#endif
    enum GuardValue : intptr_t
    {
        Invalidated = 0,
        Uninitialized = 1,
        Invalidated_DuringSweep = 2
    };

#if ENABLE_NATIVE_CODEGEN
    bool IsPoly() const { return this->isPoly; }

    JitPolyEquivalentTypeGuard * AsPolyTypeCheckGuard()
    {
        AssertOrFailFast(this->IsPoly());
        return (JitPolyEquivalentTypeGuard*)(this);
    }
#endif
};

class PropertyGuardValidator
{
    // Required by EquivalentTypeGuard::SetType.
    CompileAssert(offsetof(PropertyGuard, value) == 0);
    // CompileAssert(offsetof(ConstructorCache, guard.value) == offsetof(PropertyGuard, value));
};


#if ENABLE_NATIVE_CODEGEN

class JitIndexedPropertyGuard : public Js::PropertyGuard
{
private:
    int index;

public:
    JitIndexedPropertyGuard(intptr_t value, int index) :
        Js::PropertyGuard(value), index(index) {}

    JitIndexedPropertyGuard(int index) :
        Js::PropertyGuard(), index(index) {}

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

    JitEquivalentTypeGuard(int index, uint32 objTypeSpecFldId) :
        JitIndexedPropertyGuard(index), cache(nullptr), objTypeSpecFldId(objTypeSpecFldId)
    {
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

class JitPolyEquivalentTypeGuard : public JitEquivalentTypeGuard
{
public:
    JitPolyEquivalentTypeGuard(int index, uint32 objTypeSpecFldId) : JitEquivalentTypeGuard(index, objTypeSpecFldId)
    {
        isPoly = true;

        for (uint i = 0; i < GetSize(); i++)
        {
            polyValues[i] = GuardValue::Uninitialized;
        }
    }

private:
    intptr_t polyValues[EQUIVALENT_TYPE_CACHE_SIZE];

public:
    static int32 GetOffsetOfPolyValues() { return offsetof(JitPolyEquivalentTypeGuard, polyValues); }

    // TODO: Revisit optimal size. Make it vary according to the number of caches? Is this worth a variable-size allocation?
    uintptr_t GetSize() const { return EQUIVALENT_TYPE_CACHE_SIZE; }
    intptr_t const * GetAddressOfPolyValues() { return &this->polyValues[0]; }

    intptr_t GetPolyValue(uint8 index) const
    {
        AssertOrFailFast(index < GetSize());
        return polyValues[index];
    }

    void SetPolyValue(intptr_t value, uint8 index)
    {
        AssertOrFailFast(index < GetSize());
        polyValues[index] = value;
    }

    uint8 GetIndexForValue(intptr_t value) const
    {
        return (uint8)((value >> PolymorphicInlineCacheShift) & (GetSize() - 1));
    }

    void Invalidate()
    {
        for (uint8 i = 0; i < GetSize(); i++)
        {
            Invalidate(i);
        }
    }

    void Invalidate(uint8 i)
    {
        AssertOrFailFast(i < GetSize());
        polyValues[i] = GuardValue::Invalidated;
    }

    void InvalidateDuringSweep()
    {
        for (uint8 i = 0; i < GetSize(); i++)
        {
            InvalidateDuringSweep(i);
        }
    }

    void InvalidateDuringSweep(uint8 i)
    {
        AssertOrFailFast(i < GetSize());
        polyValues[i] = GuardValue::Invalidated_DuringSweep;
    }
};

#endif // ENABLE_NATIVE_CODEGEN

inline void PropertyGuard::Invalidate()
{ 
    this->value = GuardValue::Invalidated; 
#if ENABLE_NATIVE_CODEGEN
    if (this->IsPoly())
    {
        this->AsPolyTypeCheckGuard()->JitPolyEquivalentTypeGuard::Invalidate();
    }
#endif
}

inline void PropertyGuard::InvalidateDuringSweep()
{
#if DBG
    wasReincarnated = true;
#endif
    this->value = GuardValue::Invalidated_DuringSweep;
#if ENABLE_NATIVE_CODEGEN
    if (this->IsPoly())
    {
        this->AsPolyTypeCheckGuard()->JitPolyEquivalentTypeGuard::InvalidateDuringSweep();
    }
#endif
}

};

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Memory
{
class Recycler;
class RecyclerNonLeafAllocator;

// Dummy tag classes to mark yes/no write barrier policy
//
struct _write_barrier_policy {};
struct _no_write_barrier_policy {};

// ----------------------------------------------------------------------------
// Field write barrier
//
//      Determines if a field needs to be wrapped in WriteBarrierPtr
// ----------------------------------------------------------------------------

// Type write barrier policy
//
// By default following potentially contains GC pointers and use write barrier policy:
//      pointer, WriteBarrierPtr, _write_barrier_policy
//
template <class T>
struct TypeWriteBarrierPolicy { typedef _no_write_barrier_policy Policy; };
template <class T>
struct TypeWriteBarrierPolicy<T*> { typedef _write_barrier_policy Policy; };
template <class T>
struct TypeWriteBarrierPolicy<WriteBarrierPtr<T>> { typedef _write_barrier_policy Policy; };
template <>
struct TypeWriteBarrierPolicy<_write_barrier_policy> { typedef _write_barrier_policy Policy; };

// AllocatorType write barrier policy
//
// Recycler allocator type => _write_barrier_policy
// Note that Recycler allocator type consists of multiple allocators:
//      Recycler, RecyclerNonLeafAllocator, RecyclerLeafAllocator
//
template <class AllocatorType>
struct _AllocatorTypeWriteBarrierPolicy { typedef _no_write_barrier_policy Policy; };
template <>
struct _AllocatorTypeWriteBarrierPolicy<Recycler> { typedef _write_barrier_policy Policy; };

template <class Policy1, class Policy2>
struct _AndWriteBarrierPolicy { typedef _no_write_barrier_policy Policy; };
template <>
struct _AndWriteBarrierPolicy<_write_barrier_policy, _write_barrier_policy>
{
    typedef _write_barrier_policy Policy;
};

// Combine Allocator + Data => write barrier policy
// Specialize RecyclerNonLeafAllocator
//
template <class Allocator, class T>
struct AllocatorWriteBarrierPolicy
{
    typedef typename AllocatorInfo<Allocator, void>::AllocatorType AllocatorType;
    typedef typename _AndWriteBarrierPolicy<
        typename _AllocatorTypeWriteBarrierPolicy<AllocatorType>::Policy,
        typename TypeWriteBarrierPolicy<T>::Policy>::Policy Policy;
};
template <class T>
struct AllocatorWriteBarrierPolicy<RecyclerNonLeafAllocator, T> { typedef _write_barrier_policy Policy; };
template <>
struct AllocatorWriteBarrierPolicy<RecyclerNonLeafAllocator, int> { typedef _no_write_barrier_policy Policy; };

// Choose write barrier Field type: T unchanged, or WriteBarrierPtr based on Policy.
//
template <class T, class Policy>
struct _WriteBarrierFieldType { typedef T Type; };
template <class T>
struct _WriteBarrierFieldType<T*, _write_barrier_policy> { typedef WriteBarrierPtr<T> Type; };

template <class T,
          class Allocator = Recycler,
          class Policy = typename AllocatorWriteBarrierPolicy<Allocator, T>::Policy>
struct WriteBarrierFieldTypeTraits { typedef typename _WriteBarrierFieldType<T, Policy>::Type Type; };


// ----------------------------------------------------------------------------
// Array write barrier
//
//      Determines if an array operation needs to trigger write barrier
// ----------------------------------------------------------------------------

// ArrayWriteBarrier behavior
//
typedef void FN_VerifyIsNotBarrierAddress(void*, size_t);
extern "C" FN_VerifyIsNotBarrierAddress* g_verifyIsNotBarrierAddress;
template <class Policy>
struct _ArrayWriteBarrier
{
    template <class T>
    static void WriteBarrier(T * address, size_t count)
    {
#if defined(RECYCLER_WRITE_BARRIER)
#if ENABLE_DEBUG_CONFIG_OPTIONS
        if (Js::Configuration::Global.flags.StrictWriteBarrierCheck)
        {
            if (g_verifyIsNotBarrierAddress)
            {
                g_verifyIsNotBarrierAddress(address, count);
            }
        }
#endif
#endif
    }
};

#ifdef RECYCLER_WRITE_BARRIER
template <>
struct _ArrayWriteBarrier<_write_barrier_policy>
{
    template <class T>
    static void WriteBarrier(T * address, size_t count)
    {
        RecyclerWriteBarrierManager::WriteBarrier(address, sizeof(T) * count);
    }
};
#endif

// Determines array write barrier policy based on array item type.
//
// Note: If individual array item needs write barrier, we choose to use
// WriteBarrierPtr with the item type. Thus we only specialize on
// WriteBarrierPtr (and _write_barrier_policy if user wants to force write
// barrier).
//
template <class T>
struct _ArrayItemWriteBarrierPolicy
    { typedef _no_write_barrier_policy Policy; };
template <class T>
struct _ArrayItemWriteBarrierPolicy<WriteBarrierPtr<T>>
    { typedef _write_barrier_policy Policy; };
template <>
struct _ArrayItemWriteBarrierPolicy<_write_barrier_policy>
    { typedef _write_barrier_policy Policy; };

// Trigger write barrier on changing array content if element type determines
// write barrier is needed. Ignore otherwise.
//
template <class T, class PolicyType = T>
void ArrayWriteBarrier(T * address, size_t count)
{
    typedef typename _ArrayItemWriteBarrierPolicy<PolicyType>::Policy Policy;
    return _ArrayWriteBarrier<Policy>::WriteBarrier(address, count);
}

// Copy array content. Triggers write barrier on the dst array content if if
// Allocator and element type determines write barrier is needed.
//
template <class T, class PolicyType = T>
void CopyArray(T* dst, size_t dstCount, const T* src, size_t srcCount)
{
    js_memcpy_s(reinterpret_cast<void*>(dst), sizeof(T) * dstCount,
                reinterpret_cast<const void*>(src), sizeof(T) * srcCount);
    ArrayWriteBarrier<T, PolicyType>(dst, dstCount);
}
template <class T, class PolicyType = T>
void CopyArray(WriteBarrierPtr<T>& dst, size_t dstCount,
               const WriteBarrierPtr<T>& src, size_t srcCount)
{
    return CopyArray<T, PolicyType>(
        static_cast<T*>(dst), dstCount, static_cast<const T*>(src), srcCount);
}
template <class T, class PolicyType = T>
void CopyArray(T* dst, size_t dstCount,
               const WriteBarrierPtr<T>& src, size_t srcCount)
{
    return CopyArray<T, PolicyType>(
        dst, dstCount, static_cast<const T*>(src), srcCount);
}
template <class T, class PolicyType = T>
void CopyArray(WriteBarrierPtr<T>& dst, size_t dstCount,
               const T* src, size_t srcCount)
{
    return CopyArray<T, PolicyType>(
        static_cast<T*>(dst), dstCount, src, srcCount);
}

// Copy pointer array to WriteBarrierPtr array
//
template <class T, class PolicyType = WriteBarrierPtr<T>>
void CopyArray(WriteBarrierPtr<T>* dst, size_t dstCount,
               T* const * src, size_t srcCount)
{
    CompileAssert(sizeof(WriteBarrierPtr<T>) == sizeof(T*));
    return CopyArray<T*, PolicyType>(
        reinterpret_cast<T**>(dst), dstCount, src, srcCount);
}

// Move Array content (memmove)
//
template <class T, class PolicyType = T>
void MoveArray(T* dst, const T* src, size_t count)
{
    memmove(reinterpret_cast<void*>(dst),
            reinterpret_cast<const void*>(src),
            sizeof(T) * count);
    ArrayWriteBarrier<T, PolicyType>(dst, count);
}

template <class T>
void ClearArray(T* dst, size_t count)
{
    // assigning NULL don't need write barrier, just cast it and null it out
    memset(reinterpret_cast<void*>(dst), 0, sizeof(T) * count);
}
template <class T>
void ClearArray(WriteBarrierPtr<T>& dst, size_t count)
{
    ClearArray(static_cast<T*>(dst), count);
}


template <typename T>
class NoWriteBarrierField
{
public:
    NoWriteBarrierField() {}
    explicit NoWriteBarrierField(T const& value) : value(value) {}

    // Getters
    operator T const&() const { return value; }
    operator T&() { return value; }

    T const* operator&() const { return &value; }
    T* operator&() { return &value; }

    // Setters
    NoWriteBarrierField& operator=(T const& value)
    {
#if ENABLE_DEBUG_CONFIG_OPTIONS
        RecyclerWriteBarrierManager::VerifyIsNotBarrierAddress(this);
#endif
        this->value = value;
        return *this;
    }

private:
    T value;
};

template <typename T>
class NoWriteBarrierPtr
{
public:
    NoWriteBarrierPtr() {}
    NoWriteBarrierPtr(T * value) : value(value) {}

    // Getters
    T * operator->() const { return this->value; }
    operator T* const & () const { return this->value; }

    T* const * operator&() const { return &value; }
    T** operator&() { return &value; }

    // Setters
    NoWriteBarrierPtr& operator=(T * value)
    {
#if ENABLE_DEBUG_CONFIG_OPTIONS
        RecyclerWriteBarrierManager::VerifyIsNotBarrierAddress(this);
#endif
        this->value = value;
        return *this;
    }
private:
    T * value;
};

template <typename T>
class WriteBarrierObjectConstructorTrigger
{
public:
    WriteBarrierObjectConstructorTrigger(T* object, Recycler* recycler):
        object((char*) object),
        recycler(recycler)
    {
    }

    ~WriteBarrierObjectConstructorTrigger()
    {
        // WriteBarrier-TODO: trigger write barrier if the GC is in concurrent mark state
    }

    operator T*()
    {
        return object;
    }

private:
    T* object;
    Recycler* recycler;
};

template <typename T>
class WriteBarrierPtr
{
public:
    WriteBarrierPtr() {}
    WriteBarrierPtr(T * ptr)
    {
        // WriteBarrier
        NoWriteBarrierSet(ptr);
    }

    // Getters
    T * operator->() const { return ptr; }
    operator T* const & () const { return ptr; }

    const WriteBarrierPtr* AddressOf() const { return this; }
    WriteBarrierPtr* AddressOf() { return this; }

    // Taking immutable address is ok
    //
    T* const * operator&() const
    {
        return &ptr;
    }
    // Taking mutable address is not allowed
    //
    // T** operator&()
    // {
    //     static_assert(false, "Might need to set barrier for this operation, and use AddressOf instead.");
    //     return &ptr;
    // }

    // Setters
    WriteBarrierPtr& operator=(T * ptr)
    {
        WriteBarrierSet(ptr);
        return *this;
    }
    WriteBarrierPtr& operator=(const std::nullptr_t& ptr)
    {
        NoWriteBarrierSet(ptr);
        return *this;
    }
    void NoWriteBarrierSet(T * ptr)
    {
        this->ptr = ptr;
    }
    void WriteBarrierSet(T * ptr)
    {
        NoWriteBarrierSet(ptr);
#ifdef RECYCLER_WRITE_BARRIER
        RecyclerWriteBarrierManager::WriteBarrier(this);
#endif
    }

    WriteBarrierPtr& operator=(WriteBarrierPtr const& other)
    {
        WriteBarrierSet(other.ptr);
        return *this;
    }

    WriteBarrierPtr& operator++()  // prefix ++
    {
        ++ptr;
#ifdef RECYCLER_WRITE_BARRIER
        RecyclerWriteBarrierManager::WriteBarrier(this);
#endif
        return *this;
    }

    WriteBarrierPtr operator++(int)  // postfix ++
    {
        WriteBarrierPtr result(*this);
        ++(*this);
        return result;
    }

    static void MoveArray(WriteBarrierPtr * dst, WriteBarrierPtr * src, size_t count)
    {
        memmove((void *)dst, src, sizeof(WriteBarrierPtr) * count);
        WriteBarrier(dst, count);
    }
    static void CopyArray(WriteBarrierPtr * dst, size_t dstCount, T const* src, size_t srcCount)
    {
        js_memcpy_s((void *)dst, sizeof(WriteBarrierPtr) * dstCount, src, sizeof(T *) * srcCount);
        WriteBarrier(dst, dstCount);
    }
    static void CopyArray(WriteBarrierPtr * dst, size_t dstCount, WriteBarrierPtr const* src, size_t srcCount)
    {
        js_memcpy_s((void *)dst, sizeof(WriteBarrierPtr) * dstCount, src, sizeof(WriteBarrierPtr) * srcCount);
        WriteBarrier(dst, dstCount);
    }
    static void ClearArray(WriteBarrierPtr * dst, size_t count)
    {
        // assigning NULL don't need write barrier, just cast it and null it out
        memset((void *)dst, 0, sizeof(WriteBarrierPtr<T>) * count);
    }
private:
    T * ptr;
};


template <class T>
struct _AddressOfType
{
    inline static T* AddressOf(T& val) { return &val; }
    inline static const T* AddressOf(const T& val) { return &val; }
};

template <class T>
struct _AddressOfType< WriteBarrierPtr<T> >
{
    inline static WriteBarrierPtr<T>* AddressOf(WriteBarrierPtr<T>& val)
    {
        return val.AddressOf();
    }
    inline static const WriteBarrierPtr<T>* AddressOf(const WriteBarrierPtr<T>& val)
    {
        return val.AddressOf();
    }
};

template <class T>
inline T* AddressOf(T& val) { return _AddressOfType<T>::AddressOf(val); }
template <class T>
inline const T* AddressOf(const T& val) { return _AddressOfType<T>::AddressOf(val); }
}  // namespace Memory


template<class T> inline
const T& min(const T& a, const NoWriteBarrierField<T>& b) { return a < b ? a : b; }

template<class T> inline
const T& min(const NoWriteBarrierField<T>& a, const T& b) { return a < b ? a : b; }

template<class T> inline
const T& min(const NoWriteBarrierField<T>& a, const NoWriteBarrierField<T>& b) { return a < b ? a : b; }

template<class T> inline
const T& max(const NoWriteBarrierField<T>& a, const T& b) { return a > b ? a : b; }

// TODO: Add this method back once we figure out why OACR is tripping on it
template<class T> inline
const T& max(const T& a, const NoWriteBarrierField<T>& b) { return a > b ? a : b; }

template<class T> inline
const T& max(const NoWriteBarrierField<T>& a, const NoWriteBarrierField<T>& b) { return a > b ? a : b; }


// Disallow memcpy, memmove of WriteBarrierPtr

template <typename T>
void *  __cdecl memmove(_Out_writes_bytes_all_opt_(_Size) WriteBarrierPtr<T> * _Dst, _In_reads_bytes_opt_(_Size) const void * _Src, _In_ size_t _Size)
{
    CompileAssert(false);
}

template <typename T>
void __stdcall js_memcpy_s(__bcount(sizeInBytes) WriteBarrierPtr<T> *dst, size_t sizeInBytes, __bcount(count) const void *src, size_t count)
{
    CompileAssert(false);
}

template <typename T>
void *  __cdecl memset(_Out_writes_bytes_all_(_Size) WriteBarrierPtr<T> * _Dst, _In_ int _Val, _In_ size_t _Size)
{
    CompileAssert(false);
}

#include <Memory/WriteBarrierMacros.h>
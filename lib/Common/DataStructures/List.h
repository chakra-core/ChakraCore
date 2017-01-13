//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    template <class TListType, bool clearOldEntries>
    class CopyRemovePolicy;

    template <typename TListType, bool clearOldEntries>
    class FreeListedRemovePolicy;

    template <typename TListType, bool clearOldEntries>
    class WeakRefFreeListedRemovePolicy;
};

namespace JsUtil
{
    template <
        class T,
        class TAllocator = Recycler,
        template <typename Value> class TComparer = DefaultComparer>
    class ReadOnlyList
    {
    public:
        typedef TComparer<T> TComparerType;

    protected:
        Field(Field(T, TAllocator) *, TAllocator) buffer;
        Field(int) count;
        FieldNoBarrier(TAllocator*) alloc;

        ReadOnlyList(TAllocator* alloc)
            : buffer(nullptr),
            count(0),
            alloc(alloc)
        {
        }

    public:
        virtual bool IsReadOnly() const
        {
            return true;
        }

        virtual void Delete()
        {
            AllocatorDelete(TAllocator, alloc, this);
        }

        const T* GetBuffer() const
        {
            return AddressOf(this->buffer[0]);
        }

        template<class TList>
        bool Equals(TList list)
        {
            CompileAssert(sizeof(T) == sizeof(*list->GetBuffer()));
            return list->Count() == this->Count()
                && memcmp(this->buffer, list->GetBuffer(), sizeof(T)* this->Count()) == 0;
        }

        template<class TAllocator>
        static ReadOnlyList * New(TAllocator* alloc, __in_ecount(count) T* buffer, DECLSPEC_GUARD_OVERFLOW int count)
        {
            return AllocatorNew(TAllocator, alloc, ReadOnlyList, buffer, count, alloc);
        }

        ReadOnlyList(__in_ecount(count) T* buffer, int count, TAllocator* alloc)
            : buffer(buffer),
            count(count),
            alloc(alloc)
        {
        }

        virtual ~ReadOnlyList()
        {
        }

        int Count() const
        {
            return count;
        }

        bool Empty() const
        {
            return Count() == 0;
        }

        // Gets the count of items using the specified criteria for considering an item.
        template <typename TConditionalFunction>
        int CountWhere(TConditionalFunction function) const
        {
            int conditionalCount = 0;
            for (int i = 0; i < this->count; ++i)
            {
                if (function(this->buffer[i]))
                {
                    ++conditionalCount;
                }
            }

            return conditionalCount;
        }

        const T& Item(int index) const
        {
            Assert(index >= 0 && index < count);
            return buffer[index];
        }

        bool Contains(const T& item) const
        {
            for (int i = 0; i < count; i++)
            {
                if (TComparerType::Equals(item, buffer[i]))
                {
                    return true;
                }
            }
            return false;
        }

        // Checks if any of the elements satisfy the condition in the passed in function.
        template <typename TConditionalFunction>
        bool Any(TConditionalFunction function)
        {
            for (int i = 0; i < count; ++i)
            {
                if (function(this->buffer[i]))
                {
                    return true;
                }
            }

            return false;
        }

        // Checks if all of the elements satisfy the condition in the passed in function.
        template <typename TConditionalFunction>
        bool All(TConditionalFunction function)
        {
            for (int i = 0; i < count; ++i)
            {
                if (!function(this->buffer[i]))
                {
                    return false;
                }
            }

            return true;
        }

        // Performs a binary search on a range of elements in the list (assumes the list is sorted).
        template <typename TComparisonFunction>
        int BinarySearch(TComparisonFunction compare, int fromIndex, int toIndex)
        {
            AssertMsg(fromIndex >= 0, "Invalid starting index for binary searching.");
            AssertMsg(toIndex < this->count, "Invalid ending index for binary searching.");

            while (fromIndex <= toIndex)
            {
                int midIndex = fromIndex + (toIndex - fromIndex) / 2;
                T item = this->Item(midIndex);
                int compareResult = compare(item, midIndex);
                if (compareResult > 0)
                {
                    toIndex = midIndex - 1;
                }
                else if (compareResult < 0)
                {
                    fromIndex = midIndex + 1;
                }
                else
                {
                    return midIndex;
                }
            }
            return -1;
        }

        // Performs a binary search on the elements in the list (assumes the list is sorted).
        template <typename TComparisonFunction>
        int BinarySearch(TComparisonFunction compare)
        {
            return BinarySearch<TComparisonFunction>(compare, 0, this->Count() - 1);
        }
    };

    template <
        class T,
        class TAllocator = Recycler,
        bool isLeaf = false,
        template <class TListType, bool clearOldEntries> class TRemovePolicy = Js::CopyRemovePolicy,
        template <typename Value> class TComparer = DefaultComparer>
    class List : public ReadOnlyList<T, TAllocator, TComparer>
    {
    public:
        typedef ReadOnlyList<T, TAllocator, TComparer> ParentType;
        typedef typename ParentType::TComparerType TComparerType;
        typedef T TElementType;         // For TRemovePolicy
        static const int DefaultIncrement = 4;

    private:
        typedef List<T, TAllocator, isLeaf, TRemovePolicy, TComparer> TListType;
        friend TRemovePolicy<TListType, true>;
        typedef TRemovePolicy<TListType, true /* clearOldEntries */>  TRemovePolicyType;
        typedef ListTypeAllocatorFunc<TAllocator, isLeaf> AllocatorInfo;
        typedef typename AllocatorInfo::EffectiveAllocatorType EffectiveAllocatorType;

        Field(int) length;
        Field(int) increment;
        Field(TRemovePolicyType) removePolicy;

        Field(T, TAllocator) * AllocArray(DECLSPEC_GUARD_OVERFLOW int size)
        {
            typedef Field(T, TAllocator) TField;
            return AllocatorNewArrayBaseFuncPtr(TAllocator, this->alloc, AllocatorInfo::GetAllocFunc(), TField, size);
        }

        void FreeArray(Field(T, TAllocator) * oldBuffer, int oldBufferSize)
        {
            AllocatorFree(this->alloc, AllocatorInfo::GetFreeFunc(), oldBuffer, oldBufferSize);
        }

        PREVENT_COPY(List); // Disable copy constructor and operator=

    public:
        virtual bool IsReadOnly() const override
        {
            return false;
        }

        virtual void Delete() override
        {
            AllocatorDelete(TAllocator, this->alloc, this);
        }

        void EnsureArray()
        {
            EnsureArray(0);
        }

        void EnsureArray(DECLSPEC_GUARD_OVERFLOW int32 requiredCapacity)
        {
            if (this->buffer == nullptr)
            {
                int32 newSize = max(requiredCapacity, increment);

                this->buffer = AllocArray(newSize);
                this->count = 0;
                this->length = newSize;
            }
            else if (this->count == length || requiredCapacity > this->length)
            {
                int32 newLength = 0, newBufferSize = 0, oldBufferSize = 0;

                if (Int32Math::Add(length, 1u, &newLength)
                    || Int32Math::Shl(newLength, 1u, &newLength))
                {
                    JsUtil::ExternalApi::RaiseOnIntOverflow();
                }

                newLength = max(requiredCapacity, newLength);

                if (Int32Math::Mul(sizeof(T), newLength, &newBufferSize)
                    || Int32Math::Mul(sizeof(T), length, &oldBufferSize))
                {
                    JsUtil::ExternalApi::RaiseOnIntOverflow();
                }

                Field(T, TAllocator)* newbuffer = AllocArray(newLength);
                Field(T, TAllocator)* oldbuffer = this->buffer;
                CopyArray<Field(T, TAllocator), Field(T, TAllocator), EffectiveAllocatorType>(
                    newbuffer, newLength, oldbuffer, length);

                FreeArray(oldbuffer, oldBufferSize);

                this->length = newLength;
                this->buffer = newbuffer;
            }
        }

        template<class T>
        void Copy(const T* list)
        {
            CompileAssert(sizeof(TElementType) == sizeof(typename T::TElementType));
            if (list->Count() > 0)
            {
                this->EnsureArray(list->Count());
                js_memcpy_s(this->buffer, UInt32Math::Mul(sizeof(TElementType), this->length), list->GetBuffer(), UInt32Math::Mul(sizeof(TElementType), list->Count()));
            }
            this->count = list->Count();
        }

        static List * New(TAllocator * alloc, int increment = DefaultIncrement)
        {
            return AllocatorNew(TAllocator, alloc, List, alloc, increment);
        }

        List(TAllocator* alloc, int increment = DefaultIncrement) :
            increment(increment), removePolicy(this), ParentType(alloc)
        {
            this->buffer = nullptr;
            this->count = 0;
            length = 0;
        }

        virtual ~List() override
        {
            this->Reset();
        }

        TAllocator * GetAllocator() const
        {
            return this->alloc;
        }

        const T& Item(int index) const
        {
            return ParentType::Item(index);
        }

        Field(T, TAllocator)& Item(int index)
        {
            Assert(index >= 0 && index < this->count);
            return this->buffer[index];
        }

        T& Last()
        {
            Assert(this->count >= 1);
            return this->Item(this->count - 1);
        }

        // Finds the last element that satisfies the condition in the passed in function.
        // Returns true if the element was found; false otherwise.
        template <typename TConditionalFunction>
        bool Last(TConditionalFunction function, T& outElement)
        {
            for (int i = count - 1; i >= 0; --i)
            {
                if (function(this->buffer[i]))
                {
                    outElement = this->buffer[i];
                    return true;
                }
            }

            return false;
        }

        void Item(int index, const T& item)
        {
            Assert(index >= 0 && index < this->count);
            this->buffer[index] = item;
        }

        void SetItem(int index, const T& item)
        {
            EnsureArray(index + 1);
            // TODO: (SWB)(leish) find a way to force user defined copy constructor
            this->buffer[index] = item;
            this->count = max(this->count, index + 1);
        }

        void SetExistingItem(int index, const T& item)
        {
            Item(index, item);
        }

        bool IsItemValid(int index)
        {
            return removePolicy.IsItemValid(this, index);
        }

        int SetAtFirstFreeSpot(const T& item)
        {
            int indexToSetAt = removePolicy.GetFreeItemIndex(this);

            if (indexToSetAt == -1)
            {
                return Add(item);
            }

            this->buffer[indexToSetAt] = item;
            return indexToSetAt;
        }

        int Add(const T& item)
        {
            EnsureArray();
            this->buffer[this->count] = item;
            int pos = this->count;
            this->count++;
            return pos;
        }

        int32 AddRange(__readonly _In_reads_(count) const T* items, int32 count)
        {
            Assert(items != nullptr);
            Assert(count > 0);

            int32 requiredSize = 0, availableByteSpace = 0, givenBufferSize = 0;

            if (Int32Math::Add(this->count,  count, &requiredSize))
            {
                JsUtil::ExternalApi::RaiseOnIntOverflow();
            }

            EnsureArray(requiredSize);

            if (Int32Math::Sub(this->length,  this->count, &availableByteSpace)
                || Int32Math::Mul(sizeof(T), availableByteSpace, &availableByteSpace)
                || Int32Math::Mul(sizeof(T), count, &givenBufferSize))
            {
                JsUtil::ExternalApi::RaiseOnIntOverflow();
            }

            js_memcpy_s(buffer + this->count, availableByteSpace, items, givenBufferSize);
            this->count = requiredSize;

            return requiredSize; //Returns count
        }


        void AddRange(TListType const& list)
        {
            list.Map([this](int index, T const& item)
            {
                this->Add(item);
            });
        }

        // Trims the end of the list
        template <bool weaklyRefItems>
        T CompactEnd()
        {
            while (this->count != 0)
            {
                AnalysisAssert(!weaklyRefItems || (this->buffer[this->count - 1] != nullptr));
                if (weaklyRefItems ?
                    this->buffer[this->count - 1]->Get() != nullptr :
                    this->buffer[this->count - 1] != nullptr)
                {
                    return this->buffer[this->count - 1];
                }
                this->count--;
                this->buffer[this->count] = nullptr;
            }

            return nullptr;
        }

        void Remove(const T& item)
        {
            removePolicy.Remove(this, item);
        }

        T RemoveAtEnd()
        {
            Assert(this->count >= 1);
            T item = this->Item(this->count - 1);
            RemoveAt(this->count - 1);
            return item;
        }

        void RemoveAt(int index)
        {
            removePolicy.RemoveAt(this, index);
        }

        void Clear()
        {
            this->count = 0;
        }

        void ClearAndZero()
        {
            if(this->count == 0)
            {
                return;
            }

            ClearArray(this->buffer, this->count);
            Clear();
        }

        void Sort()
        {
            Sort([](void *, const void * a, const void * b) {
                return TComparerType::Compare(*(T*)a, *(T*)b);
            }, nullptr);
        }

        void Sort(int(__cdecl * _PtFuncCompare)(void *, const void *, const void *), void *_Context)
        {
            // We can call QSort only if the remove policy for this list is CopyRemovePolicy
            CompileAssert((IsSame<TRemovePolicyType, Js::CopyRemovePolicy<TListType, false> >::IsTrue) ||
                (IsSame<TRemovePolicyType, Js::CopyRemovePolicy<TListType, true> >::IsTrue));
            if (this->count)
            {
                qsort_s<Field(T, TAllocator), Field(T, TAllocator), EffectiveAllocatorType>(
                    this->buffer, this->count, _PtFuncCompare, _Context);
            }
        }

        template<class DebugSite, class TMapFunction>
        HRESULT Map(DebugSite site, TMapFunction map) const // external debugging version
        {
            return Js::Map(site, PointerValue(this->buffer), this->count, map);
        }

        template<class TMapFunction>
        bool MapUntil(TMapFunction map) const
        {
            return MapUntilFrom(0, map);
        }

        template<class TMapFunction>
        bool MapUntilFrom(int start, TMapFunction map) const
        {
            for (int i = start; i < this->count; i++)
            {
                if (TRemovePolicyType::IsItemValid(this->buffer[i]))
                {
                    if (map(i, this->buffer[i]))
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        template<class TMapFunction>
        void Map(TMapFunction map) const
        {
            MapFrom(0, map);
        }

        template<class TMapFunction>
        void MapAddress(TMapFunction map) const
        {
            for (int i = 0; i < this->count; i++)
            {
                if (TRemovePolicyType::IsItemValid(this->buffer[i]))
                {
                    map(i, &this->buffer[i]);
                }
            }
        }

        template<class TMapFunction>
        void MapFrom(int start, TMapFunction map) const
        {
            for (int i = start; i < this->count; i++)
            {
                if (TRemovePolicyType::IsItemValid(this->buffer[i]))
                {
                    map(i, this->buffer[i]);
                }
            }
        }

        template<class TMapFunction>
        void ReverseMap(TMapFunction map)
        {
            for (int i = this->count - 1; i >= 0; i--)
            {
                if (TRemovePolicyType::IsItemValid(this->buffer[i]))
                {
                    map(i, this->buffer[i]);
                }
            }
        }

        void Reset()
        {
            if (this->buffer != nullptr)
            {
                auto freeFunc = AllocatorInfo::GetFreeFunc();
                AllocatorFree(this->alloc, freeFunc, this->buffer, sizeof(T) * length); // TODO: Better version of DeleteArray?

                this->buffer = nullptr;
                this->count = 0;
                length = 0;
            }
        }
    };

}

namespace Js
{
    //
    // A simple wrapper on List to synchronize access.
    // Note that this wrapper class only exposes a few methods of List (through "private" inheritance).
    // It applies proper lock policy to exposed methods.
    //
    template <
        class T,                                    // Item type in the list
        class ListType,
        class LockPolicy = DefaultContainerLockPolicy,   // Controls lock policy for read/map/write/add/remove items
        class SyncObject = CriticalSection
    >
    class SynchronizableList sealed: private ListType // Make base class private to lock down exposed methods
    {
    private:
        FieldNoBarrier(SyncObject*) syncObj;

    public:
        template <class Arg1>
        SynchronizableList(Arg1 arg1, SyncObject* syncObj)
            : ListType(arg1), syncObj(syncObj)
        {
        }

        template <class Arg1, class Arg2>
        SynchronizableList(Arg1 arg1, Arg2 arg2, SyncObject* syncObj)
            : ListType(arg1, arg2), syncObj(syncObj)
        {
        }

        template <class Arg1, class Arg2, class Arg3>
        SynchronizableList(Arg1 arg1, Arg2 arg2, Arg3 arg3, SyncObject* syncObj)
            : ListType(arg1, arg2, arg3), syncObj(syncObj)
        {
        }

        int Count() const
        {
            typename LockPolicy::ReadLock autoLock(syncObj);
            return __super::Count();
        }

        const T& Item(int index) const
        {
            typename LockPolicy::ReadLock autoLock(syncObj);
            return __super::Item(index);
        }

        void Item(int index, const T& item)
        {
            typename LockPolicy::WriteLock autoLock(syncObj);
            __super::Item(index, item);
        }

        void SetExistingItem(int index, const T& item)
        {
            typename LockPolicy::WriteLock autoLock(syncObj);
            __super::SetExistingItem(index, item);
        }

        bool IsItemValid(int index)
        {
            typename LockPolicy::ReadLock autoLock(syncObj);
            return __super::IsItemValid(index);
        }

        int SetAtFirstFreeSpot(const T& item)
        {
            typename LockPolicy::WriteLock autoLock(syncObj);
            return __super::SetAtFirstFreeSpot(item);
        }

        void ClearAndZero()
        {
            typename LockPolicy::WriteLock autoLock(syncObj);
            __super::ClearAndZero();
        }

        void RemoveAt(int index)
        {
            typename LockPolicy::AddRemoveLock autoLock(syncObj);
            return __super::RemoveAt(index);
        }

        int Add(const T& item)
        {
            typename LockPolicy::AddRemoveLock autoLock(syncObj);
            return __super::Add(item);
        }

        template<class TMapFunction>
        void Map(TMapFunction map) const
        {
            typename LockPolicy::ReadLock autoLock(syncObj);
            __super::Map(map);
        }

        template<class TMapFunction>
        bool MapUntil(TMapFunction map) const
        {
            typename LockPolicy::ReadLock autoLock(syncObj);
            return __super::MapUntil(map);
        }

        template<class DebugSite, class TMapFunction>
        HRESULT Map(DebugSite site, TMapFunction map) const // external debugging version
        {
            // No lock needed. Threads are suspended during external debugging.
            return __super::Map(site, map);
        }
    };

    template <typename TListType, bool clearOldEntries = false>
    class CopyRemovePolicy
    {
        typedef typename TListType::TElementType TElementType;
        typedef typename TListType::TComparerType TComparerType;

    public:
        CopyRemovePolicy(TListType * list) {};
        void Remove(TListType* list, const TElementType& item)
        {
            TElementType* buffer = list->buffer;
            int& count = list->count;

            for (int i = 0; i < count; i++)
            {
                if (TComparerType::Equals(buffer[i], item))
                {
                    for (int j = i + 1; j < count; i++, j++)
                    {
                        buffer[i] = buffer[j];
                    }
                    count--;

                    if (clearOldEntries)
                    {
                        ClearArray(buffer + count, 1);
                    }
                    break;
                }
            }
        }

        int GetFreeItemIndex(TListType* list)
        {
            return -1;
        }

        void RemoveAt(TListType* list, int index)
        {
            Assert(index >= 0 && index < list->count);
            for (int j = index + 1; j < list->count; index++, j++)
            {
                list->buffer[index] = list->buffer[j];
            }
            list->count--;

            if (clearOldEntries)
            {
                ClearArray(list->buffer + list->count, 1);
            }
        }

        static bool IsItemValid(const TElementType& item)
        {
            return true;
        }

        bool IsItemValid(TListType* list, int index)
        {
            Assert(index >= 0 && index < list->count);
            return true;
        }
    };

    template <typename TListType, bool clearOldEntries = false>
    class FreeListedRemovePolicy
    {
    protected:
        typedef typename TListType::TElementType TElementType;
        typedef typename TListType::TComparerType TComparerType;

        Field(int) freeItemIndex;

    public:
        FreeListedRemovePolicy(TListType * list):
          freeItemIndex(-1)
        {
            CompileAssert(IsPointer<TElementType>::IsTrue);
        }

        static bool IsItemValid(const TElementType& item)
        {
            return (item != nullptr && (::Math::PointerCastToIntegralTruncate<unsigned int>(item) & 1) == 0);
        }

        bool IsItemValid(TListType* list, int index)
        {
            const TElementType& item = list->Item(index);
            return IsItemValid(item);
        }

        void Remove(TListType* list, const TElementType& item)
        {
            TElementType* buffer = list->buffer;
            int& count = list->count;

            for (int i = 0; i < count; i++)
            {
                if (TComparerType::Equals(buffer[i], item))
                {
                    RemoveAt(list, i);
                    break;
                }
            }
        }

        int GetFreeItemIndex(TListType* list)
        {
            int currentFreeIndex = this->freeItemIndex;
            if (currentFreeIndex != -1)
            {
                unsigned int nextFreeIndex = ::Math::PointerCastToIntegralTruncate<unsigned int>(list->Item(currentFreeIndex));

                if (nextFreeIndex != ((unsigned int) -1))
                {
                    // Since this is an unsigned shift, the sign bit is 0, which is what we want
                    this->freeItemIndex = (int) ((nextFreeIndex) >> 1);
                }
                else
                {
                    this->freeItemIndex = -1;
                }

                return currentFreeIndex;
            }

            return -1;
        }

        void RemoveAt(TListType* list, int index)
        {
            Assert(index >= 0 && index < list->Count());
            Assert(IsItemValid(list, index));

            unsigned int storedIndex = (unsigned int) this->freeItemIndex;

            // Sentinel value, so leave that as is
            // Otherwise, this has the range of all +ve integers
            if (this->freeItemIndex != -1)
            {
                // Set a tag bit to indicate this is a free list index, rather than a list value
                // Pointers will be aligned anyway
                storedIndex = (storedIndex << 1) | 1;
            }

            list->SetExistingItem(index, (TElementType) (storedIndex));
            this->freeItemIndex = index;
        }
    };

    template <typename TListType, bool clearOldEntries = false>
    class WeakRefFreeListedRemovePolicy : public FreeListedRemovePolicy<TListType, clearOldEntries>
    {
        typedef FreeListedRemovePolicy<TListType, clearOldEntries> Base;
        typedef typename Base::TElementType TElementType;
    private:
        Field(uint) lastWeakReferenceCleanupId;

        void CleanupWeakReference(TListType * list)
        {
            list->Map([list](int i, TElementType weakRef)
            {
                if (weakRef->Get() == nullptr)
                {
                    list->RemoveAt(i);
                }
            });

            this->lastWeakReferenceCleanupId = list->alloc->GetWeakReferenceCleanupId();
        }
    public:
        WeakRefFreeListedRemovePolicy(TListType * list) : Base(list)
        {
            this->lastWeakReferenceCleanupId = list->alloc->GetWeakReferenceCleanupId();
        }
        int GetFreeItemIndex(TListType * list)
        {
            if (list->alloc->GetWeakReferenceCleanupId() != this->lastWeakReferenceCleanupId)
            {
                CleanupWeakReference(list);
            }
            return __super::GetFreeItemIndex(list);
        }
    };
}

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace JsUtil
{
    interface IWeakReferenceDictionary
    {
        virtual void Cleanup() = 0;
    };

    template <
        class TKey,
        class TValue,
        class SizePolicy = PowerOf2SizePolicy,
        template <typename ValueOrKey> class Comparer = DefaultComparer
    >
    class WeakReferenceDictionary: public BaseDictionary<TKey, RecyclerWeakReference<TValue>*, RecyclerNonLeafAllocator, SizePolicy, Comparer, WeakRefValueDictionaryEntry>,
                                   public IWeakReferenceDictionary
    {
        typedef BaseDictionary<TKey, RecyclerWeakReference<TValue>*, RecyclerNonLeafAllocator, SizePolicy, Comparer, WeakRefValueDictionaryEntry> Base;
        typedef typename Base::EntryType EntryType;

    public:
        WeakReferenceDictionary(Recycler* recycler, int capacity = 0):
          Base(recycler, capacity)
        {
            Assert(reinterpret_cast<void*>(this) == reinterpret_cast<void*>((IWeakReferenceDictionary*) this));
        }

        virtual void Cleanup() override
        {
            this->MapAndRemoveIf([](typename Base::EntryType &entry)
            {
                return (Base::EntryType::NeedsCleanup(entry));
            });
        }

    private:
        using Base::Clone;
        using Base::Copy;

        PREVENT_COPY(WeakReferenceDictionary);
    };

#if ENABLE_WEAK_REFERENCE_REGIONS

    template <class TKey>
    class WeakRefRegionValueDictionaryEntry : public SimpleDictionaryKeyEntry<TKey>
    {
    public:
        void Clear()
        {
            this->value = TKey();
        }
    };

    // TODO: It would be good to adapt WeaklyReferencedKeyDictionary to also use WeakRefRegions
    //       One possibility is to create a BaseSplitDictionary which has the collections of
    //       buckets, entries, and RecyclerWeakReferenceRegionItems, and then the entries are
    //       either value/next or key/next pairs, with the weak ref region storing the keys or
    //       values in a weak manner. 
    template <
        class TKey,
        class TValue,
        class SizePolicy = PowerOf2SizePolicy,
        template <typename ValueOrKey> class Comparer = DefaultComparer,
        typename Lock = NoResizeLock,
        class AllocType = Recycler // Should always be recycler; this is to sufficiently confuse the RecyclerChecker
    >
    class WeakReferenceRegionDictionary : protected Lock, public IWeakReferenceDictionary
    {
        typedef WeakRefRegionValueDictionaryEntry<TKey> EntryType;
        typedef RecyclerWeakReferenceRegionItem<TValue> ValueType;

        typedef typename AllocatorInfo<Recycler, TValue>::AllocatorFunc EntryAllocatorFuncType;

    private:
        Field(int*) buckets;
        Field(EntryType*) entries;
        Field(ValueType*) values;
        FieldNoBarrier(Recycler*) alloc;
        Field(int) size;
        Field(uint) bucketCount;
        Field(int) count;
        Field(int) freeList;
        Field(int) freeCount;
        Field(int) modFunctionIndex;

        static const int FreeListSentinel = -2;

        PREVENT_COPY(WeakReferenceRegionDictionary);

        enum InsertOperations
        {
            Insert_Add,          // FatalInternalError if the item already exist in debug build
            Insert_AddNew,          // Ignore add if the item already exist
            Insert_Item             // Replace the item if it already exist
        };

        class AutoDoResize
        {
        public:
            AutoDoResize(Lock& lock) : lock(lock) { lock.BeginResize(); };
            ~AutoDoResize() { lock.EndResize(); };
        private:
            Lock & lock;
        };

    public:

        virtual void Cleanup() override
        {
            this->MapAndRemoveIf([](EntryType& entry, ValueType& value)
            {
                return value == nullptr;
            });
        }

        WeakReferenceRegionDictionary(Recycler* allocator, int capacity = 0)
            : buckets(nullptr),
            entries(nullptr),
            values(nullptr),
            alloc(allocator),
            size(0),
            bucketCount(0),
            count(0),
            freeList(0),
            freeCount(0),
            modFunctionIndex(UNKNOWN_MOD_INDEX)
        {
            Assert(reinterpret_cast<void*>(this) == reinterpret_cast<void*>((IWeakReferenceDictionary*)this));
            Assert(allocator);

            // If initial capacity is negative or 0, lazy initialization on
            // the first insert operation is performed.
            if (capacity > 0)
            {
                Initialize(capacity);
            }
        }

        inline int Capacity() const
        {
            return size;
        }

        inline int Count() const
        {
            return count - freeCount;
        }

        TValue Item(const TKey& key)
        {
            int i = FindEntry(key);
            Assert(i >= 0);
            return values[i];
        }

        const TValue Item(const TKey& key) const
        {
            int i = FindEntry(key);
            Assert(i >= 0);
            return values[i];
        }

        int Add(const TKey& key, const TValue& value)
        {
            return Insert<Insert_Add>(key, value);
        }

        int AddNew(const TKey& key, const TValue& value)
        {
            return Insert<Insert_AddNew>(key, value);
        }

        int Item(const TKey& key, const TValue& value)
        {
            return Insert<Insert_Item>(key, value);
        }

        bool Contains(KeyValuePair<TKey, TValue> keyValuePair)
        {
            int i = FindEntry(keyValuePair.Key());
            if (i >= 0 && Comparer<TValue>::Equals(values[i], keyValuePair.Value()))
            {
                return true;
            }
            return false;
        }

        bool Remove(KeyValuePair<TKey, TValue> keyValuePair)
        {
            int i, last;
            uint targetBucket;
            if (FindEntryWithKey(keyValuePair.Key(), &i, &last, &targetBucket))
            {
                const TValue &value = values[i];
                if (Comparer<TValue>::Equals(value, keyValuePair.Value()))
                {
                    RemoveAt(i, last, targetBucket);
                    return true;
                }
            }
            return false;
        }

        void Clear()
        {
            if (count > 0)
            {
                memset(buckets, -1, bucketCount * sizeof(buckets[0]));
                memset(entries, 0, sizeof(EntryType) * size);
                memset(values, 0, sizeof(ValueType) * size); // TODO: issues with background threads?
                count = 0;
                freeCount = 0;
            }
        }

        void ResetNoDelete()
        {
            this->size = 0;
            this->bucketCount = 0;
            this->buckets = nullptr;
            this->entries = nullptr;
            this->values = nullptr;
            this->count = 0;
            this->freeCount = 0;
            this->modFunctionIndex = UNKNOWN_MOD_INDEX;
        }

        void Reset()
        {
            ResetNoDelete();
        }

        bool ContainsKey(const TKey& key) const
        {
            return FindEntry(key) >= 0;
        }

        template <typename TLookup>
        inline const TValue& LookupWithKey(const TLookup& key, const TValue& defaultValue) const
        {
            int i = FindEntryWithKey(key);
            if (i >= 0)
            {
                return values[i];
            }
            return defaultValue;
        }

        inline const TValue& Lookup(const TKey& key, const TValue& defaultValue) const
        {
            return LookupWithKey<TKey>(key, defaultValue);
        }

        template <typename TLookup>
        bool TryGetValue(const TLookup& key, TValue* value) const
        {
            int i = FindEntryWithKey(key);
            if (i >= 0)
            {
                *value = values[i];
                return true;
            }
            return false;
        }


        bool TryGetValueAndRemove(const TKey& key, TValue* value)
        {
            int i, last;
            uint targetBucket;
            if (FindEntryWithKey(key, &i, &last, &targetBucket))
            {
                *value = values[i];
                RemoveAt(i, last, targetBucket);
                return true;
            }
            return false;
        }

        const TValue& GetValueAt(const int index) const
        {
            Assert(index >= 0);
            Assert(index < count);

            return values[index];
        }

        TKey const& GetKeyAt(const int index) const
        {
            Assert(index >= 0);
            Assert(index < count);

            return entries[index].Key();
        }

        bool TryGetValueAt(int index, TValue * value) const
        {
            if (index >= 0 && index < count)
            {
                *value = values[index];
                return true;
            }
            return false;
        }

        bool Remove(const TKey& key)
        {
            int i, last;
            uint targetBucket;
            if (FindEntryWithKey(key, &i, &last, &targetBucket))
            {
                RemoveAt(i, last, targetBucket);
                return true;
            }
            return false;
        }

        // Returns whether the dictionary was resized or not
        bool EnsureCapacity()
        {
            if (freeCount == 0 && count == size)
            {
                Resize();
                return true;
            }

            return false;
        }

        int GetNextIndex()
        {
            if (freeCount != 0)
            {
                Assert(freeCount > 0);
                Assert(freeList >= 0);
                Assert(freeList < count);
                return freeList;
            }

            return count;
        }

        int GetLastIndex()
        {
            return count - 1;
        }

        void LockResize()
        {
            __super::LockResize();
        }

        void UnlockResize()
        {
            __super::UnlockResize();
        }

    private:

        template <typename TLookup>
        static hash_t GetHashCodeWithKey(const TLookup& key)
        {
            // set last bit to 1 to avoid false positive to make hash appears to be a valid recycler address.
            // In the same line, 0 should be use to indicate a non-existing entry.
            return TAGHASH(Comparer<TLookup>::GetHashCode(key));
        }

        static hash_t GetHashCode(const TKey& key)
        {
            return GetHashCodeWithKey<TKey>(key);
        }

        static uint GetBucket(hash_t hashCode, int bucketCount, int modFunctionIndex)
        {
            return SizePolicy::GetBucket(UNTAGHASH(hashCode), bucketCount, modFunctionIndex);
        }

        uint GetBucket(hash_t hashCode) const
        {
            return GetBucket(hashCode, this->bucketCount, modFunctionIndex);
        }

        static bool IsFreeEntry(const EntryType &entry)
        {
            // A free entry's next index will be (FreeListSentinel - nextIndex), such that it is always <= FreeListSentinel, for fast entry iteration
            // allowing for skipping over free entries. -1 is reserved for the end-of-chain marker for a used entry.
            return entry.next <= FreeListSentinel;
        }


        void SetNextFreeEntryIndex(EntryType &freeEntry, const int nextFreeEntryIndex)
        {
            Assert(!IsFreeEntry(freeEntry));
            Assert(nextFreeEntryIndex >= -1);
            Assert(nextFreeEntryIndex < count);

            // The last entry in the free list chain will have a next of FreeListSentinel to indicate that it is a free entry. The end of the
            // free list chain is identified using freeCount.
            freeEntry.next = nextFreeEntryIndex >= 0 ? FreeListSentinel - nextFreeEntryIndex : FreeListSentinel;
        }

        static int GetNextFreeEntryIndex(const EntryType &freeEntry)
        {
            Assert(IsFreeEntry(freeEntry));
            return FreeListSentinel - freeEntry.next;
        }

        template <typename LookupType>
        inline int FindEntryWithKey(const LookupType& key) const
        {
            int * localBuckets = buckets;
            if (localBuckets != nullptr)
            {
                hash_t hashCode = GetHashCodeWithKey<LookupType>(key);
                uint targetBucket = this->GetBucket(hashCode);
                EntryType * localEntries = entries;
                for (int i = localBuckets[targetBucket]; i >= 0; i = localEntries[i].next)
                {
                    if (localEntries[i].template KeyEquals<Comparer<TKey>>(key, hashCode))
                    {
                        return i;
                    }

                }
            }

            return -1;
        }

        inline int FindEntry(const TKey& key) const
        {
            return FindEntryWithKey<TKey>(key);
        }

        template <typename LookupType>
        inline bool FindEntryWithKey(const LookupType& key, int *const i, int *const last, uint *const targetBucket)
        {
            int * localBuckets = buckets;
            if (localBuckets != nullptr)
            {
                hash_t hashCode = GetHashCodeWithKey<LookupType>(key);
                *targetBucket = this->GetBucket(hashCode);
                *last = -1;
                EntryType * localEntries = entries;
                for (*i = localBuckets[*targetBucket]; *i >= 0; *last = *i, *i = localEntries[*i].next)
                {
                    if (localEntries[*i].template KeyEquals<Comparer<TKey>>(key, hashCode))
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        template<class Fn>
        void MapAndRemoveIf(Fn fn)
        {
            for (uint i = 0; i < bucketCount; i++)
            {
                if (buckets[i] != -1)
                {
                    for (int currentIndex = buckets[i], lastIndex = -1; currentIndex != -1;)
                    {
                        // If the predicate says we should remove this item
                        if (fn(entries[currentIndex], values[currentIndex]) == true)
                        {
                            const int nextIndex = entries[currentIndex].next;
                            RemoveAt(currentIndex, lastIndex, i);
                            currentIndex = nextIndex;
                        }
                        else
                        {
                            lastIndex = currentIndex;
                            currentIndex = entries[currentIndex].next;
                        }
                    }
                }
            }
        }

        void Initialize(int capacity)
        {
            // minimum capacity is 4
            int initSize = max(capacity, 4);
            int modIndex = UNKNOWN_MOD_INDEX;
            uint initBucketCount = SizePolicy::GetBucketSize(initSize, &modIndex);
            AssertMsg(initBucketCount > 0, "Size returned by policy should be greater than 0");

            int* newBuckets = nullptr;
            EntryType* newEntries = nullptr;
            ValueType* newValues = nullptr;
            Allocate(&newBuckets, &newEntries, &newValues, initBucketCount, initSize);

            // Allocation can throw - assign only after allocation has succeeded.
            this->buckets = newBuckets;
            this->entries = newEntries;
            this->values = newValues;
            this->bucketCount = initBucketCount;
            this->size = initSize;
            this->modFunctionIndex = modIndex;
            Assert(this->freeCount == 0);

        }


        template <InsertOperations op>
        int Insert(const TKey& key, const TValue& value)
        {
            int * localBuckets = buckets;
            if (localBuckets == nullptr)
            {
                Initialize(0);
                localBuckets = buckets;
            }

#if DBG
            // Always search and verify
            const bool needSearch = true;
#else
            const bool needSearch = (op != Insert_Add);
#endif
            hash_t hashCode = GetHashCode(key);
            uint targetBucket = this->GetBucket(hashCode);
            if (needSearch)
            {
                EntryType * localEntries = entries;
                for (int i = localBuckets[targetBucket]; i >= 0; i = localEntries[i].next)
                {
                    if (localEntries[i].template KeyEquals<Comparer<TKey>>(key, hashCode))
                    {
                        Assert(op != Insert_Add);
                        if (op == Insert_Item)
                        {
                            values[i] = value;
                            return i;
                        }
                        return -1;
                    }
                }
            }

            // Ideally we'd do cleanup only if weak references have been collected since the last resize
            // but that would require us to use an additional field to store the last recycler cleanup id
            // that we saw
            // We can add that optimization later if we have to.
            if (freeCount == 0 && count == size)
            {
                this->Cleanup();
            }

            int index;
            if (freeCount != 0)
            {
                Assert(freeCount > 0);
                Assert(freeList >= 0);
                Assert(freeList < count);
                index = freeList;
                freeCount--;
                if (freeCount != 0)
                {
                    freeList = GetNextFreeEntryIndex(entries[index]);
                }
            }
            else
            {
                // If there's nothing free, then in general, we set index to count, and increment count
                // If we resize, we also need to recalculate the target
                // However, if cleanup is supported, then before resize, we should try and clean up and see
                // if something got freed, and if it did, reuse that index
                if (count == size)
                {
                    Resize();
                    targetBucket = this->GetBucket(hashCode);
                    index = count;
                    count++;
                }
                else
                {
                    index = count;
                    count++;
                }

                Assert(count <= size);
                Assert(index < size);
            }

            entries[index].Set(key, hashCode);
            values[index] = value;
            entries[index].next = buckets[targetBucket];
            buckets[targetBucket] = index;

            return index;
        }

        void Resize()
        {
            AutoDoResize autoDoResize(*this);

            int newSize = SizePolicy::GetNextSize(count);
            int modIndex = UNKNOWN_MOD_INDEX;
            uint newBucketCount = SizePolicy::GetBucketSize(newSize, &modIndex);

            __analysis_assume(newSize > count);
            int* newBuckets = nullptr;
            EntryType* newEntries = nullptr;
            ValueType* newValues = nullptr;
            if (newBucketCount == bucketCount)
            {
                // no need to rehash
                newEntries = AllocateEntries(newSize);
                newValues = AllocateValues(newSize);
                CopyArray<EntryType>(newEntries, newSize, entries, count);
                CopyArray<ValueType>(newValues, newSize, values, count); // TODO: concurrency issues?

                this->entries = newEntries;
                this->values = newValues;
                this->size = newSize;
                this->modFunctionIndex = modIndex;
                return;
            }

            Allocate(&newBuckets, &newEntries, &newValues, newBucketCount, newSize);
            CopyArray<EntryType>(newEntries, newSize, entries, count);
            CopyArray<ValueType>(newValues, newSize, values, count); // TODO: concurrency issues?

            // When TAllocator is of type Recycler, it is possible that the Allocate above causes a collection, which
            // in turn can cause entries in the dictionary to be removed - i.e. the dictionary contains weak references
            // that remove themselves when no longer valid. This means the free list might not be empty anymore.
            this->modFunctionIndex = modIndex;
            for (int i = 0; i < count; i++)
            {
                __analysis_assume(i < newSize);

                if (!IsFreeEntry(newEntries[i]))
                {
                    hash_t hashCode = newEntries[i].template GetHashCode<Comparer<TKey>>();
                    int bucket = GetBucket(hashCode, newBucketCount, modFunctionIndex);
                    newEntries[i].next = newBuckets[bucket];
                    newBuckets[bucket] = i;
                }
            }

            this->buckets = newBuckets;
            this->entries = newEntries;
            this->values = newValues;
            bucketCount = newBucketCount;
            size = newSize;
        }

        __ecount(bucketCount) int *AllocateBuckets(DECLSPEC_GUARD_OVERFLOW const uint bucketCount)
        {
            return
                AllocateArray<AllocType, int, false>(
                    TRACK_ALLOC_INFO(alloc, int, AllocType, 0, bucketCount),
                    TypeAllocatorFunc<AllocType, int>::GetAllocFunc(),
                    bucketCount);
        }

        __ecount(size) EntryType * AllocateEntries(DECLSPEC_GUARD_OVERFLOW int size, const bool zeroAllocate = true)
        {
            // Note that the choice of leaf/non-leaf node is decided for the EntryType on the basis of TValue. By default, if
            // TValue is a pointer, a non-leaf allocation is done. This behavior can be overridden by specializing
            // TypeAllocatorFunc for TValue.
            return
                AllocateArray<Recycler, EntryType, false>(
                    TRACK_ALLOC_INFO(alloc, EntryType, Recycler, 0, size),
                    zeroAllocate ? EntryAllocatorFuncType::GetAllocZeroFunc() : EntryAllocatorFuncType::GetAllocFunc(),
                    size);
        }

        __ecount(size) ValueType * AllocateValues(DECLSPEC_GUARD_OVERFLOW int size)
        {
            return alloc->CreateWeakReferenceRegion<TValue>(size);
        }


        void Allocate(__deref_out_ecount(bucketCount) int** ppBuckets, __deref_out_ecount(size) EntryType** ppEntries, __deref_out_ecount(size) ValueType** ppValues, DECLSPEC_GUARD_OVERFLOW uint bucketCount, DECLSPEC_GUARD_OVERFLOW int size)
        {
            int *const buckets = AllocateBuckets(bucketCount);
            Assert(buckets); // no-throw allocators are currently not supported

            EntryType *entries;
            entries = AllocateEntries(size);
            Assert(entries); // no-throw allocators are currently not supported

            ValueType * values = AllocateValues(size);

            memset(buckets, -1, bucketCount * sizeof(buckets[0]));

            *ppBuckets = buckets;
            *ppEntries = entries;
            *ppValues = values;
        }

        inline void RemoveAt(const int i, const int last, const uint targetBucket)
        {
            if (last < 0)
            {
                buckets[targetBucket] = entries[i].next;
            }
            else
            {
                entries[last].next = entries[i].next;
            }
            entries[i].Clear();
            SetNextFreeEntryIndex(entries[i], freeCount == 0 ? -1 : freeList);
            freeList = i;
            freeCount++;
        }
    };
#endif
};

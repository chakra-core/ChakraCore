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

    template<bool, typename T = void>
    struct enable_if {};

    template<typename T>
    struct enable_if<true, T>
    {
        typedef T type;
    };

    template <
        class TEntry,
        class TWeak,
        bool keyIsWeak,
        class SizePolicy = PowerOf2SizePolicy,
        template <typename ValueOrKey> class Comparer = DefaultComparer,
        typename Lock = NoResizeLock,
        class AllocType = Recycler // Should always be recycler; this is to sufficiently confuse the RecyclerChecker
    >
    class SplitWeakDictionary : protected Lock, public IWeakReferenceDictionary
    {
    private:
        template <typename T1, typename T2, bool second>
        struct Select;
        
        template <typename T1, typename T2>
        struct Select<T1, T2, false>
        {
            typedef T1 type;
        };

        template <typename T1, typename T2>
        struct Select<T1, T2, true>
        {
            typedef T2 type;
        };

        typedef typename Select<TEntry, TWeak, keyIsWeak>::type TBKey;
        typedef typename Select<TEntry, TWeak, !keyIsWeak>::type TBValue;

        typedef typename AllocatorInfo<Recycler, TEntry>::AllocatorFunc EntryAllocatorFuncType;

        enum InsertOperations
        {
            Insert_Add,          // FatalInternalError if the item already exist in debug build
            Insert_AddNew,          // Ignore add if the item already exist
            Insert_Item             // Replace the item if it already exist
        };

    protected:

        typedef ValueEntry<TEntry> EntryType;
        typedef RecyclerWeakReferenceRegionItem<TWeak> WeakType;

        Field(int*) buckets;
        Field(EntryType*) entries;
        Field(WeakType*) weakRefs;
        FieldNoBarrier(Recycler*) alloc;
        Field(int) size;
        Field(uint) bucketCount;
        Field(int) count;
        Field(int) freeList;
        Field(int) freeCount;
        Field(int) modFunctionIndex;

    private:
        static const int FreeListSentinel = -2;

        PREVENT_COPY(SplitWeakDictionary);


        class AutoDoResize
        {
        public:
            AutoDoResize(Lock& lock) : lock(lock) { lock.BeginResize(); };
            ~AutoDoResize() { lock.EndResize(); };
        private:
            Lock & lock;
        };

    public:
        // Allow SplitWeakDictionary field to be inlined in classes with DEFINE_VTABLE_CTOR_MEMBER_INIT
        SplitWeakDictionary(VirtualTableInfoCtorEnum) { }

        SplitWeakDictionary(Recycler* allocator, int capacity = 0)
            : buckets(nullptr),
            entries(nullptr),
            weakRefs(nullptr),
            alloc(allocator),
            size(0),
            bucketCount(0),
            count(0),
            freeList(0),
            freeCount(0),
            modFunctionIndex(UNKNOWN_MOD_INDEX)
        {
            Assert(allocator);
            Assert(reinterpret_cast<void*>(this) == reinterpret_cast<void*>((IWeakReferenceDictionary*)this));

            // If initial capacity is negative or 0, lazy initialization on
            // the first insert operation is performed.
            if (capacity > 0)
            {
                Initialize(capacity);
            }
        }

        virtual void Cleanup() override
        {
            this->MapAndRemoveIf([](EntryType& entry, WeakType& weakRef)
            {
                return weakRef == nullptr;
            });
        }

        inline int Capacity() const
        {
            return size;
        }

        inline int Count() const
        {
            return count - freeCount;
        }

        void Clear()
        {
            if (count > 0)
            {
                memset(buckets, -1, bucketCount * sizeof(buckets[0]));
                memset(entries, 0, sizeof(EntryType) * size);
                memset(weakRefs, 0, sizeof(WeakType) * size); // TODO: issues with background threads?
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
            this->weakRefs = nullptr;
            this->count = 0;
            this->freeCount = 0;
            this->modFunctionIndex = UNKNOWN_MOD_INDEX;
        }

        void Reset()
        {
            ResetNoDelete();
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

        TBValue Item(const TBKey& key)
        {
            int i = FindEntry(key);
            Assert(i >= 0);
            return values[i];
        }

        TBValue Item(const TBKey& key) const
        {
            int i = FindEntry(key);
            Assert(i >= 0);
            return values[i];
        }

        int Add(const TBKey& key, const TBValue& value)
        {
            return Insert<Insert_Add>(key, value);
        }

        int AddNew(const TBKey& key, const TBValue& value)
        {
            return Insert<Insert_AddNew>(key, value);
        }

        int Item(const TBKey& key, const TBValue& value)
        {
            return Insert<Insert_Item>(key, value);
        }

        bool Contains(KeyValuePair<TBKey, TBValue> keyValuePair)
        {
            int i = FindEntry(keyValuePair.Key());
            TBValue val = this->GetValue(i);
            if (i >= 0 && Comparer<TBValue>::Equals(val, keyValuePair.Value()))
            {
                return true;
            }
            return false;
        }

        bool ContainsKey(const TBKey& key) const
        {
            return FindEntry(key) >= 0;
        }

        template <typename TLookup>
        inline const TBValue& LookupWithKey(const TLookup& key, const TBValue& defaultValue) 
        {
            int i = FindEntryWithKey(key);
            if (i >= 0)
            {
                return this->GetValue(i);
            }
            return defaultValue;
        }

        inline const TBValue& Lookup(const TBKey& key, const TBValue& defaultValue) 
        {
            return LookupWithKey<TBKey>(key, defaultValue);
        }

        template <typename TLookup>
        bool TryGetValue(const TLookup& key, TBValue* value) 
        {
            int i = FindEntryWithKey(key);
            if (i >= 0)
            {
                *value = this->GetValue(i);
                return true;
            }
            return false;
        }


        bool TryGetValueAndRemove(const TBKey& key, TBValue* value)
        {
            int i, last;
            uint targetBucket;
            if (FindEntryWithKey(key, &i, &last, &targetBucket))
            {
                *value = this->GetValue(i);
                RemoveAt(i, last, targetBucket);
                return true;
            }
            return false;
        }

        bool Remove(const TBKey& key)
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

        bool Remove(KeyValuePair<TBKey, TBValue> keyValuePair)
        {
            int i, last;
            uint targetBucket;
            if (FindEntryWithKey(keyValuePair.Key(), &i, &last, &targetBucket))
            {
                const TBValue& value = this->GetValue(i);
                if (Comparer<TBValue>::Equals(value, keyValuePair.Value()))
                {
                    RemoveAt(i, last, targetBucket);
                    return true;
                }
            }
            return false;
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

    protected:

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
                        if (fn(entries[currentIndex], weakRefs[currentIndex]) == true)
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

        template <typename TLookup>
        static hash_t GetHashCodeWithKey(const TLookup& key)
        {
            // set last bit to 1 to avoid false positive to make hash appears to be a valid recycler address.
            // In the same line, 0 should be use to indicate a non-existing entry.
            return TAGHASH(Comparer<TLookup>::GetHashCode(key));
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

        void Initialize(int capacity)
        {
            // minimum capacity is 4
            int initSize = max(capacity, 4);
            int modIndex = UNKNOWN_MOD_INDEX;
            uint initBucketCount = SizePolicy::GetBucketSize(initSize, &modIndex);
            AssertMsg(initBucketCount > 0, "Size returned by policy should be greater than 0");

            int* newBuckets = nullptr;
            EntryType* newEntries = nullptr;
            WeakType* newWeakRefs = nullptr;
            Allocate(&newBuckets, &newEntries, &newWeakRefs, initBucketCount, initSize);

            // Allocation can throw - assign only after allocation has succeeded.
            this->buckets = newBuckets;
            this->entries = newEntries;
            this->weakRefs = newWeakRefs;
            this->bucketCount = initBucketCount;
            this->size = initSize;
            this->modFunctionIndex = modIndex;
            Assert(this->freeCount == 0);
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
            weakRefs[i].Clear();
            SetNextFreeEntryIndex(entries[i], freeCount == 0 ? -1 : freeList);
            freeList = i;
            freeCount++;
        }

        inline int FindEntry(const TBKey& key)
        {
            return FindEntryWithKey<TBKey>(key);
        }

        template <typename LookupType>
        inline int FindEntryWithKey(const LookupType& key)
        {
            int * localBuckets = buckets;
            if (localBuckets != nullptr)
            {
                hash_t hashCode = GetHashCodeWithKey<LookupType>(key);
                uint targetBucket = this->GetBucket(hashCode);
                EntryType * localEntries = entries;
                for (int i = localBuckets[targetBucket], last = -1; i >= 0;)
                {
                    TBKey k = this->GetKey(i);
                    if (this->RemoveIfCollected(k, &i, last, targetBucket))
                    {
                        continue;
                    }
                    if (Comparer<TBKey>::Equals(k, key))
                    {
                        return i;
                    }

                    last = i;
                    i = localEntries[i].next;
                }
            }
            return -1;
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
                for (*i = localBuckets[*targetBucket]; *i >= 0;)
                {
                    TBKey k = this->GetKey(*i);
                    if (this->RemoveIfCollected(k, i, *last, *targetBucket))
                    {
                        continue;
                    }
                    if (Comparer<TBKey>::Equals(k, key))
                    {
                        return true;
                    }

                    *last = *i;
                    *i = localEntries[*i].next;
                }
            }
            return false;
        }

    private:

        void Resize()
        {
            AutoDoResize autoDoResize(*this);
            __analysis_assert(count > 1);

            int newSize = SizePolicy::GetNextSize(count);
            int modIndex = UNKNOWN_MOD_INDEX;
            uint newBucketCount = SizePolicy::GetBucketSize(newSize, &modIndex);

            __analysis_assume(newSize > count);
            int* newBuckets = nullptr;
            EntryType* newEntries = nullptr;
            WeakType* newWeakRefs = nullptr;
            if (newBucketCount == bucketCount)
            {
                // no need to rehash
                newEntries = AllocateEntries(newSize);
                newWeakRefs = AllocateWeakRefs(newSize);
                CopyArray<EntryType>(newEntries, newSize, entries, count);
                CopyArray<WeakType>(newWeakRefs, newSize, weakRefs, count); // TODO: concurrency issues?

                this->entries = newEntries;
                this->weakRefs = newWeakRefs;
                this->size = newSize;
                this->modFunctionIndex = modIndex;
                return;
            }

            Allocate(&newBuckets, &newEntries, &newWeakRefs, newBucketCount, newSize);

            this->modFunctionIndex = modIndex;

            // Need to re-bucket the entries
            // Also need to account for whether the weak refs have been collected
            // Have to go in order so we can remove entries as appropriate
            this->count = 0;
            for (uint i = 0; i < this->bucketCount; ++i)
            {
                if (this->buckets[i] < 0)
                {
                    continue;
                }

                for (int currentEntry = this->buckets[i]; currentEntry != -1; )
                {
                    if (IsFreeEntry(entries[currentEntry]))
                    {
                        // Entry is free; this shouldn't happen, but stop following the chain
                        AssertMsg(false, "Following bucket chains should not result in free entries");
                        break;
                    }
                    if (this->weakRefs[currentEntry] == nullptr)
                    {
                        // The weak ref has been collected; don't bother bringing it to the new collection
                        currentEntry = this->entries[currentEntry].next;
                        continue;
                    }

                    hash_t hashCode = GetHashCodeWithKey<TBKey>(this->GetKey(currentEntry));
                    int bucket = GetBucket(hashCode, newBucketCount, modFunctionIndex);
                    newEntries[count].next = newBuckets[bucket];
                    newEntries[count].SetValue(this->entries[currentEntry].Value());
                    newWeakRefs[count] = this->weakRefs[currentEntry];
                    newBuckets[bucket] = count;
                    ++count;

                    currentEntry = this->entries[currentEntry].next;
                }
            }
            this->freeCount = 0;
            this->freeList = 0;
            this->buckets = newBuckets;
            this->entries = newEntries;
            this->weakRefs = newWeakRefs;
            this->bucketCount = newBucketCount;
            this->size = newSize;

        }

        template <InsertOperations op>
        int Insert(const TBKey& key, const TBValue& value)
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
            hash_t hashCode = GetHashCodeWithKey<TBKey>(key);
            uint targetBucket = this->GetBucket(hashCode);
            if (needSearch)
            {
                EntryType * localEntries = entries;
                for (int i = localBuckets[targetBucket], last = -1; i >= 0;)
                {
                    TBKey k = this->GetKey(i);
                    if (this->RemoveIfCollected(k, &i, last, targetBucket))
                    {
                        continue;
                    }

                    if (Comparer<TBKey>::Equals(k, key))
                    {
                        Assert(op != Insert_Add);
                        if (op == Insert_Item)
                        {
                            SetValue(value, i);
                            return i;
                        }
                        return -1;
                    }

                    last = i;
                    i = localEntries[i].next;
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

            SetKeyValue(key, value, index);
            entries[index].next = buckets[targetBucket];
            buckets[targetBucket] = index;

            return index;
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

        __ecount(size) WeakType * AllocateWeakRefs(DECLSPEC_GUARD_OVERFLOW int size)
        {
            return alloc->CreateWeakReferenceRegion<TWeak>(size);
        }


        void Allocate(__deref_out_ecount(bucketCount) int** ppBuckets, __deref_out_ecount(size) EntryType** ppEntries, __deref_out_ecount(size) WeakType** ppWeakRefs, DECLSPEC_GUARD_OVERFLOW uint bucketCount, DECLSPEC_GUARD_OVERFLOW int size)
        {
            int *const newBuckets = AllocateBuckets(bucketCount);
            Assert(newBuckets); // no-throw allocators are currently not supported

            EntryType *newEntries = AllocateEntries(size);
            Assert(newEntries); // no-throw allocators are currently not supported

            WeakType * newWeakRefs = AllocateWeakRefs(size);

            memset(newBuckets, -1, bucketCount * sizeof(newBuckets[0]));

            *ppBuckets = newBuckets;
            *ppEntries = newEntries;
            *ppWeakRefs = newWeakRefs;
        }

        template <bool weakKey = keyIsWeak>
        inline typename enable_if<weakKey, TBKey>::type GetKey(int index) const
        {
            return this->weakRefs[index];
        }

        template <bool weakKey = keyIsWeak>
        inline typename enable_if<!weakKey, TBKey>::type GetKey(int index) const
        {
            return this->entries[index].Value();
        }

        template <bool weakKey = keyIsWeak>
        inline typename enable_if<weakKey, TBValue>::type GetValue(int index) const
        {
            return this->entries[index].Value();
        }

        template <bool weakKey = keyIsWeak>
        inline typename enable_if<!weakKey, TBValue>::type GetValue(int index) const
        {
            return this->weakRefs[index];
        }

        template <bool weakKey = keyIsWeak>
        inline typename enable_if<weakKey, bool>::type RemoveIfCollected(TBKey const key, int* i, int last, int targetBucket)
        {
            if (key == nullptr)
            {
                // Key has been collected
                int next = entries[*i].next;
                RemoveAt(*i, last, targetBucket);
                *i = next;
                return true;
            }
            return false;
        }

        template <bool weakKey = keyIsWeak>
        inline typename enable_if<!weakKey, bool>::type RemoveIfCollected(TBKey const key, int* i, int last, int targetBucket)
        {
            return false;
        }

        template <bool weakKey = keyIsWeak>
        inline typename enable_if<weakKey, void>::type SetKeyValue(const TBKey& key, const TBValue& value, int index)
        {
            weakRefs[index] = key;
            entries[index].SetValue(value);
        }

        template <bool weakKey = keyIsWeak>
        inline typename enable_if<!weakKey, void>::type SetKeyValue(const TBKey& key, const TBValue& value, int index)
        {
            entries[index].SetValue(key);
            weakRefs[index] = value;
        }

        template <bool weakKey = keyIsWeak>
        inline typename enable_if<weakKey, void>::type SetValue(const TBValue& value, int index)
        {
            this->entries[index].SetValue(value);
        }

        template <bool weakKey = keyIsWeak>
        inline typename enable_if<!weakKey, void>::type SetValue(const TBValue& value, int index)
        {
            this->weakRefs[index] = value;
        }
    };

    template <
        class TKey,
        class TValue,
        class SizePolicy = PowerOf2SizePolicy,
        template <typename ValueOrKey> class Comparer = DefaultComparer,
        typename Lock = NoResizeLock
    >
    class WeakReferenceRegionDictionary : public SplitWeakDictionary<TKey, TValue, false, SizePolicy, Comparer, Lock, Recycler>
    {
        typedef SplitWeakDictionary<TKey, TValue, false, SizePolicy, Comparer, Lock, Recycler> Base;
        typedef typename Base::EntryType EntryType;
        typedef typename Base::WeakType ValueType;

    public:

        WeakReferenceRegionDictionary(Recycler* allocator, int capacity = 0)
            : Base(allocator, capacity)
        {
        }
    };

    template <
        class TKey,
        class TValue,
        template <typename ValueOrKey> class Comparer = DefaultComparer,
        class SizePolicy = PrimeSizePolicy,
        typename Lock = NoResizeLock
    >
    class WeakReferenceRegionKeyDictionary : public SplitWeakDictionary<TValue, TKey, true, SizePolicy, Comparer, Lock, Recycler>
    {
        typedef SplitWeakDictionary<TValue, TKey, true, SizePolicy, Comparer, Lock, Recycler> Base;

        typedef typename Base::EntryType EntryType;
        typedef typename Base::WeakType KeyType;

    public:
        // Allow WeakReferenceRegionKeyDictionary field to be inlined in classes with DEFINE_VTABLE_CTOR_MEMBER_INIT
        WeakReferenceRegionKeyDictionary(VirtualTableInfoCtorEnum v): Base(v) { }

        WeakReferenceRegionKeyDictionary(Recycler* allocator, int capacity = 0)
            : Base(allocator, capacity)
        {
        }

        template <class Fn>
        void Map(Fn fn)
        {
            this->MapAndRemoveIf([=](EntryType& entry, KeyType& weakRef)
            {
                if (weakRef == nullptr)
                {
                    return true;
                }
                fn(weakRef, entry.Value(), weakRef);
                return false;
            });
        }

        bool Clean()
        {
            this->Cleanup();
            return this->freeCount > 0;
        }
    };
#endif // ENABLE_WEAK_REFERENCE_REGIONS
};

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class TinyDictionary
    {
        static const int PowerOf2_BUCKETS = 16;
        static const int BUCKETS_DWORDS = PowerOf2_BUCKETS / sizeof(DWORD);
        static const byte NIL = 0xff;

        Field(DWORD) bucketsData[BUCKETS_DWORDS];  // use DWORDs to enforce alignment
        Field(byte) next[0];

public:
        TinyDictionary()
        {
            CompileAssert(BUCKETS_DWORDS * sizeof(DWORD) == PowerOf2_BUCKETS);
            CompileAssert(BUCKETS_DWORDS == 4);
            DWORD* init = bucketsData;
            init[0] = init[1] = init[2] = init[3] =0xffffffff;
        }

        uint32 ReduceKeyToIndex(PropertyId key)
        {
            // we use 4-bit bucket index, but we often have keys that are larger. 
            // use Fibonacci hash to reduce the possibility of collisions
#if TARGET_64
            return (key * 11400714819323198485llu) >> 60;
#else
            return (key * 2654435769ul) >> 28;
#endif
        }

        void Add(PropertyId key, byte value)
        {
            Assert(value < 128);

            byte* buckets = reinterpret_cast<byte*>(bucketsData);
            uint32 bucketIndex = ReduceKeyToIndex(key);

            byte i = buckets[bucketIndex];

            // if the bucket was empty (NIL), put a tagged value to indicate the end of the chain.
            // NB: [OS:17745531] In extreme rare cases 127th pid to insert hashes into a a bucket still unused by the previous 126 values.
            //     We cannot tag 127 since it would become NIL and we would lose the value. 
            //     We can however chain 127 --> NIL, since it is the same as having two 127 in the bucket, which is ok.
            if ((i == NIL) & (value != 127))
            {
                // set the highest bit to mark the value as the last in the chain
                buckets[bucketIndex] = value | 128;
            }
            else
            {
                buckets[bucketIndex] = value;
                next[value] = i;
            }
        }

        // Template shared with diagnostics
        template <class Data>
        inline bool TryGetValue(PropertyId key, PropertyIndex* index, const Data& data)
        {
            byte* buckets = reinterpret_cast<byte*>(bucketsData);
            uint32 bucketIndex = ReduceKeyToIndex(key);

            byte i = buckets[bucketIndex];
            if (i != NIL)
            {
                for(;;)
                {                    
                    byte idx = i & 127; // strip the sentinel bit

                    if (data[idx]->GetPropertyId() == key)
                    {
                        *index = idx;
                        return true;
                    }

                    if (i & 128)
                    {
                        // this was the last value in the chain
                        break;
                    }

                    Assert(next[idx] == NIL || (next[idx] & 127) != idx);
                    i = next[idx];
                }
            }

            return false;
        }
    };

    class TypePath
    {
        friend class PathTypeHandlerBase;
        friend class SimplePathTypeHandlerWithAttr;
        friend class PathTypeHandlerWithAttr;

    public:
        // This is the space between the end of the TypePath and the allocation granularity that can be used for assignments too.
#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
#if defined(TARGET_64)
#define TYPE_PATH_ALLOC_GRANULARITY_GAP 0
#else
#define TYPE_PATH_ALLOC_GRANULARITY_GAP 2
#endif
#else
#if defined(TARGET_64)
#define TYPE_PATH_ALLOC_GRANULARITY_GAP 1
#else
#define TYPE_PATH_ALLOC_GRANULARITY_GAP 3
#endif
#endif
        // Although we can allocate 2 more, 
        // TinyDictionary can hold only up to 128 items (see TinyDictionary::Add),
        // Besides 128+ would put struct Data into another bucket.
        // Just waste some slot in that case for 32-bit
        static const uint MaxPathTypeHandlerLength = 128;
        static const uint InitialTypePathSize = 16 + TYPE_PATH_ALLOC_GRANULARITY_GAP;

    private:
        struct Data
        {
            Data(uint8 pathSize) : pathSize(pathSize), pathLength(0)
#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
                , maxInitializedLength(0)
#endif
            {}

#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
            Field(BVStatic<MaxPathTypeHandlerLength>) fixedFields;
            Field(BVStatic<MaxPathTypeHandlerLength>) usedFixedFields;

            // We sometimes set up PathTypeHandlers and associate TypePaths before we create any instances
            // that populate the corresponding slots, e.g. for object literals or constructors with only
            // this statements.  This field keeps track of the longest instance associated with the given
            // TypePath.
            Field(uint8) maxInitializedLength;
#endif
            Field(uint8) pathLength;      // Entries in use
            Field(uint8) pathSize;        // Allocated entries

            // This map has to be at the end, because TinyDictionary has a zero size array
            Field(TinyDictionary) map;

            template<bool addNewId>
            int Add(const PropertyRecord* propId, Field(const PropertyRecord *)* assignments)
            {
                uint currentPathLength = this->pathLength;
                Assert(currentPathLength < this->pathSize);
                if (currentPathLength >= this->pathSize)
                {
                    Throw::InternalError();
                }

                if (addNewId)
                {
#if DBG
                    PropertyIndex temp;
                    if (this->map.TryGetValue(propId->GetPropertyId(), &temp, assignments))
                    {
                        AssertMsg(false, "Adding a duplicate to the type path");
                    }
#endif
                    this->map.Add((unsigned int)propId->GetPropertyId(), (byte)currentPathLength);
                }
                assignments[currentPathLength] = propId;
                this->pathLength++;
                return currentPathLength;
            }
        };
        Field(Data*) data;

#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
        Field(RecyclerWeakReference<DynamicObject>*) singletonInstance;
#endif

        // PropertyRecord assignments are allocated off the end of the structure
        Field(const PropertyRecord *) assignments[];


        TypePath() :
#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
            singletonInstance(nullptr),
#endif
            data(nullptr)
        {
        }

        Data * GetData() { return data; }
    public:
        static TypePath* New(Recycler* recycler, uint size = InitialTypePathSize);

        template<bool checkAttributes>
        TypePath * Branch(Recycler * recycler, int pathLength, bool couldSeeProto, ObjectSlotAttributes * attributes = nullptr)
        {
            AssertMsg(pathLength < this->GetPathLength(), "Why are we branching at the tip of the type path?");
            Assert(checkAttributes == (attributes != nullptr));

            // Ensure there is at least one free entry in the new path, so we can extend it.
            // TypePath::New will take care of aligning this appropriately.
            TypePath * branchedPath = TypePath::New(recycler, pathLength + 1);

            for (PropertyIndex i = 0; i < pathLength; i++)
            {
                if (checkAttributes && attributes[i] == ObjectSlotAttr_Setter)
                {
                    branchedPath->AddInternal<false>(assignments[i]);
                }
                else
                {
                    branchedPath->AddInternal<true>(assignments[i]);
                }

#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
                if (couldSeeProto)
                {
                    if (this->GetData()->usedFixedFields.Test(i))
                    {
                        // We must conservatively copy all used as fixed bits if some prototype instance could also take
                        // this transition.  See comment in PathTypeHandlerBase::ConvertToSimpleDictionaryType.
                        // Yes, we could devise a more efficient way of copying bits 1 through pathLength, if performance of this
                        // code path proves important enough.
                        branchedPath->GetData()->usedFixedFields.Set(i);
                    }
                    else if (this->GetData()->fixedFields.Test(i))
                    {
                        // We must clear any fixed fields that are not also used as fixed if some prototype instance could also take
                        // this transition.  See comment in PathTypeHandlerBase::ConvertToSimpleDictionaryType.
                        this->GetData()->fixedFields.Clear(i);
                    }
                }
#endif

            }

#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
            // When branching, we must ensure that fixed field values on the prefix shared by the two branches are always
            // consistent.  Hence, we can't leave any of them uninitialized, because they could later get initialized to
            // different values, by two different instances (one on the old branch and one on the new branch).  If that happened
            // and the instance from the old branch later switched to the new branch, it would magically gain a different set
            // of fixed properties!
            if (this->GetMaxInitializedLength() < pathLength)
            {
                this->SetMaxInitializedLength(pathLength);
            }
            branchedPath->SetMaxInitializedLength(pathLength);
#endif

#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
            if (PHASE_VERBOSE_TRACE1(FixMethodPropsPhase))
            {
                Output::Print(_u("FixedFields: TypePath::Branch: singleton: 0x%p(0x%p)\n"), PointerValue(this->singletonInstance), this->singletonInstance->Get());
                Output::Print(_u("   fixed fields:"));

                for (PropertyIndex i = 0; i < GetPathLength(); i++)
                {
                    Output::Print(_u(" %s %d%d%d,"), GetPropertyId(i)->GetBuffer(),
                        i < GetMaxInitializedLength() ? 1 : 0,
                        GetIsFixedFieldAt(i, GetPathLength()) ? 1 : 0,
                        GetIsUsedFixedFieldAt(i, GetPathLength()) ? 1 : 0);
                }

                Output::Print(_u("\n"));
            }
#endif

            return branchedPath;
        }

        TypePath * Grow(Recycler * alloc);

        const PropertyRecord* GetPropertyIdUnchecked(int index)
        {
            Assert(((uint)index) < ((uint)this->GetPathLength()));
            return assignments[index];
        }

        const PropertyRecord* GetPropertyId(int index)
        {
            if (((uint)index) < ((uint)this->GetPathLength()))
                return GetPropertyIdUnchecked(index);
            else
                return nullptr;
        }

        template<bool isSetter = false>
        int Add(const PropertyRecord * propertyRecord)
        {
#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
            Assert(this->GetPathLength() == this->GetMaxInitializedLength());
            this->GetData()->maxInitializedLength++;
#endif
            return AddInternal<!isSetter>(propertyRecord);
        }

        uint8 GetPathLength() { return this->GetData()->pathLength; }
        uint8 GetPathSize() { return this->GetData()->pathSize; }

        PropertyIndex Lookup(PropertyId propId,int typePathLength);
        PropertyIndex LookupInline(PropertyId propId,int typePathLength);

    private:
    template<bool addNewId>
    int AddInternal(const PropertyRecord * propId)
    {
        int propertyIndex = this->GetData()->Add<addNewId>(propId, assignments);

#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
        if (PHASE_VERBOSE_TRACE1(FixMethodPropsPhase))
        {
            Output::Print(_u("FixedFields: TypePath::AddInternal: singleton = 0x%p(0x%p)\n"),
                PointerValue(this->singletonInstance), this->singletonInstance != nullptr ? this->singletonInstance->Get() : nullptr);
            Output::Print(_u("   fixed fields:"));

            for (PropertyIndex i = 0; i < GetPathLength(); i++)
            {
                Output::Print(_u(" %s %d%d%d,"), GetPropertyId(i)->GetBuffer(),
                    i < GetMaxInitializedLength() ? 1 : 0,
                    GetIsFixedFieldAt(i, GetPathLength()) ? 1 : 0,
                    GetIsUsedFixedFieldAt(i, GetPathLength()) ? 1 : 0);
            }

            Output::Print(_u("\n"));
        }
#endif

        return propertyIndex;
    }

#if ENABLE_FIXED_FIELDS
#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
        uint8 GetMaxInitializedLength() { return this->GetData()->maxInitializedLength; }
        void SetMaxInitializedLength(int newMaxInitializedLength)
        {
            Assert(newMaxInitializedLength >= 0);
            Assert(newMaxInitializedLength <= MaxPathTypeHandlerLength);
            Assert(this->GetMaxInitializedLength() <= newMaxInitializedLength);
            this->GetData()->maxInitializedLength = (uint8)newMaxInitializedLength;
        }

        Var GetSingletonFixedFieldAt(PropertyIndex index, int typePathLength, ScriptContext * requestContext);

        bool HasSingletonInstance() const
        {
            return this->singletonInstance != nullptr;
        }

        RecyclerWeakReference<DynamicObject>* GetSingletonInstance() const
        {
            return this->singletonInstance;
        }

        void SetSingletonInstance(RecyclerWeakReference<DynamicObject>* instance, int typePathLength)
        {
            Assert(this->singletonInstance == nullptr && instance != nullptr);
            Assert(typePathLength >= this->GetMaxInitializedLength());
            this->singletonInstance = instance;
        }

        void ClearSingletonInstance()
        {
            this->singletonInstance = nullptr;
        }

        void ClearSingletonInstanceIfSame(DynamicObject* instance)
        {
            if (this->singletonInstance != nullptr && this->singletonInstance->Get() == instance)
            {
                ClearSingletonInstance();
            }
        }

        void ClearSingletonInstanceIfDifferent(DynamicObject* instance)
        {
            if (this->singletonInstance != nullptr && this->singletonInstance->Get() != instance)
            {
                ClearSingletonInstance();
            }
        }

        bool GetIsFixedFieldAt(PropertyIndex index, int typePathLength)
        {
            Assert(index < this->GetPathLength());
            Assert(index < typePathLength);
            Assert(typePathLength <= this->GetPathLength());

            return this->GetData()->fixedFields.Test(index) != 0;
        }

        bool GetIsUsedFixedFieldAt(PropertyIndex index, int typePathLength)
        {
            Assert(index < this->GetPathLength());
            Assert(index < typePathLength);
            Assert(typePathLength <= this->GetPathLength());

            return this->GetData()->usedFixedFields.Test(index) != 0;
        }

        void SetIsUsedFixedFieldAt(PropertyIndex index, int typePathLength)
        {
            Assert(index < this->GetMaxInitializedLength());
            Assert(CanHaveFixedFields(typePathLength));
            this->GetData()->usedFixedFields.Set(index);
        }

        void ClearIsFixedFieldAt(PropertyIndex index, int typePathLength)
        {
            Assert(index < this->GetMaxInitializedLength());
            Assert(index < typePathLength);
            Assert(typePathLength <= this->GetPathLength());

            this->GetData()->fixedFields.Clear(index);
            this->GetData()->usedFixedFields.Clear(index);
        }

        bool CanHaveFixedFields(int typePathLength)
        {
            // We only support fixed fields on singleton instances.
            // If the instance in question is a singleton, it must be the tip of the type path.
            return this->singletonInstance != nullptr && typePathLength >= this->GetData()->maxInitializedLength;
        }

        void AddBlankFieldAt(PropertyIndex index, int typePathLength);

        void AddSingletonInstanceFieldAt(DynamicObject* instance, PropertyIndex index, bool isFixed, int typePathLength);

        void AddSingletonInstanceFieldAt(PropertyIndex index, int typePathLength);

#if DBG
        bool HasSingletonInstanceOnlyIfNeeded();
#endif

#else
        int GetMaxInitializedLength() { Assert(false); return this->GetPathLength(); }

        Var GetSingletonFixedFieldAt(PropertyIndex index, int typePathLength, ScriptContext * requestContext);

        bool HasSingletonInstance() const { Assert(false); return false; }
        RecyclerWeakReference<DynamicObject>* GetSingletonInstance() const { Assert(false); return nullptr; }
        void SetSingletonInstance(RecyclerWeakReference<DynamicObject>* instance, int typePathLength) { Assert(false); }
        void ClearSingletonInstance() { Assert(false); }
        void ClearSingletonInstanceIfSame(DynamicObject* instance) { Assert(false); }
        void ClearSingletonInstanceIfDifferent(DynamicObject* instance) { Assert(false); }

        bool GetIsFixedFieldAt(PropertyIndex index, int typePathLength) { Assert(false); return false; }
        bool GetIsUsedFixedFieldAt(PropertyIndex index, int typePathLength) { Assert(false); return false; }
        void SetIsUsedFixedFieldAt(PropertyIndex index, int typePathLength) { Assert(false); }
        void ClearIsFixedFieldAt(PropertyIndex index, int typePathLength) { Assert(false); }
        bool CanHaveFixedFields(int typePathLength) { Assert(false); return false; }
        void AddBlankFieldAt(PropertyIndex index, int typePathLength) { Assert(false); }
        void AddSingletonInstanceFieldAt(DynamicObject* instance, PropertyIndex index, bool isFixed, int typePathLength) { Assert(false); }
        void AddSingletonInstanceFieldAt(PropertyIndex index, int typePathLength) { Assert(false); }
#if DBG
        bool HasSingletonInstanceOnlyIfNeeded();
#endif
#endif
#endif
    };
}

CompileAssert((sizeof(Js::TypePath) % HeapConstants::ObjectGranularity) == (HeapConstants::ObjectGranularity - TYPE_PATH_ALLOC_GRANULARITY_GAP * sizeof(void *)) % HeapConstants::ObjectGranularity);

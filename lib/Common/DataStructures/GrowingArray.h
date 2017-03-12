//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// Contains a class which will provide a uint32 array which can grow dynamically
// It behaves almost same as regex::List<> except it has less members, is customized for being used in SmallSpanSequence of FunctionBody


#pragma once

#ifdef DIAG_MEM
extern int listFreeAmount;
#endif

namespace JsUtil
{
    template <class ValueType, class TAllocator>
    class GrowingArray
    {
    public:
        typedef Field(ValueType, TAllocator) TValue;
        typedef typename AllocatorInfo<TAllocator, TValue>::AllocatorType AllocatorType;
        static GrowingArray* Create(uint32 _length);

        GrowingArray(AllocatorType* allocator, uint32 _length)
            : buffer(nullptr),
            alloc(allocator),
            count(0),
            length(_length)
        {
            EnsureArray();
        }

        ~GrowingArray()
        {
            if (buffer != nullptr)
            {
                AllocatorFree(alloc, (TypeAllocatorFunc<AllocatorType, int>::GetFreeFunc()), buffer, UInt32Math::Mul(length, sizeof(TValue)));
            }
        }

        TValue ItemInBuffer(uint32 index) const
        {
            if (index >= count)
            {
                return (TValue)0;
            }

            return buffer[index];
        }

        void ItemInBuffer(uint32 index, TValue item)
        {
            EnsureArray();
            Assert(index < count);
            buffer[index] = item;
        }

        void Add(TValue item)
        {
            EnsureArray();
            buffer[count] = item;
            count++;
        }

        uint32 Count() const { return count; }
        void SetCount(uint32 _count) { count = _count; }
        uint32 GetLength() const { return length; }
        TValue* GetBuffer() const { return buffer; }

        GrowingArray * Clone()
        {
            GrowingArray * pNewArray = AllocatorNew(AllocatorType, alloc, GrowingArray, alloc, length);
            pNewArray->count = count;
            if (buffer)
            {
                pNewArray->buffer = AllocateArray<AllocatorType, TValue, false>(
                    TRACK_ALLOC_INFO(alloc, TValue, AllocatorType, 0, length),
                    TypeAllocatorFunc<AllocatorType, TValue>::GetAllocFunc(),
                    length);
                CopyArray<TValue, TValue, TAllocator>(pNewArray->buffer, length, buffer, length);
            }

            return pNewArray;
        }

    private:
        Field(TValue*, TAllocator) buffer;
        Field(uint32) count;
        Field(uint32) length;
        FieldNoBarrier(AllocatorType*) alloc;

        void EnsureArray()
        {
            if (buffer == nullptr)
            {
                buffer = AllocateArray<AllocatorType, TValue, false>(
                    TRACK_ALLOC_INFO(alloc, TValue, AllocatorType, 0, length),
                    TypeAllocatorFunc<AllocatorType, TValue>::GetAllocFunc(),
                    length);
                count = 0;
            }
            else if (count == length)
            {
                uint32 newLength = UInt32Math::AddMul<1, 2>(length);
                TValue * newbuffer = AllocateArray<AllocatorType, TValue, false>(
                    TRACK_ALLOC_INFO(alloc, TValue, AllocatorType, 0, newLength),
                    TypeAllocatorFunc<AllocatorType, TValue>::GetAllocFunc(),
                    newLength);
                CopyArray<TValue, TValue, TAllocator>(newbuffer, newLength, buffer, length);
#ifdef DIAG_MEM
                listFreeAmount += length;
#endif
                if (length != 0)
                {
                    const size_t lengthByteSize = UInt32Math::Mul(length, sizeof(TValue));
                    AllocatorFree(alloc, (TypeAllocatorFunc<AllocatorType, int>::GetFreeFunc()), buffer, lengthByteSize);
                }
                length = newLength;
                buffer = newbuffer;
            }
        }
    };
    typedef GrowingArray<uint32, HeapAllocator> GrowingUint32HeapArray;
}

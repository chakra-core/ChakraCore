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
    template <class TValue, class TAllocator>
    class GrowingArray
    {
    public:
        static GrowingArray* Create(uint32 _length);

        GrowingArray(TAllocator* allocator, uint32 _length)
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
                AllocatorFree(alloc, &TAllocator::Free, buffer, UInt32Math::Mul(length, sizeof(TValue)));
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
            GrowingArray * pNewArray = AllocatorNew(TAllocator, alloc, GrowingArray, alloc, length);
            pNewArray->count = count;
            if (buffer)
            {
                pNewArray->buffer = AllocateArray<TAllocator, TValue, false>(
                    TRACK_ALLOC_INFO(alloc, TValue, TAllocator, 0, length),
                    &TAllocator::Alloc,
                    length);
                const size_t byteSize = UInt32Math::Mul(length, sizeof(TValue));
                js_memcpy_s(pNewArray->buffer, byteSize, buffer, byteSize);
            }

            return pNewArray;
        }
    private:

        TValue* buffer;
        uint32 count;
        uint32 length;
        TAllocator* alloc;

        void EnsureArray()
        {
            if (buffer == nullptr)
            {
                buffer = AllocateArray<TAllocator, TValue, false>(
                    TRACK_ALLOC_INFO(alloc, TValue, TAllocator, 0, length),
                    &TAllocator::Alloc,
                    length);
                count = 0;
            }
            else if (count == length)
            {
                uint32 newLength = UInt32Math::AddMul<1, 2>(length);
                TValue * newbuffer = AllocateArray<TAllocator, TValue, false>(
                    TRACK_ALLOC_INFO(alloc, TValue, TAllocator, 0, newLength),
                    &TAllocator::Alloc,
                    newLength);
                const size_t lengthByteSize = UInt32Math::Mul(length, sizeof(TValue));
                const size_t newLengthByteSize = UInt32Math::Mul(newLength, sizeof(TValue));
                js_memcpy_s(newbuffer, newLengthByteSize, buffer, lengthByteSize);
#ifdef DIAG_MEM
                listFreeAmount += length;
#endif
                if (length != 0)
                {
                    AllocatorFree(alloc, &TAllocator::Free, buffer, lengthByteSize);
                }
                length = newLength;
                buffer = newbuffer;
            }
        }
    };
    typedef GrowingArray<uint32, HeapAllocator> GrowingUint32HeapArray;
}

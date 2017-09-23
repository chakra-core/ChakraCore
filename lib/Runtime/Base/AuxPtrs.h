//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Js
{
    // Use fixed size structure to save pointers
    // AuxPtrsFix(16 bytes or 32 bytes) layout:
    //                        max count  metadata(init'd type)        pointers array
    //     --------------------------------------------------------------------------------------------------
    //     AuxPtr16 on x64    1 byte     1 bytes                      8 bytes to hold up to 1 pointer
    //     AuxPtr32 on x64    1 byte     3 bytes                      24 bytes to hold up to 3 pointers
    //     AuxPtr16 on x86    1 byte     3 bytes                      12 bytes to hold up to 3 pointers
    //     AuxPtr32 on x86    1 byte     6 bytes                      24 bytes to hold up to 6 pointers
    template<typename FieldsEnum, uint8 size, uint8 _MaxCount = (size - 1) / (1 + sizeof(void*))>
    struct AuxPtrsFix
    {
        static const uint8 MaxCount;
        FieldWithBarrier(uint8) count;                 // always saving maxCount
        typename FieldWithBarrier(FieldsEnum) type[_MaxCount];  // save instantiated pointer enum
        FieldWithBarrier(void*) ptr[_MaxCount];        // save instantiated pointer address
        AuxPtrsFix();
        AuxPtrsFix(AuxPtrsFix<FieldsEnum, 16>* ptr16); // called when promoting from AuxPtrs16 to AuxPtrs32
        void* Get(FieldsEnum e);
        bool Set(FieldsEnum e, void* p);
    };

    template<typename FieldsEnum, uint8 size, uint8 _MaxCount>
    const uint8 AuxPtrsFix<FieldsEnum, size, _MaxCount>::MaxCount = _MaxCount;

    // Use flexible size structure to save pointers. when pointer count exceeds AuxPtrsFix<FieldsEnum, 32>::MaxCount,
    // it will promote to this structure to save the pointers
    // Layout:
    //     count      array of positions       array of instantiated pointers
    //     ----------------------------------------------------------------
    //     1 byte     FieldsEnum::Max bytes    dynamic array depending on how many pointers has been instantiated
    template<class T, typename FieldsEnum>
    struct AuxPtrs
    {
        typedef AuxPtrsFix<FieldsEnum, 16> AuxPtrs16;
        typedef AuxPtrsFix<FieldsEnum, 32> AuxPtrs32;
        typedef AuxPtrs<T, FieldsEnum> AuxPtrsT;
        FieldWithBarrier(uint8) count;                                      // save instantiated pointers count
        FieldWithBarrier(uint8) capacity;                                   // save number of pointers can be hold in current instance of AuxPtrs
        FieldWithBarrier(uint8) offsets[static_cast<int>(FieldsEnum::Max)]; // save position of each instantiated pointers, if not instantiate, it's invalid
        FieldWithBarrier(void*) ptrs[1];                                    // instantiated pointer addresses
        AuxPtrs(uint8 capacity, AuxPtrs32* ptr32);               // called when promoting from AuxPtrs32 to AuxPtrs
        AuxPtrs(uint8 capacity, AuxPtrs* ptr);                   // called when expanding (i.e. promoting from AuxPtrs to bigger AuxPtrs)
        void* Get(FieldsEnum e);
        bool Set(FieldsEnum e, void* p);
        static void AllocAuxPtrFix(T* host, uint8 size);
        static void AllocAuxPtr(T* host, uint8 count);
        static void* GetAuxPtr(const T* host, FieldsEnum e);
        static void SetAuxPtr(T* host, FieldsEnum e, void* ptr);
    };


    template<typename FieldsEnum, uint8 size, uint8 _MaxCount>
    AuxPtrsFix<FieldsEnum, size, _MaxCount>::AuxPtrsFix()
    {
        static_assert(_MaxCount == AuxPtrsFix<FieldsEnum, 16>::MaxCount, "Should only be called on AuxPtrsFix<FieldsEnum, 16>");
        this->count = AuxPtrsFix<FieldsEnum, 16>::MaxCount;
        for (uint8 i = 0; i < count; i++)
        {
            this->type[i] = FieldsEnum::Invalid;
        }
    }

    template<typename FieldsEnum, uint8 size, uint8 _MaxCount>
    AuxPtrsFix<FieldsEnum, size, _MaxCount>::AuxPtrsFix(AuxPtrsFix<FieldsEnum, 16>* ptr16)
    {
        static_assert(_MaxCount == AuxPtrsFix<FieldsEnum, 32>::MaxCount, "Should only be called on AuxPtrsFix<FieldsEnum, 32>");
        this->count = AuxPtrsFix<FieldsEnum, 32>::MaxCount;
        for (uint8 i = 0; i < AuxPtrsFix<FieldsEnum, 16>::MaxCount; i++)
        {
            this->type[i] = ptr16->type[i];
            this->ptr[i] = ptr16->ptr[i];
        }
        for (uint8 i = AuxPtrsFix<FieldsEnum, 16>::MaxCount; i < count; i++)
        {
            this->type[i] = FieldsEnum::Invalid;
        }
    }
    template<typename FieldsEnum, uint8 size, uint8 _MaxCount>
    inline void* AuxPtrsFix<FieldsEnum, size, _MaxCount>::Get(FieldsEnum e)
    {
        Assert(count == _MaxCount);
        for (uint8 i = 0; i < _MaxCount; i++) // using _MaxCount instead of count so compiler can optimize in case _MaxCount is 1.
        {
            if ((FieldsEnum) type[i] == e)
            {
                return ptr[i];
            }
        }
        return nullptr;
    }
    template<typename FieldsEnum, uint8 size, uint8 _MaxCount>
    inline bool AuxPtrsFix<FieldsEnum, size, _MaxCount>::Set(FieldsEnum e, void* p)
    {
        Assert(count == _MaxCount);
        for (uint8 i = 0; i < _MaxCount; i++)
        {
            if ((FieldsEnum) type[i] == e || (FieldsEnum) type[i] == FieldsEnum::Invalid)
            {
                ptr[i] = p;
                type[i] = e;
                return true;
            }
        }
        return false;
    }
    template<class T, typename FieldsEnum>
    AuxPtrs<T, FieldsEnum>::AuxPtrs(uint8 capacity, AuxPtrs32* ptr32)
    {
        Assert(ptr32->count >= AuxPtrs32::MaxCount);
        this->count = ptr32->count;
        this->capacity = capacity;
        memset(offsets, (uint8)FieldsEnum::Invalid, (uint8)FieldsEnum::Max);
        for (uint8 i = 0; i < ptr32->count; i++)
        {
            offsets[(uint8)(FieldsEnum)ptr32->type[i]] = i;
            ptrs[i] = ptr32->ptr[i];
        }
    }
    template<class T, typename FieldsEnum>
    AuxPtrs<T, FieldsEnum>::AuxPtrs(uint8 capacity, AuxPtrs* ptr)
    {
        ArrayWriteBarrierVerifyBits(&this->ptrs, ptr->count);
        memcpy(this, ptr, offsetof(AuxPtrs, ptrs) + ptr->count * sizeof(void*));
        ArrayWriteBarrier(&this->ptrs, ptr->count);
        this->capacity = capacity;
    }
    template<class T, typename FieldsEnum>
    inline void* AuxPtrs<T, FieldsEnum>::Get(FieldsEnum e)
    {
        uint8 u = (uint8)e;
        return offsets[u] == (uint8)FieldsEnum::Invalid ? nullptr : (void*)ptrs[offsets[u]];
    }
    template<class T, typename FieldsEnum>
    inline bool AuxPtrs<T, FieldsEnum>::Set(FieldsEnum e, void* p)
    {
        uint8 u = (uint8)e;
        if (offsets[u] != (uint8)FieldsEnum::Invalid)
        {
            ptrs[offsets[u]] = p;
            return true;
        }
        else
        {
            if (count == capacity)
            {
                // need to expand
                return false;
            }
            else
            {
                offsets[u] = count++;
                ptrs[(uint8)offsets[u]] = p;
                return true;
            }
        }
    }

    template<class T, typename FieldsEnum>
    void AuxPtrs<T, FieldsEnum>::AllocAuxPtrFix(T* host, uint8 size)
    {
        Recycler* recycler = host->GetRecycler();
        Assert(recycler != nullptr);

        if (size == 16)
        {
            host->auxPtrs = (AuxPtrs<T, FieldsEnum>*)RecyclerNewWithBarrierStructZ(recycler, AuxPtrs16);
        }
        else if (size == 32)
        {
            host->auxPtrs = (AuxPtrs<T, FieldsEnum>*)RecyclerNewWithBarrierPlusZ(recycler, 0, AuxPtrs32, (AuxPtrs16*)(void*)host->auxPtrs);
        }
        else
        {
            Assert(false);
        }
    }

    template<class T, typename FieldsEnum>
    void AuxPtrs<T, FieldsEnum>::AllocAuxPtr(T* host, uint8 count)
    {
        Recycler* recycler = host->GetRecycler();
        Assert(recycler != nullptr);
        Assert(count >= AuxPtrs32::MaxCount);

        auto requestSize = sizeof(AuxPtrs<T, FieldsEnum>) + (count - 1)*sizeof(void*);
        auto allocSize = ::Math::Align<uint8>((uint8)requestSize, 16);
        auto capacity = (uint8)((allocSize - offsetof(AuxPtrsT, ptrs)) / sizeof(void*));

        if (host->auxPtrs->count != AuxPtrs32::MaxCount) // expanding
        {
            host->auxPtrs = RecyclerNewWithBarrierPlusZ(recycler, allocSize - sizeof(AuxPtrsT), AuxPtrsT, capacity, host->auxPtrs);
        }
        else // promoting from AuxPtrs32
        {
            host->auxPtrs = RecyclerNewWithBarrierPlusZ(recycler, allocSize - sizeof(AuxPtrsT), AuxPtrsT, capacity, (AuxPtrs32*)(void*)host->auxPtrs);
        }
    }

    template<class T, typename FieldsEnum>
    inline void* AuxPtrs<T, FieldsEnum>::GetAuxPtr(const T* host, FieldsEnum e)
    {
        auto tmpAuxPtrs = PointerValue(host->auxPtrs);
        if (tmpAuxPtrs->count == AuxPtrs16::MaxCount)
        {
            return ((AuxPtrs16*)(void*)tmpAuxPtrs)->Get(e);
        }
        if (tmpAuxPtrs->count == AuxPtrs32::MaxCount)
        {
            return ((AuxPtrs32*)(void*)tmpAuxPtrs)->Get(e);
        }
        return tmpAuxPtrs->Get(e);
    }
    template<class T, typename FieldsEnum>
    void AuxPtrs<T, FieldsEnum>::SetAuxPtr(T* host, FieldsEnum e, void* ptr)
    {

        if (host->auxPtrs == nullptr)
        {
            AuxPtrs<FunctionProxy, FieldsEnum>::AllocAuxPtrFix(host, 16);
            bool ret = ((AuxPtrs16*)(void*)host->auxPtrs)->Set(e, ptr);
            Assert(ret);
            return;
        }
        if (host->auxPtrs->count == AuxPtrs16::MaxCount)
        {
            bool ret = ((AuxPtrs16*)(void*)host->auxPtrs)->Set(e, ptr);
            if (ret)
            {
                return;
            }
            else
            {
                AuxPtrs<FunctionProxy, FieldsEnum>::AllocAuxPtrFix(host, 32);
            }
        }
        if (host->auxPtrs->count == AuxPtrs32::MaxCount)
        {
            bool ret = ((AuxPtrs32*)(void*)host->auxPtrs)->Set(e, ptr);
            if (ret)
            {
                return;
            }
            else
            {
                AuxPtrs<FunctionProxy, FieldsEnum>::AllocAuxPtr(host, AuxPtrs32::MaxCount + 1);
            }
        }

        bool ret = host->auxPtrs->Set(e, ptr);
        if (!ret)
        {
            AuxPtrs<FunctionProxy, FieldsEnum>::AllocAuxPtr(host, host->auxPtrs->count + 1);
            ret = host->auxPtrs->Set(e, ptr);
            Assert(ret);
        }
    }
}

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#if !defined(_M_ARM64)
CompileAssert(false)
#endif

struct XDataAllocation sealed : public SecondaryAllocation
{
    // ---- Methods ----- //
    XDataAllocation() :
        pdataCount(0)
        , functionTable(NULL)
        , xdataSize(0)
    {}

    RUNTIME_FUNCTION* GetPdataArray() const
    {
        return reinterpret_cast<RUNTIME_FUNCTION*>(address + xdataSize);
    }

    bool IsFreed() const
    {
        return address == nullptr;
    }

    void Free()
    {
        address = nullptr;
        pdataCount = 0;
        functionTable = nullptr;
        xdataSize = 0;
    }

    // ---- Data members ---- //
    ushort pdataCount;                   // ARM requires more than 1 pdata/function
    FunctionTableHandle functionTable;   // stores the handle to the growable function table
    ushort xdataSize;
};

//
// Allocates xdata and pdata entries for ARM architecture on the heap. They are freed when released.
//
//
class XDataAllocator sealed : public SecondaryAllocator
{
    // --------- Public functions ---------/
public:
    XDataAllocator(BYTE* address, uint size);

    bool Initialize(void* segmentStart, void* segmentEnd);
    void Delete();
    bool Alloc(ULONG_PTR functionStart, DWORD functionSize, ushort pdataCount, ushort xdataSize, SecondaryAllocation* allocation);
    void Release(const SecondaryAllocation& address);
    bool CanAllocate();
    static DWORD GetAllocSize(ushort pdataCount, ushort xdataSize)
    {
        __debugbreak();
        return 0;
    }

    static void Register(XDataAllocation * xdataInfo, intptr_t functionStart, DWORD functionSize);
    static void Unregister(XDataAllocation * xdataInfo);
};

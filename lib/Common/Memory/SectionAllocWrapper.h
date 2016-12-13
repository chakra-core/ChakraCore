//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
#if _WIN32
namespace Memory
{

struct SectionInfo
{
    HANDLE handle;
    void * runtimeBaseAddress;
};

// multi level map, modeled after HeapBlockMap
class SectionMap32
{
#if TARGET_64
    friend class SectionMap64;
#endif
public:
#if TARGET_64
    SectionMap32(__in char * startAddress);
#else
    SectionMap32();
#endif
    ~SectionMap32();

    bool SetSection(void * address, uint pageCount, SectionInfo * section);
    SectionInfo * GetSection(void * address);
    void ClearSection(void * address, uint pageCount);
private:
#if TARGET_64
    static const size_t TotalSize = 0x100000000;        // 4GB
#endif
    static const uint L1Count = 4096;
    static const uint L2Count = 256;
    static uint GetLevel1Id(void * address)
    {
        return ::Math::PointerCastToIntegralTruncate<uint>(address) / L2Count / AutoSystemInfo::PageSize;
    }
    static uint GetLevel2Id(void * address)
    {
        return ::Math::PointerCastToIntegralTruncate<uint>(address) % (L2Count * AutoSystemInfo::PageSize) / AutoSystemInfo::PageSize;
    }

    bool EnsureSection(void * address, uint pageCount);
    void SetSectionNoCheck(void * address, uint pageCount, SectionInfo * section);
    void Cleanup();

    class L2MapChunk
    {
    public:
        SectionInfo * Get(void * address);
        void Set(uint id2, uint pageCount, SectionInfo * section);
        void Clear(uint id2, uint pageCount);
        bool IsEmpty() const;
        ~L2MapChunk();
    private:
        SectionInfo * map[L2Count];
    };
    uint count;
    L2MapChunk * map[L1Count];

#if TARGET_64
    // On 64 bit, this structure only maps one particular 32 bit space, always 4GB aligned
    char * startAddress;
#endif
};

#if TARGET_64

class SectionMap64
{
public:
    SectionMap64();
    ~SectionMap64();

    bool SetSection(void * address, uint pageCount, SectionInfo * section);
    SectionInfo * GetSection(void * address);
    void ClearSection(void * address, uint pageCount);

private:
    bool EnsureSection(void * address, size_t pageCount);
    void SetSectionNoCheck(void * address, size_t pageCount, SectionInfo * section);

    static const uint PagesPer4GB = 1 << 20; // = 1M,  assume page size = 4K

    struct Node
    {
        Node(__in char * startAddress) : map(startAddress) { }

        uint nodeIndex;
        Node * next;
        SectionMap32 map;
    };

    Node * FindOrInsertNode(void * address);
    Node * FindNode(void * address) const;

    template <class Fn>
    void ForEachNodeInAddressRange(void * address, size_t pageCount, Fn fn);

    Node * list;

    static uint GetNodeIndex(void * address)
    {
        return GetNodeIndex((ULONG64)address);
    }

    static uint GetNodeIndex(ULONG64 address)
    {
        return (uint)((ULONG64)address >> 32);
    }

    static char * GetNodeStartAddress(void * address)
    {
        return (char *)(((size_t)address) & ~(SectionMap32::TotalSize - 1));
    }

};

typedef SectionMap64 SectionMap;

#else
typedef SectionMap32 SectionMap;
#endif // TARGET_64

class SectionAllocWrapper
{
public:
    SectionAllocWrapper(HANDLE process);

    LPVOID  Alloc(LPVOID lpAddress, DECLSPEC_GUARD_OVERFLOW size_t dwSize, DWORD allocationType, DWORD protectFlags, bool isCustomHeapAllocation);
    BOOL    Free(LPVOID lpAddress, size_t dwSize, DWORD dwFreeType);
    LPVOID  AllocLocal(LPVOID lpAddress, DECLSPEC_GUARD_OVERFLOW size_t dwSize);
    BOOL    FreeLocal(LPVOID lpAddress);

private:

    HANDLE process;
    SectionMap sections;
};

class PreReservedSectionAllocWrapper
{
public:
#if _M_IX86_OR_ARM32
    static const uint PreReservedAllocationSegmentCount = 256; // (256 * 64K) == 16 MB, if 64k is the AllocationGranularity
#else // _M_X64_OR_ARM64
    static const uint PreReservedAllocationSegmentCount = 4096; //(4096 * 64K) == 256MB, if 64k is the AllocationGranularity
#endif

#if !_M_X64_OR_ARM64 && _CONTROL_FLOW_GUARD
    static const unsigned MaxPreReserveSegment = 6;
#endif

    PreReservedSectionAllocWrapper(HANDLE process);
    ~PreReservedSectionAllocWrapper();

    LPVOID  Alloc(LPVOID lpAddress, DECLSPEC_GUARD_OVERFLOW size_t dwSize, DWORD allocationType, DWORD protectFlags, bool isCustomHeapAllocation);
    BOOL    Free(LPVOID lpAddress, size_t dwSize, DWORD dwFreeType);
    LPVOID  AllocLocal(LPVOID lpAddress, DECLSPEC_GUARD_OVERFLOW size_t dwSize);
    BOOL    FreeLocal(LPVOID lpAddress);

    bool        IsInRange(void * address);
    static bool IsInRange(void * regionStart, void * address);
    LPVOID      EnsurePreReservedRegion();

    LPVOID      GetPreReservedEndAddress();
    static LPVOID GetPreReservedEndAddress(void * regionStart);
#if !_M_X64_OR_ARM64 && _CONTROL_FLOW_GUARD
    static int  NumPreReservedSegment() { return numPreReservedSegment; }
#endif

#if DBG_DUMP || defined(ENABLE_IR_VIEWER)
    bool        IsPreReservedEndAddress(LPVOID address)
    {
        return IsPreReservedRegionPresent() && address == GetPreReservedEndAddress();
    }
#endif
private:
    LPVOID      EnsurePreReservedRegionInternal();
    bool        IsPreReservedRegionPresent();
    LPVOID      GetPreReservedStartAddress();

    BVStatic<PreReservedAllocationSegmentCount>     freeSegments;
    LPVOID                                          preReservedStartAddress;
    CriticalSection                                 cs;
#if !_M_X64_OR_ARM64 && _CONTROL_FLOW_GUARD
    static uint  numPreReservedSegment;
#endif

    HANDLE process;
    HANDLE section;
};

} // namespace Memory
#endif

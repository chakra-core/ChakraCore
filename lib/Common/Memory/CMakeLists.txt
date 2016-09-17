add_library (Chakra.Common.Memory OBJECT
    # xplat-todo: Include platform\XDataAllocator.cpp
    # Needed on windows, need a replacement for linux to do
    # amd64 stack walking
    Allocator.cpp
    ArenaAllocator.cpp

    CustomHeap.cpp
    CommonMemoryPch.cpp
    EtwMemoryTracking.cpp
    ForcedMemoryConstraints.cpp
    HeapAllocator.cpp
    HeapAllocatorOperators.cpp
    HeapBlock.cpp
    HeapBlockMap.cpp
    HeapBucket.cpp
    HeapInfo.cpp
    IdleDecommitPageAllocator.cpp
    LargeHeapBlock.cpp
    LargeHeapBucket.cpp
    LeakReport.cpp
    MarkContext.cpp
    MemoryLogger.cpp
    MemoryTracking.cpp
    MemUtils.cpp
    PageAllocator.cpp
    Recycler.cpp
    RecyclerHeuristic.cpp
    RecyclerObjectDumper.cpp
    RecyclerObjectGraphDumper.cpp
    RecyclerPageAllocator.cpp
    RecyclerSweep.cpp    
    RecyclerWriteBarrierManager.cpp
    SmallFinalizableHeapBlock.cpp
    SmallFinalizableHeapBucket.cpp
    SmallHeapBlockAllocator.cpp
    SmallLeafHeapBlock.cpp
    SmallLeafHeapBucket.cpp
    SmallNormalHeapBlock.cpp
    SmallNormalHeapBucket.cpp
    StressTest.cpp
    VirtualAllocWrapper.cpp
    amd64/amd64_SAVE_REGISTERS.S
    )

include_directories(..)

target_include_directories (
    Chakra.Common.Memory PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

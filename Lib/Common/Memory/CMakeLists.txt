add_library (Chakra.Common.Memory
    Allocator.cpp
    ArenaAllocator.cpp
    CommonMemoryPch.cpp
    EtwMemoryTracking.cpp
    ForcedMemoryConstraints.cpp
    HeapAllocator.cpp
    HeapAllocatorOperators.cpp

    # xplat-todo: Fix me
    #HeapBlock.cpp

    HeapBlockMap.cpp
    HeapBucket.cpp

    # xplat-todo: Fix me
    #HeapInfo.cpp

    IdleDecommitPageAllocator.cpp

    # xplat-todo: Fix me
    #LargeHeapBlock.cpp

    LargeHeapBucket.cpp
    LeakReport.cpp
    MarkContext.cpp
    MemoryLogger.cpp
    MemoryTracking.cpp
    PageAllocator.cpp

    # xplat-todo: Fix me
    #Recycler.cpp

    RecyclerHeuristic.cpp
    RecyclerObjectDumper.cpp
    RecyclerObjectGraphDumper.cpp
    RecyclerPageAllocator.cpp
    RecyclerSweep.cpp

    # xplat-todo: Fix me
    #RecyclerWriteBarrierManager.cpp

    SmallFinalizableHeapBlock.cpp
    SmallFinalizableHeapBucket.cpp
    SmallHeapBlockAllocator.cpp
    SmallLeafHeapBlock.cpp
    SmallLeafHeapBucket.cpp
    SmallNormalHeapBlock.cpp
    SmallNormalHeapBucket.cpp

    # xplat-todo: Are all the APIs used here ported?
    # StressTest.cpp

    VirtualAllocWrapper.cpp
    )

include_directories(..)

target_include_directories (
    Chakra.Common.Memory PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

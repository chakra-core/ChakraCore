add_library (Chakra.Common.DataStructures OBJECT
    BigInt.cpp
    BufferBuilder.cpp
    CommonDataStructuresPch.cpp
    DictionaryStats.cpp
    FixedBitVector.cpp
    FixedBitVectorEnumerator.cpp
    GrowingArray.cpp
    ImmutableList.cpp
    InternalString.cpp
    InternalStringNoCaseComparer.cpp
    Interval.cpp
    SizePolicy.cpp
    )

include_directories(..)

target_include_directories (
    Chakra.Common.DataStructures PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

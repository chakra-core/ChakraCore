add_executable (GCStress
  GCStress.cpp
  RecyclerTestObject.cpp
  stdafx.cpp
  StubExternalApi.cpp
    )

include_directories(..)

target_include_directories (GCStress  
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  $<BUILD_INTERFACE:${ROOT_SOURCE_DIR}/lib/Common>
  $<BUILD_INTERFACE:${ROOT_SOURCE_DIR}/lib/Common/Memory>  
  )

set(LINK_FLAGS ${LINK_FLAGS} "-Wl,-whole-archive")

target_link_libraries (GCStress
  Chakra.Common.Memory
  Chakra.Common.Common
  Chakra.Common.Core
  Chakra.Common.DataStructures
  Chakra.Common.Exceptions
  Chakra.Pal
  )

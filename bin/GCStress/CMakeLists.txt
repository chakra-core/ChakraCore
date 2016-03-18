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

target_link_libraries (GCStress
  PRIVATE Chakra.Common.Memory
  PRIVATE Chakra.Common.Common  
  PRIVATE Chakra.Common.Core
  PRIVATE Chakra.Common.DataStructures
  PRIVATE Chakra.Common.Exceptions
  PRIVATE Chakra.Pal
  )

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

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  set(lib_target
    -Wl,--no-undefined
    -Wl,--start-group
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  set(lib_target
    -Wl,-undefined,error
    )
else()
  message("This platform is not yet supported")
endif() # Linux ?

# make sure to include Common.Core before others
# this will help linker on some platforms for correct
# initialization order
set(lib_target "${lib_target}"
  Chakra.Common.Core
  )

# common link deps
set(lib_target "${lib_target}"
  Chakra.Common.Memory
  Chakra.Runtime.PlatformAgnostic
  Chakra.Common.Common
  Chakra.Common.DataStructures
  Chakra.Common.Exceptions
  )

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  set(lib_target "${lib_target}"
    -Wl,--end-group
    Chakra.Pal
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  set(lib_target "${lib_target}"
    Chakra.Pal
    )
endif() # Linux ?

target_link_libraries (GCStress ${lib_target})

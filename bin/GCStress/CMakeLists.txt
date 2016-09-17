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

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  set(LINKER_START_GROUP -Wl,--start-group)
  set(LINKER_END_GROUP -Wl,--end-group)
endif()

if(CC_TARGETS_X86)
  set(lib_target "-m32")
endif()

# common link deps
set(lib_target "${lib_target}"
  -Wl,-undefined,error
  ${LINKER_START_GROUP}
  Chakra.Common.Core
  Chakra.Pal
  Chakra.Jsrt
  ${LINKER_END_GROUP}
  )

target_link_libraries (GCStress ${lib_target})

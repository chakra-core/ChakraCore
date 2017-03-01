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

if(CC_TARGET_OS_ANDROID OR CC_TARGET_OS_LINUX)
  set(LINKER_START_GROUP -Wl,--start-group)
  set(LINKER_END_GROUP -Wl,--end-group)
elseif(CC_TARGET_OS_OSX)
  if(CC_TARGETS_X86)
    set(lib_target "${lib_target} -arch i386")
  elseif(CC_TARGETS_ARM)
    set(lib_target "${lib_target} -arch arm")
  endif()
endif()

# common link deps
set(lib_target "${lib_target}"
  -Wl,-undefined,error
  ${LINKER_START_GROUP}
  ChakraCoreStatic
  ${LINKER_END_GROUP}
  ${CC_LTO_ENABLED}
  dl
  )

if(CC_TARGET_OS_OSX)
  set(lib_target "${lib_target}"
    "-framework CoreFoundation"
    "-framework Security"
    )
elseif(NOT CC_TARGET_OS_ANDROID)
  set(lib_target "${lib_target}"
    "pthread"
    )
endif()

target_link_libraries (GCStress ${lib_target})

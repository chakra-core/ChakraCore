add_library (Chakra.SCACore OBJECT
    SCACore.cpp
    SCACorePch.cpp
    SCADeserialization.cpp
    SCAEngine.cpp
    SCAPropBag.cpp
    SCASerialization.cpp
    StreamHelper.cpp
    StreamReader.cpp
    StreamWriter.cpp
    )


target_include_directories (
    Chakra.SCACore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    ../Parser
    ../Common
    ../Backend
    ../JITIDL
    ../Runtime
    ../Runtime/ByteCode
    )

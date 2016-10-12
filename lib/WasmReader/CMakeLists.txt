add_library (Chakra.WasmReader OBJECT
    
    WasmReaderPch.cpp
    WasmDataSegment.cpp
    WasmFunctionInfo.cpp
    WasmModule.cpp
    WasmSection.cpp
    WasmSignature.cpp
    WasmBinaryReader.cpp
    WasmByteCodeGenerator.cpp
    )

target_include_directories (
    Chakra.WasmReader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    ../JITIDL
    ../Backend
    ../Runtime
    ../Runtime/ByteCode
    ../Parser
    )

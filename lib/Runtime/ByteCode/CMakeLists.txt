add_library (Chakra.Runtime.ByteCode OBJECT
    AsmJsByteCodeDumper.cpp
    AsmJsByteCodeWriter.cpp
    WasmByteCodeWriter.cpp
    BackendOpCodeAttr.cpp
    ByteBlock.cpp
    ByteCodeDumper.cpp
    ByteCodeEmitter.cpp
    ByteCodeGenerator.cpp
    ByteCodeReader.cpp
    ByteCodeSerializer.cpp
    ByteCodeWriter.cpp
    FuncInfo.cpp
    OpCodeUtil.cpp
    OpCodeUtilAsmJs.cpp
    OpCodes.cpp
    RuntimeByteCodePch.cpp
    Scope.cpp
    ScopeInfo.cpp
    StatementReader.cpp
    Symbol.cpp
    )

target_include_directories (
    Chakra.Runtime.ByteCode PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library (Chakra.Common.Exceptions OBJECT
    # CommonExceptionsPch.cpp
    ExceptionCheck.cpp
    ExceptionBase.cpp
    ReportError.cpp
    Throw.cpp
)

target_include_directories (
    Chakra.Common.Exceptions PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

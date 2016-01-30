add_library (Chakra.Common.Exceptions
  # CommonExceptionsPch.cpp
  ExceptionCheck.cpp
  ExceptionBase.cpp
  reporterror.cpp
  Throw.cpp
)

target_include_directories (
  Chakra.Common.Exceptions PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

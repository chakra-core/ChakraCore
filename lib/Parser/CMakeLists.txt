# xplat-todo: This is a skeleton make file and not used in build yet.
# Please add this to build and fix issues.

add_library (Chakra.Parser
    Alloc.cpp
    BackgroundParser.cpp
    CaseInsensitive.cpp
    CharClassifier.cpp
    CharSet.cpp
    CharTrie.cpp
    DebugWriter.cpp
    Hash.cpp
    OctoquadIdentifier.cpp
    Parse.cpp
    ParserPch.cpp
    RegexCompileTime.cpp
    RegexParser.cpp
    RegexPattern.cpp
    RegexRuntime.cpp
    RegexStats.cpp
    Scan.cpp
    StandardChars.cpp
    TextbookBoyerMoore.cpp
    cmperr.cpp
    errstr.cpp
    globals.cpp
    jserr.gen
    rterror.cpp
    screrror.cpp
    )

target_include_directories (
    Chakra.Parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

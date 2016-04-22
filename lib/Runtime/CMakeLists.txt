project(CHAKRA_RUNTIME)

include_directories(
    $(CHAKRA_RUNTIME_SOURCE_DIR)
    $(CHAKRA_RUNTIME_SOURCE_DIR)../Common
    $(CHAKRA_RUNTIME_SOURCE_DIR)../Backend
    $(CHAKRA_RUNTIME_SOURCE_DIR)../Parser
    $(CHAKRA_RUNTIME_SOURCE_DIR)/ByteCode
    $(CHAKRA_RUNTIME_SOURCE_DIR)/PlatformAgnostic
    $(CHAKRA_RUNTIME_SOURCE_DIR)/Math
    )

add_subdirectory (Base)
add_subdirectory (PlatformAgnostic)
add_subdirectory (ByteCode)
add_subdirectory (Debug)
add_subdirectory (Language)
add_subdirectory (Library)
add_subdirectory (Math)
add_subdirectory (Types)

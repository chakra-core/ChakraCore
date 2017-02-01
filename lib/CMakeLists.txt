add_compile_options(-fPIC)

if(BuildJIT)
    add_subdirectory (Backend)
endif()

add_subdirectory (Common)
add_subdirectory (Parser)
add_subdirectory (Runtime)
add_subdirectory (Jsrt)
if (CC_TARGETS_AMD64)
    add_subdirectory (WasmReader)
endif()

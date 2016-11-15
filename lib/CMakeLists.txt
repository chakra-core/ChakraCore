add_compile_options(-fPIC)

if(CC_ENABLE_JIT)
    add_subdirectory (Backend)
endif()

add_subdirectory (Common)
add_subdirectory (Parser)
add_subdirectory (Runtime)
add_subdirectory (Jsrt)

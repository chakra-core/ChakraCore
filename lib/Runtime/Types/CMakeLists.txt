add_library (Chakra.Runtime.Types OBJECT
    ActivationObject.cpp
    ArrayObject.cpp
    DeferredTypeHandler.cpp
    DictionaryTypeHandler.cpp
    DynamicObject.cpp
    DynamicObjectPropertyEnumerator.cpp
    DynamicType.cpp
    ES5ArrayTypeHandler.cpp
    JavascriptEnumerator.cpp
    JavascriptStaticEnumerator.cpp
    MissingPropertyTypeHandler.cpp
    NullTypeHandler.cpp
    PathTypeHandler.cpp
    PropertyDescriptor.cpp
    RecyclableObject.cpp
    RuntimeTypePch.cpp
    ScriptFunctionType.cpp
    SimpleDictionaryTypeHandler.cpp
    SimpleDictionaryUnorderedTypeHandler.cpp
    SimpleTypeHandler.cpp
    SpreadArgument.cpp
    StaticType.cpp
    Type.cpp
    TypeHandler.cpp
    TypePath.cpp
    TypePropertyCache.cpp
    WithScopeObject.cpp
    )

target_include_directories (
    Chakra.Runtime.Types PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

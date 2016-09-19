set(CRLIB_SOURCE_CODES
    ArgumentsObject.cpp
    ArgumentsObjectEnumerator.cpp
    ArrayBuffer.cpp
    AtomicsObject.cpp
    BoundFunction.cpp
    BufferStringBuilder.cpp
    CommonExternalApiImpl.cpp
    CompoundString.cpp
    ConcatString.cpp
    DataView.cpp
    DateImplementation.cpp
    ES5Array.cpp
    ES5ArrayIndexEnumerator.cpp
    EngineInterfaceObject.cpp
    ExternalLibraryBase.cpp
    ForInObjectEnumerator.cpp
    GlobalObject.cpp
    IntlEngineInterfaceExtensionObject.cpp
    IteratorObjectEnumerator.cpp
    JSON.cpp
    JSONParser.cpp
    JSONScanner.cpp
    JSONStack.cpp
    JSONString.cpp
    JavascriptArray.cpp
    JavascriptArrayIndexEnumerator.cpp
    JavascriptArrayIndexEnumeratorBase.cpp
    JavascriptArrayIterator.cpp
    JavascriptArrayIndexSnapshotEnumerator.cpp
    JavascriptBoolean.cpp
    JavascriptBooleanObject.cpp
    JavascriptBuiltInFunctions.cpp
    JavascriptDate.cpp
    JavascriptEnumeratorIterator.cpp
    JavascriptError.cpp
    # JavascriptErrorDebug.cpp
    JavascriptExternalFunction.cpp
    JavascriptFunction.cpp
    JavascriptGenerator.cpp
    JavascriptGeneratorFunction.cpp
    JavascriptIterator.cpp
    JavascriptListIterator.cpp
    JavascriptLibrary.cpp
    JavascriptMap.cpp
    JavascriptMapIterator.cpp
    JavascriptNumber.cpp
    JavascriptNumberObject.cpp
    JavascriptObject.cpp
    JavascriptPromise.cpp
    JavascriptProxy.cpp
    JavascriptReflect.cpp
    JavascriptRegExpConstructor.cpp
    JavascriptRegExpEnumerator.cpp
    JavascriptRegularExpression.cpp
    JavascriptRegularExpressionResult.cpp
    JavascriptSet.cpp
    JavascriptSetIterator.cpp
    # JavascriptSimdBool16x8.cpp
    # JavascriptSimdBool32x4.cpp
    # JavascriptSimdBool8x16.cpp
    # JavascriptSimdFloat32x4.cpp
    # JavascriptSimdFloat64x2.cpp
    # JavascriptSimdInt16x8.cpp
    # JavascriptSimdInt32x4.cpp
    # JavascriptSimdInt8x16.cpp
    # JavascriptSimdObject.cpp
    # JavascriptSimdUint16x8.cpp
    # JavascriptSimdUint32x4.cpp
    # JavascriptSimdUint8x16.cpp
    JavascriptString.cpp
    JavascriptStringEnumerator.cpp
    JavascriptStringIterator.cpp
    JavascriptStringObject.cpp
    JavascriptSymbol.cpp
    JavascriptSymbolObject.cpp
    JavascriptTypedNumber.cpp
    JavascriptVariantDate.cpp
    JavascriptWeakMap.cpp
    JavascriptWeakSet.cpp
    LiteralString.cpp
    MathLibrary.cpp
    ModuleRoot.cpp
    ObjectPrototypeObject.cpp
    ProfileString.cpp
    PropertyString.cpp
    RegexHelper.cpp
    RootObjectBase.cpp
    RuntimeFunction.cpp
    RuntimeLibraryPch.cpp
    ScriptFunction.cpp
    # xplat-todo: enable SIMDjs on Linux
    # SimdBool16x8Lib.cpp
    # SimdBool32x4Lib.cpp
    # SimdBool8x16Lib.cpp
    # SimdFloat32x4Lib.cpp
    # SimdFloat64x2Lib.cpp
    # SimdInt16x8Lib.cpp
    # SimdInt32x4Lib.cpp
    # SimdInt8x16Lib.cpp
    # SimdUint16x8Lib.cpp
    # SimdUint32x4Lib.cpp
    # SimdUint8x16Lib.cpp
    SharedArrayBuffer.cpp
    SingleCharString.cpp
    SparseArraySegment.cpp
    StackScriptFunction.cpp
    StringCopyInfo.cpp
    SubString.cpp
    ThrowErrorObject.cpp
    TypedArray.cpp
    TypedArrayIndexEnumerator.cpp
    UriHelper.cpp
    )

if(CC_TARGETS_AMD64)
    set (CRLIB_SOURCE_CODES ${CRLIB_SOURCE_CODES}
        amd64/JavascriptFunctionA.S
    )
elseif(CC_TARGETS_X86)
    # x86 here
endif()

add_library (Chakra.Runtime.Library OBJECT ${CRLIB_SOURCE_CODES})

target_include_directories (
    Chakra.Runtime.Library PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

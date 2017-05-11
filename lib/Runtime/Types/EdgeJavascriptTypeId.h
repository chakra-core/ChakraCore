//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
enum TypeId
{
    TypeIds_Undefined = 0,
    TypeIds_Null = 1,

    TypeIds_UndefinedOrNull =  TypeIds_Null,

    TypeIds_Boolean = 2,

    // backend typeof() == "number" is true for typeIds
    // between TypeIds_FirstNumberType <= typeId <= TypeIds_LastNumberType
    TypeIds_Integer = 3,
    TypeIds_FirstNumberType = TypeIds_Integer,
    TypeIds_Number = 4,
    TypeIds_Int64Number = 5,
    TypeIds_UInt64Number = 6,
    TypeIds_LastNumberType = TypeIds_UInt64Number,

    TypeIds_String = 7,
    TypeIds_Symbol = 8,

    TypeIds_LastToPrimitiveType = TypeIds_Symbol,

    TypeIds_Enumerator = 9,
    TypeIds_VariantDate = 10,

    // SIMD types 
    //[Please maintain Float32x4 as the first SIMDType and Bool8x16 as the last]
    TypeIds_SIMDFloat32x4 = 11,
    TypeIds_SIMDFloat64x2 = 12,
    TypeIds_SIMDInt32x4 = 13,
    TypeIds_SIMDInt16x8 = 14,
    TypeIds_SIMDInt8x16 = 15,

    TypeIds_SIMDUint32x4 = 16,
    TypeIds_SIMDUint16x8 = 17,
    TypeIds_SIMDUint8x16 = 18,

    TypeIds_SIMDBool32x4 = 19,
    TypeIds_SIMDBool16x8 = 20,
    TypeIds_SIMDBool8x16 = 21,
    TypeIds_LastJavascriptPrimitiveType = TypeIds_SIMDBool8x16,

    TypeIds_HostDispatch = 22,
    TypeIds_WithScopeObject = 23,
    TypeIds_UndeclBlockVar = 24,

    TypeIds_LastStaticType = TypeIds_UndeclBlockVar,

    TypeIds_Proxy = 25,
    TypeIds_Function = 26,

    //
    // The backend expects only objects whose typeof() === "object" to have a
    // TypeId >= TypeIds_Object. Only 'null' is a special case because it
    // has a static type.
    //
    TypeIds_Object = 27,
    TypeIds_Array = 28,
    TypeIds_ArrayFirst = TypeIds_Array,
    TypeIds_NativeIntArray = 29,
  #if ENABLE_COPYONACCESS_ARRAY
    TypeIds_CopyOnAccessNativeIntArray = 30,
  #endif
    TypeIds_NativeFloatArray = 31,
    TypeIds_ArrayLast = TypeIds_NativeFloatArray,
    TypeIds_ES5Array = 32,
    TypeIds_ArrayLastWithES5 = TypeIds_ES5Array,
    TypeIds_Date = 33,
    TypeIds_RegEx = 34,
    TypeIds_Error = 35,
    TypeIds_BooleanObject = 36,
    TypeIds_NumberObject = 37,
    TypeIds_StringObject = 38,
    TypeIds_SIMDObject = 39,
    TypeIds_Arguments = 40,
    TypeIds_ArrayBuffer = 41,
    TypeIds_Int8Array = 42,
    TypeIds_TypedArrayMin = TypeIds_Int8Array,
    TypeIds_TypedArraySCAMin = TypeIds_Int8Array, // Min SCA supported TypedArray TypeId
    TypeIds_Uint8Array = 43,
    TypeIds_Uint8ClampedArray = 44,
    TypeIds_Int16Array = 45,
    TypeIds_Uint16Array = 46,
    TypeIds_Int32Array = 47,
    TypeIds_Uint32Array = 48,
    TypeIds_Float32Array = 49,
    TypeIds_Float64Array = 50,
    TypeIds_TypedArraySCAMax = TypeIds_Float64Array, // Max SCA supported TypedArray TypeId
    TypeIds_Int64Array = 51,
    TypeIds_Uint64Array = 52,
    TypeIds_CharArray = 53,
    TypeIds_BoolArray = 54,
    TypeIds_TypedArrayMax = TypeIds_BoolArray,
    TypeIds_EngineInterfaceObject = 55,
    TypeIds_DataView = 56,
    TypeIds_WinRTDate = 57,
    TypeIds_Map = 58,
    TypeIds_Set = 59,
    TypeIds_WeakMap = 60,
    TypeIds_WeakSet = 61,
    TypeIds_SymbolObject = 62,
    TypeIds_ArrayIterator = 63,
    TypeIds_MapIterator = 64,
    TypeIds_SetIterator = 65,
    TypeIds_StringIterator = 66,
    TypeIds_JavascriptEnumeratorIterator = 67,      /* Unused */
    TypeIds_Generator = 68,
    TypeIds_Promise = 69,
    TypeIds_SharedArrayBuffer = 70,

    TypeIds_WebAssemblyModule = 71,
    TypeIds_WebAssemblyInstance = 72,
    TypeIds_WebAssemblyMemory = 73,
    TypeIds_WebAssemblyTable = 74,

    TypeIds_LastBuiltinDynamicObject = TypeIds_WebAssemblyTable,
    TypeIds_GlobalObject = 75,
    TypeIds_ModuleRoot = 76,
    TypeIds_LastTrueJavascriptObjectType = TypeIds_ModuleRoot,

    TypeIds_HostObject = 77,
    TypeIds_ActivationObject = 78,
    TypeIds_SpreadArgument = 79,
    TypeIds_ModuleNamespace = 80,
    TypeIds_ListIterator = 81,
    TypeIds_ExternalIterator = 82,
    TypeIds_Limit //add a new TypeId before TypeIds_Limit or before TypeIds_LastTrueJavascriptObjectType
};

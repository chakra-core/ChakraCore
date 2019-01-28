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
    TypeIds_Number = 4, // JITTypes.h uses fixed number 4 for this
    TypeIds_Int64Number = 5,
    TypeIds_UInt64Number = 6,
    TypeIds_LastNumberType = TypeIds_UInt64Number,
    TypeIds_String = 7, // JITTypes.h uses fixed number 7 for this
    TypeIds_Symbol = 8,
    TypeIds_BigInt = 9,

    TypeIds_LastToPrimitiveType = TypeIds_BigInt,

    TypeIds_Enumerator = 10,
    TypeIds_VariantDate = 11,

    // SIMD types 
    //[Please maintain Float32x4 as the first SIMDType and Bool8x16 as the last]
    TypeIds_SIMDFloat32x4 = 12,
    TypeIds_SIMDFloat64x2 = 13,
    TypeIds_SIMDInt32x4 = 14,
    TypeIds_SIMDInt16x8 = 15,
    TypeIds_SIMDInt8x16 = 16,

    TypeIds_SIMDUint32x4 = 17,
    TypeIds_SIMDUint16x8 = 18,
    TypeIds_SIMDUint8x16 = 19,

    TypeIds_SIMDBool32x4 = 20,
    TypeIds_SIMDBool16x8 = 21,
    TypeIds_SIMDBool8x16 = 22,
    TypeIds_LastJavascriptPrimitiveType = TypeIds_SIMDBool8x16,

    TypeIds_HostDispatch = 23,
    TypeIds_UnscopablesWrapperObject = 24,
    TypeIds_UndeclBlockVar = 25,

    TypeIds_LastStaticType = TypeIds_UndeclBlockVar,

    TypeIds_Proxy = 26,
    TypeIds_Function = 27,

    //
    // The backend expects only objects whose typeof() === "object" to have a
    // TypeId >= TypeIds_Object. Only 'null' is a special case because it
    // has a static type.
    //
    TypeIds_Object = 28,
    TypeIds_Array = 29,
    TypeIds_ArrayFirst = TypeIds_Array,
    TypeIds_NativeIntArray = 30,
  #if ENABLE_COPYONACCESS_ARRAY
    TypeIds_CopyOnAccessNativeIntArray = 31,
  #endif
    TypeIds_NativeFloatArray = 32,
    TypeIds_ArrayLast = TypeIds_NativeFloatArray,
    TypeIds_ES5Array = 33,
    TypeIds_ArrayLastWithES5 = TypeIds_ES5Array,
    TypeIds_Date = 34,
    TypeIds_RegEx = 35,
    TypeIds_Error = 36,
    TypeIds_BooleanObject = 37,
    TypeIds_NumberObject = 38,
    TypeIds_StringObject = 39,
    TypeIds_BigIntObject = 40,
    TypeIds_SIMDObject = 41,
    TypeIds_Arguments = 42,
    TypeIds_ArrayBuffer = 43,
    TypeIds_Int8Array = 44,
    TypeIds_TypedArrayMin = TypeIds_Int8Array,
    TypeIds_TypedArraySCAMin = TypeIds_Int8Array, // Min SCA supported TypedArray TypeId
    TypeIds_Uint8Array = 45,
    TypeIds_Uint8ClampedArray = 46,
    TypeIds_Int16Array = 47,
    TypeIds_Uint16Array = 48,
    TypeIds_Int32Array = 49,
    TypeIds_Uint32Array = 50,
    TypeIds_Float32Array = 51,
    TypeIds_Float64Array = 52,
    TypeIds_TypedArraySCAMax = TypeIds_Float64Array, // Max SCA supported TypedArray TypeId
    TypeIds_Int64Array = 53,
    TypeIds_Uint64Array = 54,
    TypeIds_CharArray = 55,
    TypeIds_BoolArray = 56,
    TypeIds_TypedArrayMax = TypeIds_BoolArray,
    TypeIds_EngineInterfaceObject = 57,
    TypeIds_DataView = 58,
    TypeIds_WinRTDate = 59,
    TypeIds_Map = 60,
    TypeIds_Set = 61,
    TypeIds_WeakMap = 62,
    TypeIds_WeakSet = 63,
    TypeIds_SymbolObject = 64,
    TypeIds_ArrayIterator = 65,
    TypeIds_MapIterator = 66,
    TypeIds_SetIterator = 67,
    TypeIds_StringIterator = 68,
    TypeIds_JavascriptEnumeratorIterator = 69,      /* Unused */
    TypeIds_Generator = 70,
    TypeIds_Promise = 71,
    TypeIds_SharedArrayBuffer = 72,

    TypeIds_WebAssemblyModule = 73,
    TypeIds_WebAssemblyInstance = 74,
    TypeIds_WebAssemblyMemory = 75,
    TypeIds_WebAssemblyTable = 76,

    TypeIds_LastBuiltinDynamicObject = TypeIds_WebAssemblyTable,
    TypeIds_GlobalObject = 77,
    TypeIds_ModuleRoot = 78,
    TypeIds_LastTrueJavascriptObjectType = TypeIds_ModuleRoot,

    TypeIds_HostObject = 79,
    TypeIds_ActivationObject = 80,
    TypeIds_SpreadArgument = 81,
    TypeIds_ModuleNamespace = 82,
    TypeIds_ListIterator = 83,
    TypeIds_ExternalIterator = 84,
    TypeIds_Limit //add a new TypeId before TypeIds_Limit or before TypeIds_LastTrueJavascriptObjectType
};

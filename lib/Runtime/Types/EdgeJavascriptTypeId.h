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
    TypeIds_Date = 32,
    TypeIds_RegEx = 33,
    TypeIds_Error = 34,
    TypeIds_BooleanObject = 35,
    TypeIds_NumberObject = 36,
    TypeIds_StringObject = 37,
    TypeIds_Arguments = 38,
    TypeIds_ES5Array = 39,
    TypeIds_ArrayBuffer = 40,
    TypeIds_Int8Array = 41,
    TypeIds_TypedArrayMin = TypeIds_Int8Array,
    TypeIds_TypedArraySCAMin = TypeIds_Int8Array, // Min SCA supported TypedArray TypeId
    TypeIds_Uint8Array = 42,
    TypeIds_Uint8ClampedArray = 43,
    TypeIds_Int16Array = 44,
    TypeIds_Uint16Array = 45,
    TypeIds_Int32Array = 46,
    TypeIds_Uint32Array = 47,
    TypeIds_Float32Array = 48,
    TypeIds_Float64Array = 49,
    TypeIds_TypedArraySCAMax = TypeIds_Float64Array, // Max SCA supported TypedArray TypeId
    TypeIds_Int64Array = 50,
    TypeIds_Uint64Array = 51,
    TypeIds_CharArray = 52,
    TypeIds_BoolArray = 53,
    TypeIds_TypedArrayMax = TypeIds_BoolArray,
    TypeIds_EngineInterfaceObject = 54,
    TypeIds_DataView = 55,
    TypeIds_WinRTDate = 56,
    TypeIds_Map = 57,
    TypeIds_Set = 58,
    TypeIds_WeakMap = 59,
    TypeIds_WeakSet = 60,
    TypeIds_SymbolObject = 61,
    TypeIds_ArrayIterator = 62,
    TypeIds_MapIterator = 63,
    TypeIds_SetIterator = 64,
    TypeIds_StringIterator = 65,
    TypeIds_JavascriptEnumeratorIterator = 66,
    TypeIds_Generator = 67,
    TypeIds_Promise = 68,

    TypeIds_LastBuiltinDynamicObject = TypeIds_Promise,
    TypeIds_GlobalObject = 69,
    TypeIds_ModuleRoot = 70,
    TypeIds_LastTrueJavascriptObjectType = TypeIds_ModuleRoot,

    TypeIds_HostObject = 71,
    TypeIds_ActivationObject = 72,
    TypeIds_SpreadArgument = 73,
    TypeIds_ModuleNamespace = 74,

    TypeIds_Limit //add a new TypeId before TypeIds_Limit or before TypeIds_LastTrueJavascriptObjectType
};



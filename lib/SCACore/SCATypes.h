//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#define SCA_FORMAT_MAJOR  1
#define SCA_FORMAT_MINOR  0
#define SCA_FORMAT_VERSION (MAKELONG(0, MAKEWORD(SCA_FORMAT_MINOR, SCA_FORMAT_MAJOR)))
#define GetSCAMajor(header) HIBYTE(HIWORD(header))
#define GetSCAMinor(header) LOBYTE(HIWORD(header))
typedef
enum SCATypeId
{
    SCA_None = 0,
    SCA_Reference = 1,
    SCA_NullValue = 2,
    SCA_UndefinedValue = 3,
    SCA_TrueValue = 4,
    SCA_FalseValue = 5,
    SCA_Int32Value = 6,
    SCA_DoubleValue = 7,
    SCA_StringValue = 8,
    SCA_Int64Value = 9,
    SCA_Uint64Value = 10,
    SCA_LastPrimitive = SCA_Uint64Value,
    SCA_BooleanTrueObject = 21,
    SCA_BooleanFalseObject = 22,
    SCA_DateObject = 23,
    SCA_NumberObject = 24,
    SCA_StringObject = 25,
    SCA_RegExpObject = 26,
    SCA_Object = 27,
    SCA_Transferable = 28,
    SCA_DenseArray = 50,
    SCA_SparseArray = 51,
    SCA_CanvasPixelArray = 52,
    SCA_ArrayBuffer = 60,
    SCA_Int8Array = 61,
    SCA_Uint8Array = 62,
    SCA_Int16Array = 63,
    SCA_Uint16Array = 64,
    SCA_Int32Array = 65,
    SCA_Uint32Array = 66,
    SCA_Float32Array = 67,
    SCA_Float64Array = 68,
    SCA_DataView = 69,
    SCA_Uint8ClampedArray = 70,
    SCA_TypedArrayMin = SCA_Int8Array,
    SCA_TypedArrayMax = SCA_Float64Array,
    SCA_Map = 80,
    SCA_Set = 81,
    SCA_SharedArrayBuffer = 82,
    SCA_WebAssemblyModule = 85,
    SCA_WebAssemblyMemory = 86,
    SCA_FirstHostObject = 100,
    SCA_BlobObject = 101,
    SCA_FileObject = 102,
    SCA_ImageDataObject = 103,
    SCA_FileListObject = 104,
    SCA_StreamObject = 105,
    SCA_WebCryptoKeyObject = 106,
    SCA_MessagePort = 107,
    SCA_Last = (SCA_MessagePort + 1)
}   SCATypeId;

typedef unsigned int scaposition_t;

#define SCA_PROPERTY_TERMINATOR 0xFFFFFFFF
#define IsSCAPrimitive(scaTypeId) ((scaTypeId) <= SCA_LastPrimitive)
#define IsSCAHostObject(scaTypeId) ((scaTypeId) >= SCA_FirstHostObject)
#define IsSCATypedArray(scaTypeId) ((scaTypeId) == SCA_Uint8ClampedArray || ((scaTypeId) >= SCA_TypedArrayMin && (scaTypeId) <= SCA_TypedArrayMax))
#define FACILITY_SCA            FACILITY_ITF
#define E_SCA_UNSUPPORTED       MAKE_HRESULT(SEVERITY_ERROR, FACILITY_SCA, 0x1000)
#define E_SCA_NEWVERSION        MAKE_HRESULT(SEVERITY_ERROR, FACILITY_SCA, 0x1001)
#define E_SCA_DATACORRUPT       MAKE_HRESULT(SEVERITY_ERROR, FACILITY_SCA, 0x1002)
#define E_SCA_TRANSFERABLE_UNSUPPORTED       MAKE_HRESULT(SEVERITY_ERROR, FACILITY_SCA, 0x1003)
#define E_SCA_TRANSFERABLE_NEUTERED       MAKE_HRESULT(SEVERITY_ERROR, FACILITY_SCA, 0x1004)

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
// Portions of this file are copyright 2014 Mozilla Foundation, available under the Apache 2.0 license.
//-------------------------------------------------------------------------------------------------------

#ifndef ARRAYBUFFER_VIEW
#error ARRAYBUFFER_VIEW Macro must be defined before including this file
// ARRAYBUFFER_VIEW(name, align, reg, mem, heapTag, valueTag)
#endif

//              (Name            , NaturalAlignment, reg  , mem ,    heapTag, valueTag)
ARRAYBUFFER_VIEW(INT8            , 0               , int32, int8,    HEAP8  , 'I'     )
ARRAYBUFFER_VIEW(UINT8           , 0               , int32, uint8,   HEAPU8 , 'U'     )
ARRAYBUFFER_VIEW(INT16           , 1               , int32, int16,   HEAP16 , 'I'     )
ARRAYBUFFER_VIEW(UINT16          , 1               , int32, uint16,  HEAPU16, 'U'     )
ARRAYBUFFER_VIEW(INT32           , 2               , int32, int32,   HEAP32 , 'I'     )
ARRAYBUFFER_VIEW(UINT32          , 2               , int32, uint32,  HEAPU32, 'U'     )
ARRAYBUFFER_VIEW(FLOAT32         , 2               , float, float,   HEAPF32, 'F'     )
ARRAYBUFFER_VIEW(FLOAT64         , 3               , double, double, HEAPF64, 'D'     )
ARRAYBUFFER_VIEW(INT64           , 3               , int64, int64,   HEAPI64, 'L'     )
ARRAYBUFFER_VIEW(INT8_TO_INT64   , 0               , int64, int8,    HEAP8  , 'L'     )
ARRAYBUFFER_VIEW(UINT8_TO_INT64  , 0               , int64, uint8,   HEAPU8 , 'L'     )
ARRAYBUFFER_VIEW(INT16_TO_INT64  , 1               , int64, int16,   HEAP16 , 'L'     )
ARRAYBUFFER_VIEW(UINT16_TO_INT64 , 1               , int64, uint16,  HEAPU16, 'L'     )
ARRAYBUFFER_VIEW(INT32_TO_INT64  , 2               , int64, int32,   HEAP32 , 'L'     )
ARRAYBUFFER_VIEW(UINT32_TO_INT64 , 2               , int64, uint32,  HEAPU32, 'L'     )

#undef ARRAYBUFFER_VIEW

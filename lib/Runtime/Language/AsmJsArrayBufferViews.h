//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
// Portions of this file are copyright 2014 Mozilla Foundation, available under the Apache 2.0 license.
//-------------------------------------------------------------------------------------------------------

#ifndef ARRAYBUFFER_VIEW
#define ARRAYBUFFER_VIEW(name, align, RegType, MemType, irSuffix)
#endif

#ifndef ARRAYBUFFER_VIEW_INT
#define ARRAYBUFFER_VIEW_INT(name, align, RegType, MemType, irSuffix) ARRAYBUFFER_VIEW(name, align, RegType, MemType, irSuffix)
#endif

#ifndef ARRAYBUFFER_VIEW_FLT
#define ARRAYBUFFER_VIEW_FLT(name, align, RegType, MemType, irSuffix) ARRAYBUFFER_VIEW(name, align, RegType, MemType, irSuffix)
#endif

//                  (Name            , Align , RegType, MemType , irSuffix )
ARRAYBUFFER_VIEW_INT(INT8            , 0     , int32  , int8    , Int8     )
ARRAYBUFFER_VIEW_INT(UINT8           , 0     , int32  , uint8   , Uint8    )
ARRAYBUFFER_VIEW_INT(INT16           , 1     , int32  , int16   , Int16    )
ARRAYBUFFER_VIEW_INT(UINT16          , 1     , int32  , uint16  , Uint16   )
ARRAYBUFFER_VIEW_INT(INT32           , 2     , int32  , int32   , Int32    )
ARRAYBUFFER_VIEW_INT(UINT32          , 2     , int32  , uint32  , Uint32   )
ARRAYBUFFER_VIEW_FLT(FLOAT32         , 2     , float  , float   , Float32  )
ARRAYBUFFER_VIEW_FLT(FLOAT64         , 3     , double , double  , Float64  )
ARRAYBUFFER_VIEW_INT(INT64           , 3     , int64  , int64   , Int64    )
ARRAYBUFFER_VIEW_INT(INT8_TO_INT64   , 0     , int64  , int8    , Int8     )
ARRAYBUFFER_VIEW_INT(UINT8_TO_INT64  , 0     , int64  , uint8   , Uint8    )
ARRAYBUFFER_VIEW_INT(INT16_TO_INT64  , 1     , int64  , int16   , Int16    )
ARRAYBUFFER_VIEW_INT(UINT16_TO_INT64 , 1     , int64  , uint16  , Uint16   )
ARRAYBUFFER_VIEW_INT(INT32_TO_INT64  , 2     , int64  , int32   , Int32    )
ARRAYBUFFER_VIEW_INT(UINT32_TO_INT64 , 2     , int64  , uint32  , Uint32   )

#undef ARRAYBUFFER_VIEW
#undef ARRAYBUFFER_VIEW_INT
#undef ARRAYBUFFER_VIEW_FLT

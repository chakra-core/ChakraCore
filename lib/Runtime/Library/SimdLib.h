//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
#ifdef ENABLE_SIMDJS
// SIMD types
#include "Library/JavascriptSimdObject.h"
#include "Library/JavascriptSimdType.h"
#include "Library/JavascriptSimdFloat32x4.h"
#include "Library/JavascriptSimdFloat64x2.h"
#include "Library/JavascriptSimdInt32x4.h"
#include "Library/JavascriptSimdInt16x8.h"
#include "Library/JavascriptSimdInt8x16.h"
#include "Library/JavascriptSimdUint32x4.h"
#include "Library/JavascriptSimdUint16x8.h"
#include "Library/JavascriptSimdUint8x16.h"
#include "Library/JavascriptSimdBool32x4.h"
#include "Library/JavascriptSimdBool16x8.h"
#include "Library/JavascriptSimdBool8x16.h"

// SIMD libs
#include "Library/SimdFloat32x4Lib.h"
#include "Library/SimdFloat64x2Lib.h"
#include "Library/SimdInt32x4Lib.h"
#include "Library/SimdInt16x8Lib.h"
#include "Library/SimdInt8x16Lib.h"
#include "Library/SimdUint32x4Lib.h"
#include "Library/SimdUint16x8Lib.h"
#include "Library/SimdUint8x16Lib.h"
#include "Library/SimdBool32x4Lib.h"
#include "Library/SimdBool16x8Lib.h"
#include "Library/SimdBool8x16Lib.h"
#endif

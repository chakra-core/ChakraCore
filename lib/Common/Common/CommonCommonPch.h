//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "CommonMin.h"

// === C Runtime Header Files ===
#if defined(_UCRT)
#include <cmath>
#else
#include <math.h>
#endif

// === Codex Header Files ===
#include "Codex/Utf8Codex.h"

// === Common Header Files ===
#include "Common/NumberUtilitiesBase.h"
#include "Common/NumberUtilities.h"
#include "Common/IntMathCommon.h"
#include "Common/Int16Math.h"
#include "Common/Int32Math.h"
#include "Common/Int64Math.h"

#ifdef _MSC_VER
#pragma warning(push)
#if defined(PROFILE_RECYCLER_ALLOC) || defined(HEAP_TRACK_ALLOC) || defined(ENABLE_DEBUG_CONFIG_OPTIONS)
#include <typeinfo.h>
#endif
#pragma warning(pop)
#endif




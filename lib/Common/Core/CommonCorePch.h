//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

// In Debug mode, the PALs definition of max and min are insufficient
// since some of our code expects the template min-max instead, so
// including that here
#if defined(DBG) && !defined(_MSC_VER)
#define NO_PAL_MINMAX
#define _Post_equal_to_(x)
#define _Post_satisfies_(x)

#include "Core/CommonMinMax.h"
#endif

#include "CommonDefines.h"
#include "CommonMin.h"

#ifdef _MSC_VER
#pragma warning(push)
#if defined(PROFILE_RECYCLER_ALLOC) || defined(HEAP_TRACK_ALLOC) || defined(ENABLE_DEBUG_CONFIG_OPTIONS)
#include <typeinfo.h>
#endif
#pragma warning(pop)
#endif





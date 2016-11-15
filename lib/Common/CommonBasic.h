//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "Banned.h"
#include "CommonDefines.h"
#define _CRT_RAND_S         // Enable rand_s in the CRT

#if defined(PROFILE_RECYCLER_ALLOC) || defined(HEAP_TRACK_ALLOC) || defined(ENABLE_DEBUG_CONFIG_OPTIONS)
#ifdef __clang__
#include <typeinfo>
using std::type_info;
#endif
#endif

#include "CommonPal.h"

// === Core Header Files ===
#include "Core/CommonTypedefs.h"
#include "Core/Api.h"
#include "Core/CriticalSection.h"
#include "Core/Assertions.h"

// === Exceptions Header Files ===
#include "Exceptions/Throw.h"
#include "Exceptions/ExceptionCheck.h"
#include "Exceptions/ReportError.h"

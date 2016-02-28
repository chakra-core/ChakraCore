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

#include "CommonMinMemory.h"

// === C Runtime Header Files ===
#ifndef USING_PAL_STDLIB
#include <wchar.h>

#if defined(_UCRT)
#include <cmath>
#else
#include <math.h>
#endif
#else
#include "CommonPal.h"
#endif

// === Codex Header Files ===
#include "Codex/Utf8Codex.h"




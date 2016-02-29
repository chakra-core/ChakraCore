//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

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




//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#ifdef HAS_ICU
#ifdef WINDOWS10_ICU
#include <icu.h>
#else
#define U_STATIC_IMPLEMENTATION 1
#define U_SHOW_CPLUSPLUS_API 0
#include "unicode/ucal.h"
#include "unicode/ucol.h"
#include "unicode/udat.h"
#include "unicode/udatpg.h"
#include "unicode/uloc.h"
#include "unicode/unumsys.h"
#include "unicode/ustring.h"
#include "unicode/unorm2.h"
#endif
#endif

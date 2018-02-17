//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#ifdef INTL_ICU // TODO(jahorto): this needs to eventually be HAS_ICU, but HAS_ICU (HAS_REAL_ICU) and ENABLE_GLOBALIZATION conflict
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
#endif
#endif

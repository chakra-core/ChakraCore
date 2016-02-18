//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "TargetVer.h"

#include <windows.h>
#include <winbase.h>
#include <oleauto.h>
#pragma warning(disable:4985)
#include <intrin.h>
#include <wtypes.h>

#include <stdio.h>
#include <tchar.h>

// This is an intentionally lame name because we can't use Assert or Verify etc

#define VerifyCondition(expr) (DoVerify((expr), #expr, __FILE__, __LINE__))

void DoVerify(bool value, const char * expr, const char * file, int line);

inline unsigned int GetRandomInteger(unsigned int limit)
{
    return rand() % limit;
}

#include "WeightedTable.h"

#include "Common.h"
#include "CommonInl.h"

extern Recycler * recyclerInstance;

#include "GCStress.h"
#include "RecyclerTestObject.h"


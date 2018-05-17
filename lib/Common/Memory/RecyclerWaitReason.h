//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#include "Core/CommonTypedefs.h"

enum RecyclerWaitReason : uint8
{
#define P(n) n,
#include "RecyclerWaitReasonInc.h"
#undef P
};

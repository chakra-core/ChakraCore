//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

enum class RejitReason : byte
{
#define REJIT_REASON(n) n,
#include "RejitReasons.h"
#undef REJIT_REASON
};

extern const char *const RejitReasonNames[];
extern const uint NumRejitReasons;

extern const char *const GetRejitReasonName(RejitReason reason);

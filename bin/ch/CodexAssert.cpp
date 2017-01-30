//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "stdafx.h"

#ifndef CHAKRA_STATIC_LIBRARY
// The Codex library requires this assertion.
void CodexAssert(bool condition)
{
    Assert(condition);
}
#endif

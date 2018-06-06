//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeBasePch.h"

namespace Js
{
    LPCUTF8 const ISourceHolder::emptyString = (LPCUTF8)"\0";
    SimpleSourceHolder const ISourceHolder::emptySourceHolder(NO_WRITE_BARRIER_TAG(emptyString), 0, true);
}

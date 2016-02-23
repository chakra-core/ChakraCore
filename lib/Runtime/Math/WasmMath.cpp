//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeMathPch.h"

#ifdef ENABLE_WASM

namespace Wasm
{
/* static */
int
WasmMath::Ctz(int value)
{
    DWORD index;
    if (_BitScanForward(&index, value))
    {
        return index;
    }
    return 32;
}

}
#endif

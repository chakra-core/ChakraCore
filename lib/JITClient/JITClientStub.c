//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma warning(push)
// strcpy has inconsistent annotation
#pragma warning(disable: 28252)
#pragma warning(disable: 28253)

// HACK HACK HACK
// MIDL gives compile error if using [system_handle] with stub targetting win8 or below,
// but there is no issue unless the function using [system_handle] is actually called.
// We have runtime check that prevents that function from being used on old OS,
// so change #define here to bypass the error
#if !(0x603 <= _WIN32_WINNT)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x603
#endif

#include "ChakraJIT_c.c"

#pragma warning(pop)

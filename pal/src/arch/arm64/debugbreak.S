//-------------------------------------------------------------------------------------------------------
// ChakraCore/Pal
// Contains portions (c) copyright Microsoft, portions copyright (c) the .NET Foundation and Contributors
// and edits (c) copyright the ChakraCore Contributors.
// See THIRD-PARTY-NOTICES.txt in the project root for .NET Foundation license
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "unixasmmacros.inc"

LEAF_ENTRY DBG_DebugBreak, _TEXT
    EMIT_BREAKPOINT
    ret
LEAF_END_MARKED DBG_DebugBreak, _TEXT


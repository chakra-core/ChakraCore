//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// NOTE: This file is intended to be "#include" multiple times.  The call site must define the macro
// "MACRO" to be executed for each entry.
//
#if !defined(DEF_OP)
#error DEF_OP must be defined before including this file
#endif

// -----------------------------------------------------------------------------------------------
// Additional machine independent opcode used byte backend
#define MACRO_BACKEND_ONLY_WITH_DBG_ATTR(opcode, layout, attr, dbgAttrib) \
    DEF_OP(opcode, layout, attr, OpDbgAttr_BackEndOnly|dbgAttrib)

#include "ByteCode/OpCodes.h"

DEF_OP(MDStart, Empty, None, OpDbgAttr_BackEndOnly)

#define MACRO(opcode, layout, attr, ...) DEF_OP(opcode, layout, attr, OpDbgAttr_BackEndOnly)

#ifdef _M_AMD64
    #include "../../Backend/amd64/MdOpCodes.h"
#elif defined(_M_IX86)
    #include "../../Backend/i386/MdOpCodes.h"
#elif defined(_M_ARM)
    #include "../../Backend/arm/MdOpCodes.h"
#elif defined(_M_ARM64)
    #include "../../Backend/arm64/MdOpCodes.h"
#endif

#undef MACRO


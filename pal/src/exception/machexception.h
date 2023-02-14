//-------------------------------------------------------------------------------------------------------
// ChakraCore/Pal
// Contains portions (c) copyright Microsoft, portions copyright (c) the .NET Foundation and Contributors
// and edits (c) copyright the ChakraCore Contributors.
// See THIRD-PARTY-NOTICES.txt in the project root for .NET Foundation license
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*++

Module Name:

    machexception.h

Abstract:
    Private mach exception handling utilities for SEH

--*/

#ifndef _MACHEXCEPTION_H_
#define _MACHEXCEPTION_H_

#include <mach/mach.h>
#include <mach/mach_error.h>
#include <mach/thread_status.h>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// List of exception types we will be watching for
// NOTE: if you change any of these, you need to adapt s_nMachExceptionPortsMax in thread.hpp
#define PAL_EXC_ILLEGAL_MASK   (EXC_MASK_BAD_INSTRUCTION | EXC_MASK_EMULATION)
#define PAL_EXC_DEBUGGING_MASK (EXC_MASK_BREAKPOINT | EXC_MASK_SOFTWARE)
#define PAL_EXC_MANAGED_MASK   (EXC_MASK_BAD_ACCESS | EXC_MASK_ARITHMETIC)
#define PAL_EXC_ALL_MASK       (PAL_EXC_ILLEGAL_MASK | PAL_EXC_DEBUGGING_MASK | PAL_EXC_MANAGED_MASK)

// Process and thread initialization/cleanup/context routines
BOOL SEHInitializeMachExceptions();
void MachExceptionInitializeDebug(void);
PAL_NORETURN void MachSetThreadContext(CONTEXT *lpContext);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _MACHEXCEPTION_H_ */

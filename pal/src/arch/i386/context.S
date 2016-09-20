// -------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
// -------------------------------------------------------------------------------------------------------

#if defined(_DEBUG)
    .text
    .globl _DBG_CheckStackAlignment

_DBG_CheckStackAlignment:
#ifndef __i686__
  // Prolog - at this point we are at aligned - 8 (for the call)
  pushq %rbp                        // aligned -16
  movq %rsp, %rbp

  testl $0xf,%esp      // can get away with esp even on AMD64.
  jz .+3
  int3

  // Epilog
  popq %rbp
  ret
#else // adm64
// Prolog - at this point we are at aligned - 8 (for the call)
pushl %ebp                        // aligned -16
movl %esp, %ebp

testl $0xf,%esp      // can get away with esp even on AMD64.
jz .+3
int3

// Epilog
popl %ebp
ret
#endif
#endif

//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information. 
//

/*++



Module Name:

    cruntime/random.cpp

Abstract:

    Implementation of C runtime functions to do random number generation

--*/

#include "pal/palinternal.h"
#include <sodium.h>

/*++
Function :
RANDOMInitialize

Initialize libsodium

(no parameters)

Return value :
TRUE  if libsodium initialization succeeded
FALSE otherwise
--*/
BOOL RANDOMInitialize(void)
{
    if (sodium_init() == -1)
    {
        return FALSE;
    }
    
    return TRUE;
}


// Implemented in stdlib in the universal CRT
errno_t __cdecl rand_s(unsigned int* randomValue)
{
    *randomValue = randombytes_random();
    return 1;
}

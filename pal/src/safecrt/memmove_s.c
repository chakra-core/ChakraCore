//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//

/***
*memmove_s.c - contains memmove_s routine
*
*
*Purpose:
*       memmove_s() copies a source memory buffer to a destination buffer.
*       Overlapping buffers are treated specially, to avoid propagation.
*
*Revision History:
*       10-07-03  AC   Module created.
*       03-10-04  AC   Return ERANGE when buffer is too small
*       17-08-16  OB   Add additional buffer (unix) for overlapping src / dst
*                      Return ENOMEM when there is no memory for buffer
*******************************************************************************/

// use stdlib instead of PAL defined malloc to avoid forced Wint-to-pointer warning
#include <stdlib.h>
#include <string.h>

#ifndef _VALIDATE_RETURN_ERRCODE
#define _VALIDATE_RETURN_ERRCODE(c, e) \
    if (!(c)) return e
#endif

#ifndef ENOMEM
#define ENOMEM 12
#endif

#ifndef EINVAL
#define EINVAL 22
#endif

#ifndef ERANGE
#define ERANGE 34
#endif

/*
usage: see https://msdn.microsoft.com/en-us/library/8k35d1fx.aspx

notes: uses extra buffer in case the src/dst overlaps (osx/bsd)

dest
    Destination object.

src
    Source object.

count
    Number of bytes (memmove) to copy.
*/
void* __cdecl memmove_xplat(
    void * dst,
    const void * src,
    size_t count
)
{
#if defined(__APPLE__) || defined(__FreeBSD__)
    if (src <= dst && src + count > dst)
    {
        char *temp = (char*) malloc(count);
        _VALIDATE_RETURN_ERRCODE(temp != NULL, NULL);

        memcpy(temp, src, count);
        memcpy(dst, temp, count);

        free(temp);
        return dst;
    }
#endif

    return memmove(dst, src, count);
}


/*
usage: see https://msdn.microsoft.com/en-us/library/e2851we8.aspx

dest
    Destination object.

sizeInBytes
    Size of the destination buffer.

src
    Source object.

count
    Number of bytes (memmove_s) or characters (wmemmove_s) to copy.
*/
int __cdecl memmove_s(
    void * dst,
    size_t sizeInBytes,
    const void * src,
    size_t count
)
{
    if (count == 0)
    {
        /* nothing to do */
        return 0;
    }

    /* validation section */
    _VALIDATE_RETURN_ERRCODE(dst != NULL, EINVAL);
    _VALIDATE_RETURN_ERRCODE(src != NULL, EINVAL);
    _VALIDATE_RETURN_ERRCODE(sizeInBytes >= count, ERANGE);

    void *ret_val = memmove_xplat(dst, src, count);
    return ret_val != NULL ? 0 : ENOMEM; // memmove_xplat returns `NULL` only if ENOMEM
}

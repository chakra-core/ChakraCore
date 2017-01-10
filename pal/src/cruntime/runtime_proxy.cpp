//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//

// do not use other PAL headers here.
// combining wctype.h with PAL headers creates type mismatches under latest NDK

#if HAVE_COREFOUNDATION
#define CF_EXCLUDE_CSTD_HEADERS
#include <CoreFoundation/CoreFoundation.h>
#include <wctype.h>
#else
#include <wctype.h>
#endif

int proxy_iswprint(char16_t c)
{
    return iswprint(c);
}

int proxy_iswspace(char16_t c)
{
    return iswspace(c);
}

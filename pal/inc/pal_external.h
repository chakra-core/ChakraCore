//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// The interface on this file can be consumed from none chakracore dependents
// Keep this file independent from the rest of PAL

#ifndef PAL_INC_PAL_EXTERNAL_H
#define PAL_INC_PAL_EXTERNAL_H
#ifndef _WIN32

namespace CCPAL
{
    unsigned long GetMilliseconds();
}

#define GetTickCount()   CCPAL::GetMilliseconds()
#define GetTickCount64() CCPAL::GetMilliseconds()

#endif // !_WIN32
#endif // PAL_INC_PAL_EXTERNAL_H

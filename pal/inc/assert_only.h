//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// PAL free Assert definitions
#ifdef DEBUG

#define _QUOTE_(s) #s
#define _STRINGIZE_(s) _QUOTE_(s)

#ifndef __ANDROID__
#define _ERR_OUTPUT_(condition, comment) \
    fprintf(stderr, "ASSERTION (%s, line %d) %s %s\n", __FILE__, __LINE__, \
      _STRINGIZE_(condition), comment); \
    fflush(stderr);
#else // ANDROID
#include <android/log.h>
#define _ERR_OUTPUT_(condition, comment) \
    __android_log_print(ANDROID_LOG_ERROR, "chakracore-log", \
      "ASSERTION (%s, line %d) %s %s\n", __FILE__, __LINE__, \
      _STRINGIZE_(condition), comment);
#endif

#define _Assert_(condition, comment)   \
    do { \
        if (!(condition)) \
        { \
            _ERR_OUTPUT_(condition, comment) \
            abort(); \
        } \
    } while (0)

#define Assert(condition) _Assert_(condition, "")
#define AssertMsg(condition, comment) _Assert_(condition, comment)

#else // ! DEBUG

#define Assert(condition)
#define AssertMsg(condition, comment)

#endif

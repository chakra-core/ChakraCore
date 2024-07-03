//-------------------------------------------------------------------------------------------------------
// ChakraCore/Pal
// Contains portions (c) copyright Microsoft, portions copyright (c) the .NET Foundation and Contributors
// and edits (c) copyright the ChakraCore Contributors.
// See THIRD-PARTY-NOTICES.txt in the project root for .NET Foundation license
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// PAL free Assert definitions
#ifdef DEBUG

#if !defined(CHAKRACORE_STRINGIZE)
#define CHAKRACORE_STRINGIZE_IMPL(x) #x
#define CHAKRACORE_STRINGIZE(x) CHAKRACORE_STRINGIZE_IMPL(x)
#endif

#ifndef __ANDROID__
#define _ERR_OUTPUT_(condition, comment) \
    fprintf(stderr, "ASSERTION (%s, line %d) %s %s\n", __FILE__, __LINE__, \
      CHAKRACORE_STRINGIZE(condition), comment); \
    fflush(stderr);
#else // ANDROID
#include <android/log.h>
#define _ERR_OUTPUT_(condition, comment) \
    __android_log_print(ANDROID_LOG_ERROR, "chakracore-log", \
      "ASSERTION (%s, line %d) %s %s\n", __FILE__, __LINE__, \
      CHAKRACORE_STRINGIZE(condition), comment);
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

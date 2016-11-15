//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifdef __clang__
    #undef __try
    #undef __finally
    #define __try
    #define __finally
#else
    #ifndef ENABLE_SEH
        #define ENABLE_SEH 1
    #endif
#endif

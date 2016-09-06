//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimePlatformAgnosticPch.h"
#include "CommonPal.h"

#include <stdint.h>

namespace PlatformAgnostic
{
    Thread::ThreadHandle Thread::Create(unsigned stack_size,
                                        unsigned ( *start_address )( void * ),
                                        void* arg_list,
                                        ThreadInitFlag init_flag)
    {
        unsigned int flag = 0;

        switch (init_flag)
        {
        case ThreadInitRunImmediately:
            flag = 0;
            break;
        case ThreadInitCreateSuspended:
            flag = CREATE_SUSPENDED;
            break;
        case ThreadInitStackSizeParamIsAReservation:
            flag = STACK_SIZE_PARAM_IS_A_RESERVATION;
            break;
        default:
            Assert(false);
        }

        return reinterpret_cast<ThreadHandle>(CreateThread(0, stack_size, start_address, arg_list, flag, 0));
    }
} // namespace PlatformAgnostic

//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeBasePch.h"

ExpirableObject::ExpirableObject(ThreadContext* threadContext):
    registrationHandle(0)
{
    if (threadContext)
    {
        threadContext->RegisterExpirableObject(this);
    }
}

void ExpirableObject::Finalize(bool isShutdown)
{
    if (!isShutdown && this->GetRegistrationHandle() != nullptr)
    {
        ThreadContext* threadContext = ThreadContext::GetContextForCurrentThread();

        threadContext->UnregisterExpirableObject(this);
    }
}

void ExpirableObject::Dispose(bool isShutdown)
{
}

void * ExpirableObject::GetRegistrationHandle()
{
    return (void *)(registrationHandle & ~1);
}

void 
ExpirableObject::SetRegistrationHandle(void * registrationHandle)
{
    Assert(this->GetRegistrationHandle() == nullptr);
    Assert(((intptr_t)registrationHandle & 1) == 0);
    Assert(registrationHandle != nullptr);
    this->registrationHandle |= (intptr_t)registrationHandle;
}

void
ExpirableObject::ClearRegistrationHandle()
{
    Assert(this->GetRegistrationHandle() != nullptr);
    this->registrationHandle = this->registrationHandle & 1;
}

void ExpirableObject::EnterExpirableCollectMode()
{
    this->registrationHandle &= ~1;
}

bool ExpirableObject::IsObjectUsed()
{
    return (this->registrationHandle & 1);
}

void ExpirableObject::SetIsObjectUsed()
{
    this->registrationHandle |= 1;
}
